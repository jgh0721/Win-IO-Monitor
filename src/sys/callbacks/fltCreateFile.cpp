#include "fltCreateFile.hpp"

#include "irpContext.hpp"
#include "notifyMgr.hpp"
#include "WinIOMonitor_Event.hpp"
#include "WinIOMonitor_W32API.hpp"
#include "utilities/contextMgr.hpp"
#include "utilities/fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS WinIOPreCreate( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                          PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS       FltStatus       = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                    IrpContext      = NULLPTR;
    CTX_INSTANCE_CONTEXT*           InstanceContext = NULLPTR;

    __try
    {
        if( BooleanFlagOn( Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY ) || 
            BooleanFlagOn( Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE ) )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );
        CtxGetContext( FltObjects, NULLPTR, FLT_INSTANCE_CONTEXT, ( PFLT_CONTEXT* )&InstanceContext );

        if( IrpContext != NULLPTR )
        {
            PrintIrpContext( IrpContext );
            *CompletionContext = IrpContext;
            FltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
        }
    }
    __finally
    {
        CtxReleaseContext( InstanceContext );
    }
    
    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS WinIOPostCreate( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                            PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    UNREFERENCED_PARAMETER( Flags );

    NTSTATUS Status = STATUS_SUCCESS;
    auto IrpContext = ( PIRP_CONTEXT )CompletionContext;

    if( NT_SUCCESS( Data->IoStatus.Status ) && Data->IoStatus.Status != STATUS_REPARSE )
    {
        Status = CtxGetOrSetContext( FltObjects, Data->Iopb->TargetFileObject, ( PFLT_CONTEXT* )&IrpContext->StreamContext, FLT_STREAM_CONTEXT );
        if( NT_SUCCESS( Status ) )
        {
            KeEnterCriticalRegion();
            ExAcquireResourceExclusiveLite( IrpContext->StreamContext->Resource, TRUE );

            IrpContext->StreamContext->CreateCount++;

            //
            //  Set DeleteOnClose on the stream context: a delete-on-close stream will
            //  always be checked for deletion on cleanup.
            //

            IrpContext->StreamContext->DeleteOnClose = BooleanFlagOn( Data->Iopb->Parameters.Create.Options,
                                                                      FILE_DELETE_ON_CLOSE );

            ExReleaseResourceLite( IrpContext->StreamContext->Resource );
            KeLeaveCriticalRegion();

            auto SecurityContext = Data->Iopb->Parameters.Create.SecurityContext;
            auto CreateOptions = Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;

            auto DesiredAccess = SecurityContext->DesiredAccess;
            auto CreateDisposition = ( Data->Iopb->Parameters.Create.Options >> 24 ) & 0x000000ff;

            if( Data->IoStatus.Information == FILE_CREATED )
            {
                CheckEvent( IrpContext, IrpContext->Data, IrpContext->FltObjects, FILE_WAS_CREATED );
                if( IrpContext->isSendTo == true )
                {
                    ULONG PacketSize = sizeof( MSG_SEND_PACKET ) + IrpContext->ProcessFullPath.BufferSize + IrpContext->SrcFileFullPath.BufferSize + IrpContext->DstFileFullPath.BufferSize;
                    auto NotifyItem = AllocateNotifyItem( PacketSize );

                    if( NotifyItem != NULLPTR )
                    {
                        auto SendPacket = NotifyItem->SendPacket;

                        SendPacket->MessageSize = PacketSize;
                        SendPacket->MessageCategory = MSG_CATE_FILESYSTEM_NOTIFY;
                        SendPacket->MessageType = FILE_WAS_CREATED;
                        SendPacket->IsNotified = TRUE;
                        KeQuerySystemTime( &SendPacket->EventTime );

                        SendPacket->ProcessId = IrpContext->ProcessId;

                        SendPacket->LengthOfSrcFileFullPath = ( nsUtils::strlength( IrpContext->SrcFileFullPath.Buffer ) + 1 ) * sizeof( WCHAR );
                        SendPacket->OffsetOfSrcFileFullPath = sizeof( MSG_SEND_PACKET );
                        SendPacket->LengthOfDstFileFullPath = 0;
                        SendPacket->OffsetOfDstFileFullPath = SendPacket->OffsetOfSrcFileFullPath + SendPacket->LengthOfSrcFileFullPath;
                        SendPacket->LengthOfProcessFullPath = ( nsUtils::strlength( IrpContext->ProcessFullPath.Buffer ) + 1 ) * sizeof( WCHAR );
                        SendPacket->OffsetOfProcessFullPath = SendPacket->OffsetOfDstFileFullPath + SendPacket->LengthOfDstFileFullPath;

                        RtlStringCbCopyW( ( PWCH )Add2Ptr( SendPacket, SendPacket->OffsetOfSrcFileFullPath ), SendPacket->LengthOfSrcFileFullPath,
                                          IrpContext->SrcFileFullPath.Buffer );
                        RtlStringCbCopyW( ( PWCH )Add2Ptr( SendPacket, SendPacket->OffsetOfProcessFullPath ), SendPacket->LengthOfProcessFullPath,
                                          IrpContext->ProcessFullPath.Buffer );

                        SendPacket->LengthOfContents = 0;

                        AppendNotifyItem( NotifyItem );
                    }
                }
            }

            Data->IoStatus.Information;
            //FILE_CREATED

            //    FILE_OPENED

            //    FILE_OVERWRITTEN

            //    FILE_SUPERSEDED

            //    FILE_EXISTS

            //    FILE_DOES_NOT_EXIST

            switch( CreateDisposition )
            {
                case FILE_SUPERSEDE: {} break;
                case FILE_OPEN: {} break;
                case FILE_CREATE: {} break;
                case FILE_OPEN_IF: {} break;
                case FILE_OVERWRITE: {} break;
                case FILE_OVERWRITE_IF: {} break;
            }
        }

        if( !NT_SUCCESS( Status ) )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d %s %s Status=0x%08x\n",
                         IrpContext->EvtID, __FUNCTION__, "CtxGetOrSetContext FAILED", Status ) );
        }
    }

    CloseIrpContext( IrpContext );

    return FLT_POSTOP_FINISHED_PROCESSING;
}
