#include "detectDelete.hpp"


#include "irpContext.hpp"
#include "privateFCBMgr.hpp"
#include "utilities/contextMgr.hpp"
#include "communication/Communication.hpp"

#include "W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS SolProcessDelete( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status;
    BOOLEAN isFileDeleted = FALSE;
    PCTX_TRANSACTION_CONTEXT transactionContext = NULLPTR;

    //  Is this in a transacted context?
    bool IsTransaction = IrpContext->FltObjects->Transaction != NULLPTR;

    do
    {
        if( IsTransaction )
        {
            Status = CtxGetOrSetContext( IrpContext->FltObjects,
                                         IrpContext->FltObjects->Transaction,
                                         (PFLT_CONTEXT*)&transactionContext,
                                         FLT_TRANSACTION_CONTEXT );

            if( !NT_SUCCESS( Status ) )
                break;
        }

        //
        //  Notify deletion. If this is an Alternate Data Stream being deleted,
        //  check if the whole file was deleted (by calling DfIsFileDeleted) as
        //  this could be the last handle to a delete-pending file.
        //

        Status = SolIfFileDeleted( IrpContext, IsTransaction );

        if( Status == STATUS_FILE_DELETED )
        {
            isFileDeleted = TRUE;
            Status = STATUS_SUCCESS;
        }
        else if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] %s EvtID=%09d %s %s Status=0x%08x,%s\n"
                       , JudgeInOut( IrpContext ), IrpContext->EvtID, __FUNCTION__
                       , "SolIfFileDeleted FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message) );

            break;
        }

        SolNotifyDelete( IrpContext, isFileDeleted, transactionContext );

    } while( false );

    if( transactionContext != NULLPTR )
    {
        FltReleaseContext( transactionContext );
    }

    return Status;
}

void SolNotifyDelete( IRP_CONTEXT* IrpContext, BOOLEAN IsFile, CTX_TRANSACTION_CONTEXT* TransactionContext )
{
    const auto StreamContext = IrpContext->StreamContext;

    LONG IsNotified = InterlockedIncrement( &StreamContext->IsNotified );
    if( IsNotified > 1 )
        return;

    if( IsFile && TransactionContext == NULLPTR )
    {
        if( TransactionContext == NULLPTR )
        {
            KdPrintEx( ( DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[WinIOSol] %s EvtID=%09d %s %s %ws\n"
                         , JudgeInOut( IrpContext ), IrpContext->EvtID, __FUNCTION__ 
                         , "File Deleted!", IrpContext->SrcFileFullPath.Buffer ) );

            if( IrpContext->IsConcerned == true && FeatureContext.IsRunning > 0 )
                NotifyEventFileDeleteTo( IrpContext );
        }
        else
        {
            KdPrintEx( ( DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[WinIOSol] %s EvtID=%09d %s %s %ws\n"
                         , JudgeInOut( IrpContext ), IrpContext->EvtID, __FUNCTION__
                         , "File Deleted In Transaction", IrpContext->SrcFileFullPath.Buffer ) );

            if( IrpContext->IsConcerned == true && FeatureContext.IsRunning > 0 )
                SolAddTransDeleteNotify( IrpContext, TransactionContext, IsFile );
        }
    }
    else
    {
        if( TransactionContext == NULLPTR )
        {
            KdPrintEx( ( DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[WinIOSol] %s EvtID=%09d %s %s %ws\n"
                         , JudgeInOut( IrpContext ), IrpContext->EvtID, __FUNCTION__
                         , "Alternate FileStream Deleted!", IrpContext->SrcFileFullPath.Buffer ) );
        }
        else
        {
            KdPrintEx( ( DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[WinIOSol] %s EvtID=%09d %s %s %ws\n"
                         , JudgeInOut( IrpContext ), IrpContext->EvtID, __FUNCTION__
                         , "Alternate FileStream Deleted In Transaction", IrpContext->SrcFileFullPath.Buffer ) );

            if( IrpContext->IsConcerned == true && FeatureContext.IsRunning > 0 )
                SolAddTransDeleteNotify( IrpContext, TransactionContext, IsFile );

        }
    }
}

