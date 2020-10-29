#ifndef HDR_ISOLATION_DETECT_DELETE
#define HDR_ISOLATION_DETECT_DELETE

#include "fltBase.hpp"

#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*++

Routine Description:

    This routine does the processing after it is verified, in the post-cleanup
    callback, that a file or stream were deleted. It sorts out whether it's a
    file or a stream delete, whether this is in a transacted context or not,
    and issues the appropriate notifications.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - Pointer to the stream context of the deleted file/stream.

Return Value:

    STATUS_SUCCESS.

--*/
NTSTATUS SolProcessDelete( __in IRP_CONTEXT* IrpContext );

/*++

Routine Description:

    This routine does the processing after it is verified, in the post-cleanup
    callback, that a file or stream were deleted. It sorts out whether it's a
    file or a stream delete, whether this is in a transacted context or not,
    and issues the appropriate notifications.

Arguments:

    StreamContext - Pointer to the stream context of the deleted file/stream.

    IsFile - TRUE if deleting a file, FALSE for an alternate data stream.

    TransactionContext - The transaction context. Present if in a transaction,
                         NULL otherwise.

--*/
VOID SolNotifyDelete( __in IRP_CONTEXT* IrpContext, __in BOOLEAN IsFile, __inout_opt CTX_TRANSACTION_CONTEXT* TransactionContext );

/*++

Routine Description:

    This routine returns whether a file was deleted. It is called from
    DfProcessDelete after an alternate data stream is deleted. This needs to
    be done for the case when the last outstanding handle to a delete-pending
    file is a handle to a delete-pending alternate data stream. When that
    handle is closed, the whole file goes away, and we want to report a whole
    file deletion, not just an alternate data stream deletion.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - Pointer to the stream context.

    IsTransaction - TRUE if in a transaction, FALSE otherwise.

Return Value:

    STATUS_FILE_DELETED - The whole file was deleted.
    Successful status - The file still exists, this was probably just a named
                        data stream being deleted.
    Anything else - Failure in finding out if the file was deleted.

--*/
NTSTATUS SolIfFileDeleted( __in IRP_CONTEXT* IrpContext, __in bool IsTransaction );

/*++

Routine Description:

    This helper routine detects a deleted file by attempting to open it using
    its file ID.

    If the file is successfully opened this routine closes the file before returning.

Arguments:

    Data - Pointer to FLT_CALLBACK_DATA.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - Pointer to the stream context.

Return Value:

    STATUS_FILE_DELETED - Returned through DfBuildFileIdString if the file has
                          been deleted.

    STATUS_INVALID_PARAMETER - Returned from FltCreateFileEx2 when opening by ID
                               a file that doesn't exist.

    STATUS_DELETE_PENDING - The file has been set to be deleted when the last handle
                            goes away, but there are still open handles.

    Also any other NTSTATUS returned from DfBuildFileIdString, FltCreateFileEx2,
    or FltClose.

--*/
NTSTATUS SolDetectDeleteByFileId( __in IRP_CONTEXT* IrpContext );

/*++

Routine Description:

    This helper routine builds a string used to open a file by its ID.

    It will assume the file ID is properly loaded in the stream context
    (StreamContext->FileId).

Arguments:

    Data - Pointer to FLT_CALLBACK_DATA.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - Pointer to the stream context.

    String - Pointer to UNICODE_STRING (output).

Return Value:

    Return statuses forwarded by DfAllocateUnicodeString or
    FltGetInstanceContext.

--*/
NTSTATUS SolBuildFileIdString( __in IRP_CONTEXT* IrpContext, __out UNICODE_STRING* String );

/*++

Routine Description:

    This routine obtains the File ID and saves it in the stream context.

Arguments:

    Data  - Pointer to FLT_CALLBACK_DATA.

    StreamContext - Pointer to stream context that will receive the file
                    ID.

Return Value:

    Returns statuses forwarded from FltQueryInformationFile, including
    STATUS_FILE_DELETED.

--*/
NTSTATUS SolGetFileId( __in PFLT_INSTANCE Instance, __in PFILE_OBJECT FileObject, __inout CTX_STREAM_CONTEXT* StreamContext );

/*++

Routine Description:

    This helper routine returns a volume GUID name (with an added trailing
    backslash for convenience) in the VolumeGuidName string passed by the
    caller.

    The volume GUID name is cached in the instance context for the instance
    attached to the volume, and this function will set up an instance context
    with the cached name on it if there isn't one already attached to the
    instance.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    VolumeGuidName - Pointer to UNICODE_STRING, returning the volume GUID name.

Return Value:

    Return statuses forwarded by DfAllocateUnicodeString or
    FltGetVolumeGuidName. On error, caller needs to DfFreeUnicodeString on
    VolumeGuidName.

--*/
NTSTATUS SolGetVolumeGuidName( __in PFLT_VOLUME Volume, __in CTX_INSTANCE_CONTEXT* InstanceContext, __inout UNICODE_STRING* VolumeGuidName );

/*++

Routine Description:

    This helper routine simply allocates a buffer for a UNICODE_STRING and
    initializes its Length to zero.

    It uses whatever value is present in the MaximumLength field as the size
    for the allocation.

Arguments:

    String - Pointer to UNICODE_STRING.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES if it was not possible to allocate the
    buffer from pool.

    STATUS_SUCCESS otherwise.

--*/
NTSTATUS SolAllocateUnicodeString( __inout UNICODE_STRING* String );

/*++

Routine Description:

    This helper routine frees the buffer of a UNICODE_STRING and resets its
    Length to zero.

Arguments:

    String - Pointer to UNICODE_STRING.

--*/
VOID SolFreeUnicodeString( __inout UNICODE_STRING* String );

/*++

Routine Description:

    This routine adds a pending deletion notification (DF_DELETE_NOTIFY)
    object to the transaction context DeleteNotifyList. It is called from
    DfNotifyDelete when a file or stream gets deleted in a transaction.

Arguments:

    StreamContext - Pointer to the stream context.

    TransactionContext - Pointer to the transaction context.

    FileDelete - TRUE if this is a FILE deletion, FALSE if it's a STREAM
                 deletion.

Return Value:

    STATUS_SUCCESS.

--*/
NTSTATUS SolAddTransDeleteNotify( __inout IRP_CONTEXT* IrpContext, __inout CTX_TRANSACTION_CONTEXT* TransactionContext, __in BOOLEAN FileDelete );

/*++

Routine Description:

    This routine is called by the transaction notification callback to issue
    the proper notifications for a file that has been deleted in the context
    of that transaction.
    The file will be reported as finally deleted, if the transaction was
    committed, or "saved" if the transaction was rolled back.

Arguments:

    DeleteNotify - Pointer to the DF_DELETE_NOTIFY object that contains the
                   data necessary for issuing this notification.

    Commit       - TRUE if the transaction was committed, FALSE if it was
                   rolled back.

--*/
VOID SolNotifyDeleteOnTransactionEnd( __in PDF_DELETE_NOTIFY DeleteNotify, __in BOOLEAN Commit );

#endif // HDR_ISOLATION_DETECT_DELETE