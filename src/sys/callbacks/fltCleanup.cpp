#include "fltCleanup.hpp"

#include "irpContext.hpp"
#include "notifyMgr.hpp"
#include "WinIOMonitor_Event.hpp"
#include "utilities/contextMgr.hpp"
#include "utilities/fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

FLT_PREOP_CALLBACK_STATUS FLTAPI WinIOPreCleanup( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                IrpContext = CreateIrpContext( Data, FltObjects );

    do
    {
        if( IrpContext != NULLPTR )
        {
            if( IrpContext->StreamContext != NULLPTR )
            {
                KeEnterCriticalRegion();
                ExAcquireResourceExclusiveLite( IrpContext->StreamContext->Resource, TRUE );

                IrpContext->StreamContext->CleanupCount++;

                //
                //  Only streams with stream context will be sent for deletion check
                //  in post-cleanup, which makes sense because they would only ever
                //  have one if they were flagged as candidates at some point.
                //
                //  Gather file information here so that we have a name to report.
                //  The name will be accurate most of the times, and in the cases it
                //  won't, it serves as a good clue and the stream context pointer
                //  value should offer a way to disambiguate that in case of renames
                //  etc.
                //

                nsUtils::DfGetFileNameInformation( Data, IrpContext->StreamContext );

                ExReleaseResourceLite( IrpContext->StreamContext->Resource );
                KeLeaveCriticalRegion();
            }

            PrintIrpContext( IrpContext );
            FltStatus = FLT_PREOP_SYNCHRONIZE;
            *CompletionContext = IrpContext;
        }

    } while( false );

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI WinIOPostCleanup( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    auto                        IrpContext = ( IRP_CONTEXT* )CompletionContext;

    do
    {
        if( NT_SUCCESS( Data->IoStatus.Status ) && IrpContext->StreamContext != NULLPTR )
        {
            PCTX_STREAM_CONTEXT StreamContext = IrpContext->StreamContext;

            //
            //  Determine whether or not we should check for deletion. What
            //  flags a file as a deletion candidate is one or more of the following:
            //
            //  1. NumOps > 0. This means there are or were racing changes to
            //  the file delete disposition state, and, in that case,
            //  we don't know what that state is. So, let's err to the side of
            //  caution and check if it was deleted.
            //
            //  2. SetDisp. If this is TRUE and we haven't raced in setting delete
            //  disposition, this reflects the true delete disposition state of the
            //  file, meaning we must check for deletes if it is set to TRUE.
            //
            //  3. DeleteOnClose. If the file was ever opened with
            //  FILE_DELETE_ON_CLOSE, we must check to see if it was deleted.
            //  FileDispositionInformationEx allows the this flag to be unset.
            //
            //  Also, if a deletion of this stream was already notified, there is no
            //  point notifying it again.
            //

            if( ( ( StreamContext->NumOps > 0 ) ||
                  ( StreamContext->SetDisp ) ||
                  ( StreamContext->DeleteOnClose ) ) &&
                ( 0 == StreamContext->IsNotified ) )
            {
                NTSTATUS Status = STATUS_UNSUCCESSFUL;
                FILE_STANDARD_INFORMATION fileInfo;

                //
                //  The check for deletion is done via a query to
                //  FileStandardInformation. If that returns STATUS_FILE_DELETED
                //  it means the stream was deleted.
                //

                Status = FltQueryInformationFile( Data->Iopb->TargetInstance,
                                                  Data->Iopb->TargetFileObject,
                                                  &fileInfo,
                                                  sizeof( fileInfo ),
                                                  FileStandardInformation,
                                                  NULL );

                if( STATUS_FILE_DELETED == Status )
                {

                    Status = DfProcessDelete( Data,
                                              FltObjects,
                                              IrpContext );

                    if( !NT_SUCCESS( Status ) )
                    {

                        //DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                        //              "delete!%s: It was not possible to verify "
                        //              "deletion due to an error in DfProcessDelete (0x%08x)!\n",
                        //              __FUNCTION__,
                        //              status );
                    }
                }
            }
        }

    } while( false );

    CloseIrpContext( IrpContext );

    return FltStatus;
}

NTSTATUS DfProcessDelete( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, IRP_CONTEXT* IrpContext )
{
    BOOLEAN isTransaction;
    BOOLEAN isFileDeleted = FALSE;
    NTSTATUS status;
    PCTX_TRANSACTION_CONTEXT transactionContext = NULL;

    PAGED_CODE();

    //  Is this in a transacted context?
    isTransaction = ( NULL != FltObjects->Transaction );

    if( isTransaction )
    {
        //DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
        //              "delete!DfProcessDelete: In a transaction!\n" );

        status = CtxGetOrSetContext( FltObjects,
                                     FltObjects->Transaction,
                                     ( PFLT_CONTEXT* )&transactionContext,
                                     FLT_TRANSACTION_CONTEXT );

        if( !NT_SUCCESS( status ) )
        {

            return status;
        }
    }

    //
    //  Notify deletion. If this is an Alternate Data Stream being deleted,
    //  check if the whole file was deleted (by calling DfIsFileDeleted) as
    //  this could be the last handle to a delete-pending file.
    //

    status = nsUtils::DfIsFileDeleted( Data,
                                       FltObjects,
                                       IrpContext->StreamContext,
                                       isTransaction );

    if( STATUS_FILE_DELETED == status )
    {

        isFileDeleted = TRUE;
        status = STATUS_SUCCESS;

    }
    else if( !NT_SUCCESS( status ) )
    {

        //DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
        //              "delete!%s: DfIsFileDeleted returned 0x%08x!\n",
        //              __FUNCTION__,
        //              status );

        goto _exit;
    }

    DfNotifyDelete( IrpContext,
                    isFileDeleted,
                    transactionContext );

_exit:

    if( NULL != transactionContext )
    {
        FltReleaseContext( transactionContext );
    }

    return status;
}

void DfNotifyDelete( PIRP_CONTEXT IrpContext, BOOLEAN IsFile, PCTX_TRANSACTION_CONTEXT TransactionContext )
{
    PAGED_CODE();

    if( InterlockedIncrement( &IrpContext->StreamContext->IsNotified ) > 1 )
        return;

    if( IsFile != FALSE )
    {
        if( TransactionContext == NULLPTR )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, "[WinIOMon] %s %s %ws\n",
                         __FUNCTION__, "File Deleted : ", IrpContext->StreamContext->FileFullPath.Buffer ) );

            CheckEvent( IrpContext, IrpContext->Data, IrpContext->FltObjects, FILE_WAS_DELETED );
            if( IrpContext->isSendTo == true )
            {
                ULONG PacketSize = sizeof( MSG_SEND_PACKET ) + IrpContext->ProcessFullPath.BufferSize + IrpContext->SrcFileFullPath.BufferSize;
                auto NotifyItem = AllocateNotifyItem( PacketSize );

                if( NotifyItem != NULLPTR )
                {
                    auto SendPacket = NotifyItem->SendPacket;

                    SendPacket->MessageSize = PacketSize;
                    SendPacket->MessageCategory = MSG_CATE_FILESYSTEM_NOTIFY;
                    SendPacket->MessageType = FILE_WAS_DELETED;
                    SendPacket->IsNotified = TRUE;
                    KeQuerySystemTime( &SendPacket->EventTime );

                    SendPacket->ProcessId = IrpContext->ProcessId;

                    SendPacket->LengthOfSrcFileFullPath = ( nsUtils::strlength( IrpContext->SrcFileFullPath.Buffer ) + 1 ) * sizeof( WCHAR );
                    SendPacket->OffsetOfSrcFileFullPath = SendPacket->OffsetOfProcessFullPath + SendPacket->LengthOfProcessFullPath;
                    SendPacket->LengthOfDstFileFullPath = 0;
                    SendPacket->LengthOfProcessFullPath = ( nsUtils::strlength( IrpContext->ProcessFullPath.Buffer ) + 1 ) * sizeof( WCHAR );
                    SendPacket->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );

                    RtlStringCbCopyW( ( PWCH )Add2Ptr( SendPacket, SendPacket->OffsetOfProcessFullPath ), SendPacket->LengthOfProcessFullPath,
                                      IrpContext->ProcessFullPath.Buffer );
                    RtlStringCbCopyW( ( PWCH )Add2Ptr( SendPacket, SendPacket->OffsetOfSrcFileFullPath ), SendPacket->LengthOfSrcFileFullPath,
                                      IrpContext->SrcFileFullPath.Buffer );

                    SendPacket->LengthOfContents = 0;

                    AppendNotifyItem( NotifyItem );
                }
            }
        }
        else
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, "[WinIOMon] %s %s %ws\n",
                         __FUNCTION__, "File Deleted In a Transaction : ", IrpContext->StreamContext->FileFullPath.Buffer ) );

            DfAddTransDeleteNotify( IrpContext->StreamContext,
                                    TransactionContext,
                                    IsFile );
        }
    }
    else
    {
        // Alternate Data Stream is deleted

        if( TransactionContext == NULLPTR )
        {
            
        }
        else
        {
            DfAddTransDeleteNotify( IrpContext->StreamContext,
                                    TransactionContext,
                                    IsFile );
        }
    }
}

NTSTATUS DfAddTransDeleteNotify( PCTX_STREAM_CONTEXT StreamContext, PCTX_TRANSACTION_CONTEXT TransactionContext, BOOLEAN FileDelete )
{
    PDF_DELETE_NOTIFY deleteNotify;

    PAGED_CODE();

    ASSERT( NULL != TransactionContext->Resource );

    ASSERT( NULL != StreamContext );

    deleteNotify = ( PDF_DELETE_NOTIFY )ExAllocatePoolWithTag( NonPagedPool,
                                                               sizeof( DF_DELETE_NOTIFY ),
                                                               'abcd' );

    if( NULL == deleteNotify )
    {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( deleteNotify, sizeof( DF_DELETE_NOTIFY ) );

    FltReferenceContext( StreamContext );
    deleteNotify->StreamContext = StreamContext;
    deleteNotify->FileDelete = FileDelete;

    FltAcquireResourceExclusive( TransactionContext->Resource );

    InsertTailList( &TransactionContext->DeleteNotifyList,
                    &deleteNotify->Links );

    FltReleaseResource( TransactionContext->Resource );

    return STATUS_SUCCESS;
}