NTSTATUS SolIfFileDeleted( IRP_CONTEXT* IrpContext, bool IsTransaction )
{
    NTSTATUS Status = STATUS_SUCCESS;
    FILE_OBJECTID_BUFFER fileObjectIdBuf;

    //
    //  We need to know whether we're on ReFS or NTFS.
    //

    FLT_FILESYSTEM_TYPE fileSystemType = IrpContext->InstanceContext->VolumeFileSystemType;

    //
    //  FSCTL_GET_OBJECT_ID does not return STATUS_FILE_DELETED if the
    //  file was deleted in a transaction, and this is why we need another
    //  method for detecting if the file is still present: opening by ID.
    //
    //  If we're on ReFS we also need to open by file ID because ReFS does not
    //  support object IDs.
    //

    if( IsTransaction ||
        ( fileSystemType == nsW32API::FLT_FSTYPE_REFS ) )
    {
        Status = SolDetectDeleteByFileId( IrpContext );

        switch( Status )
        {
            case STATUS_INVALID_PARAMETER:

                //
                //  The file was deleted. In this case, trying to open it
                //  by ID returns STATUS_INVALID_PARAMETER.
                //

                return STATUS_FILE_DELETED;

            case STATUS_DELETE_PENDING:

                //
                //  In this case, the main file still exists, but is in
                //  a delete pending state, so we return STATUS_SUCCESS,
                //  signaling it still exists and wasn't deleted by this
                //  operation.
                //

                return STATUS_SUCCESS;

            default:

                return Status;
        }
    }
    else
    {
        //
        //  When not in a transaction, attempting to get the object ID of the
        //  file is a cheaper alternative compared to opening the file by ID.
        //

        Status = FltFsControlFile( IrpContext->Data->Iopb->TargetInstance,
                                   IrpContext->Data->Iopb->TargetFileObject,
                                   FSCTL_GET_OBJECT_ID,
                                   NULL,
                                   0,
                                   &fileObjectIdBuf,
                                   sizeof( FILE_OBJECTID_BUFFER ),
                                   NULL );

        switch( Status )
        {
            case STATUS_OBJECTID_NOT_FOUND:

                //
                //  Getting back STATUS_OBJECTID_NOT_FOUND means the file
                //  still exists, it just doesn't have an object ID.

                return STATUS_SUCCESS;

            default:

                //
                //  Else we just get back STATUS_FILE_DELETED if the file
                //  doesn't exist anymore, or some error status, so no
                //  status conversion is necessary.
                //

                NOTHING;
        }
    }

    return Status;
}

NTSTATUS SolDetectDeleteByFileId( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status;
    UNICODE_STRING fileIdString;
    HANDLE FileHandle = NULLPTR;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    IO_DRIVER_CREATE_CONTEXT driverCreateContext;

    do
    {
        //
        //  First build the file ID string.  Note that this may fail with STATUS_FILE_DELETED
        //  and short-circuit our open-by-ID.  Since we're really trying to see if
        //  the file is deleted, that's perfectly okay.
        //

        Status = SolBuildFileIdString( IrpContext, &fileIdString );

        if( !NT_SUCCESS( Status ) )
            break;

        InitializeObjectAttributes( &objectAttributes,
                                    &fileIdString,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        //
        //  It is important to initialize the IO_DRIVER_CREATE_CONTEXT structure's
        //  TxnParameters. We'll always want to do this open on behalf of a
        //  transaction because opening the file by ID is the method we use to
        //  detect if the whole file still exists when we're in a transaction.
        //

        IoInitializeDriverCreateContext( &driverCreateContext );
        if( GlobalNtOsKrnlMgr.Is_IoGetTransactionParameterBlock() == true )
            driverCreateContext.TxnParameters = GlobalNtOsKrnlMgr.IoGetTransactionParameterBlock( IrpContext->Data->Iopb->TargetFileObject );

        Status = nsW32API::FltCreateFileEx2( GlobalContext.Filter,
                                             IrpContext->Data->Iopb->TargetInstance,
                                             &FileHandle,
                                             NULL,
                                             FILE_READ_ATTRIBUTES,
                                             &objectAttributes,
                                             &ioStatus,
                                             ( PLARGE_INTEGER )NULL,
                                             0L,
                                             FILE_SHARE_VALID_FLAGS,
                                             FILE_OPEN,
                                             FILE_OPEN_REPARSE_POINT | FILE_OPEN_BY_FILE_ID,
                                             ( PVOID )NULL,
                                             0L,
                                             IO_IGNORE_SHARE_ACCESS_CHECK,
                                             &driverCreateContext );

    } while( false );

    if( FileHandle != NULLPTR )
        FltClose( FileHandle );

    SolFreeUnicodeString( &fileIdString );

    return Status;
}

NTSTATUS SolBuildFileIdString( IRP_CONTEXT* IrpContext, UNICODE_STRING* String )
{
    NTSTATUS Status;
    ASSERT( NULL != String );

    //
    //  We'll compose the string with:
    //  1. The volume GUID name.
    //  2. A backslash
    //  3. The File ID.
    //

    do
    {
        //
        //  Make sure the file ID is loaded in the StreamContext.  Note that if the
        //  file has been deleted DfGetFileId will return STATUS_FILE_DELETED.
        //  Since we're interested in detecting whether the file has been deleted
        //  that's fine; the open-by-ID will not actually take place.  We have to
        //  ensure it is loaded before building the string length below since we
        //  may get either a 64-bit or 128-bit file ID back.
        //

        Status = SolGetFileId( IrpContext->FltObjects->Instance,
                               IrpContext->FltObjects->FileObject,
                               IrpContext->StreamContext );

        if( !NT_SUCCESS( Status ) )
            break;

        //
        //  First add the lengths of 1, 2, 3 and allocate accordingly.
        //  Note that ReFS understands both 64- and 128-bit file IDs when opening
        //  by ID, so whichever size we get back from DfSizeofFileId will work.
        //

        String->MaximumLength = VOLUME_GUID_NAME_SIZE * sizeof( WCHAR ) + sizeof( WCHAR ) +
            UnifiedSizeofFileId( IrpContext->StreamContext->FileId );

        Status = SolAllocateUnicodeString( String );

        if( !NT_SUCCESS( Status ) )
        {

            return Status;
        }

        //
        //  Now obtain the volume GUID name with a trailing backslash (1 + 2).
        //

        // obtain volume GUID name here and cache it in the InstanceContext.
        Status = SolGetVolumeGuidName( IrpContext->FltObjects->Volume,
                                       IrpContext->InstanceContext,
                                       String );

        if( !NT_SUCCESS( Status ) )
        {

            SolFreeUnicodeString( String );

            return Status;
        }

        //
        //  Now append the file ID to the end of the string.
        //

        RtlCopyMemory( Add2Ptr( String->Buffer, String->Length ),
                       &IrpContext->StreamContext->FileId,
                       UnifiedSizeofFileId( IrpContext->StreamContext->FileId ) );

        String->Length += UnifiedSizeofFileId( IrpContext->StreamContext->FileId );

        ASSERT( String->Length == String->MaximumLength );
    } while( false );

    return Status;
}

NTSTATUS SolGetFileId( PFLT_INSTANCE Instance, PFILE_OBJECT FileObject, CTX_STREAM_CONTEXT* StreamContext )
{
    NTSTATUS status = STATUS_SUCCESS;
    FILE_INTERNAL_INFORMATION fileInternalInformation;

    //
    //  Only query the file system for the file ID for the first time.
    //  This is just an optimization.  It doesn't need any real synchronization
    //  because file IDs don't change.
    //

    if( !StreamContext->FileIdSet )
    {

        //
        //  Querying for FileInternalInformation gives you the file ID.
        //

        status = FltQueryInformationFile( Instance,
                                          FileObject,
                                          &fileInternalInformation,
                                          sizeof( FILE_INTERNAL_INFORMATION ),
                                          FileInternalInformation,
                                          NULL );

        if( NT_SUCCESS( status ) )
        {

            //
            //  ReFS uses 128-bit file IDs.  FileInternalInformation supports 64-
            //  bit file IDs.  ReFS signals that a particular file ID can only
            //  be represented in 128 bits by returning FILE_INVALID_FILE_ID as
            //  the file ID.  In that case we need to use FileIdInformation.
            //

            if( fileInternalInformation.IndexNumber.QuadPart == FILE_INVALID_FILE_ID )
            {
                nsW32API::FILE_ID_INFORMATION fileIdInformation;

                status = FltQueryInformationFile( Instance,
                                                  FileObject,
                                                  &fileIdInformation,
                                                  sizeof( nsW32API::FILE_ID_INFORMATION ),
                                                  (FILE_INFORMATION_CLASS)nsW32API::FileIdInformation,
                                                  NULL );
                
                if( NT_SUCCESS( status ) )
                {

                    //
                    //  We don't use DfSizeofFileId() here because we are not
                    //  measuring the size of a DF_FILE_REFERENCE.  We know we have
                    //  a 128-bit value.
                    //

                    RtlCopyMemory( &StreamContext->FileId,
                                   &fileIdInformation.FileId,
                                   sizeof( StreamContext->FileId ) );

                    //
                    //  Because there's (currently) no support for 128-bit values in
                    //  the compiler we need to ensure the setting of the ID and our
                    //  remembering that the file ID was set occur in the right order.
                    //

                    KeMemoryBarrier();

                    StreamContext->FileIdSet = TRUE;
                }

            }
            else
            {

                StreamContext->FileId.FileId64.Value = fileInternalInformation.IndexNumber.QuadPart;
                StreamContext->FileId.FileId64.UpperZeroes = 0ll;

                //
                //  Because there's (currently) no support for 128-bit values in
                //  the compiler we need to ensure the setting of the ID and our
                //  remembering that the file ID was set occur in the right order.
                //

                KeMemoryBarrier();

                StreamContext->FileIdSet = TRUE;
            }
        }
    }

    return status;
}

NTSTATUS SolGetVolumeGuidName( PFLT_VOLUME Volume, CTX_INSTANCE_CONTEXT* InstanceContext, UNICODE_STRING* VolumeGuidName )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING sourceGuidName;

    do
    {
        if( InstanceContext == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        //  sourceGuidName is the source from where we'll copy the volume
        //  GUID name. Hopefully the name is present in the instance context
        //  already (buffer is not NULL) so we'll try to use that.
        //

        sourceGuidName = &InstanceContext->VolumeGUIDName;

        if( sourceGuidName->Buffer == NULLPTR )
        {

            //
            //  The volume GUID name is not cached in the instance context
            //  yet, so we will have to query the volume for it and put it
            //  in the instance context, so future queries can get it directly
            //  from the context.
            //

            UNICODE_STRING tempString;

            //
            //  Add sizeof(WCHAR) so it's possible to add a trailing backslash here.
            //

            tempString.MaximumLength = VOLUME_GUID_NAME_SIZE * sizeof( WCHAR ) + sizeof( WCHAR );

            Status = SolAllocateUnicodeString( &tempString );

            if( !NT_SUCCESS( Status ) )
                break;

            //  while there is no guid name, don't do the open by id deletion logic.
            //  (it's actually better to defer obtaining the volume GUID name up to
            //   the point when we actually need it, in the open by ID scenario.)
            Status = FltGetVolumeGuidName( Volume,
                                           &tempString,
                                           NULL );

            if( !NT_SUCCESS( Status ) )
            {
                SolFreeUnicodeString( &tempString );

                return Status;
            }

            //
            //  Append trailing backslash.
            //

            RtlAppendUnicodeToString( &tempString, L"\\" );

            //
            //  Now set the sourceGuidName to the tempString. It is okay to
            //  set Length and MaximumLength with no synchronization because
            //  those will always be the same value (size of a volume GUID
            //  name with an extra trailing backslash).
            //

            sourceGuidName->Length = tempString.Length;
            sourceGuidName->MaximumLength = tempString.MaximumLength;

            //
            //  Setting the buffer, however, requires some synchronization,
            //  because another thread might be attempting to do the same,
            //  and even though they're exactly the same string, they're
            //  different allocations (buffers) so if the other thread we're
            //  racing with manages to set the buffer before us, we need to
            //  free our temporary string buffer.
            //

            InterlockedCompareExchangePointer( (PVOID*)&sourceGuidName->Buffer,
                                               tempString.Buffer,
                                               NULL );

            if( sourceGuidName->Buffer != tempString.Buffer )
            {

                //
                //  We didn't manage to set the buffer, so let's free the
                //  tempString buffer.
                //

                SolFreeUnicodeString( &tempString );
            }
        }

        //
        //  sourceGuidName now contains the correct GUID name, so copy that
        //  to the caller string.
        //

        RtlCopyUnicodeString( VolumeGuidName, sourceGuidName );

    } while( false );

    return Status;
}

NTSTATUS SolAllocateUnicodeString( UNICODE_STRING* String )
{
    ASSERT( NULL != String );
    ASSERT( 0 != String->MaximumLength );

    String->Length = 0;

    String->Buffer = ( PWCH )ExAllocatePoolWithTag( NonPagedPool,
                                                    String->MaximumLength,
                                                    POOL_MAIN_TAG );

    if( NULL == String->Buffer )
    {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

void SolFreeUnicodeString( UNICODE_STRING* String )
{
    ASSERT( NULL != String );
    ASSERT( 0 != String->MaximumLength );

    String->Length = 0;

    if( String->Buffer != NULL )
    {
        String->MaximumLength = 0;
        ExFreePool( String->Buffer );
        String->Buffer = NULL;
    }
}

NTSTATUS SolAddTransDeleteNotify( IRP_CONTEXT* IrpContext, CTX_TRANSACTION_CONTEXT* TransactionContext, BOOLEAN FileDelete )
{

    ASSERT( NULL != IrpContext );

    ASSERT( NULL != TransactionContext->Resource );

    ASSERT( NULL != IrpContext->StreamContext );

    auto deleteNotify = ( PDF_DELETE_NOTIFY )ExAllocatePoolWithTag( NonPagedPool,
                                                                    sizeof( DF_DELETE_NOTIFY ),
                                                                    POOL_MAIN_TAG );

    if( NULL == deleteNotify )
    {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( deleteNotify, sizeof( DF_DELETE_NOTIFY ) );

    FltReferenceContext( IrpContext->StreamContext );
    deleteNotify->StreamContext = IrpContext->StreamContext;
    deleteNotify->FileDelete = FileDelete;

    FltAcquireResourceExclusive( TransactionContext->Resource );

    InsertTailList( &TransactionContext->DeleteNotifyList, &deleteNotify->Links );

    FltReleaseResource( TransactionContext->Resource );

    return STATUS_SUCCESS;
}
