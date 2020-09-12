#ifndef HDR_CALLBACKS
#define HDR_CALLBACKS

#include "fltBase.hpp"

#include "utilities/contextMgr_Defs.hpp"

#include "fltInstance.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreOperationPassThrough( __inout PFLT_CALLBACK_DATA Data,
                               __in PCFLT_RELATED_OBJECTS FltObjects,
                               __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostOperationPassThrough( __inout PFLT_CALLBACK_DATA Data,
                                __in PCFLT_RELATED_OBJECTS FltObjects,
                                __in_opt PVOID CompletionContext,
                                __in FLT_POST_OPERATION_FLAGS Flags );

/*++

Routine Description:

    This routine is the transaction notification callback for this minifilter.
    It is called when a transaction we're enlisted in is committed or rolled
    back so that it's possible to emit notifications about files that were
    deleted in that transaction.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    TransactionContext - The transaction context, set/modified when a delete
        is detected.

    NotificationMask - A mask of flags indicating the notifications received
        from FltMgr. Should be either TRANSACTION_NOTIFY_COMMIT or
        TRANSACTION_NOTIFY_ROLLBACK.

Return Value:

    STATUS_SUCCESS - This operation is never pended.

--*/
NTSTATUS FLTAPI
FilterTransactionNotificationCallback( __in PCFLT_RELATED_OBJECTS FltObjects,
                                       __in PCTX_TRANSACTION_CONTEXT TransactionContext,
                                       __in ULONG NotificationMask );

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
VOID DfNotifyDeleteOnTransactionEnd( __in PDF_DELETE_NOTIFY DeleteNotify,
                                     __in BOOLEAN Commit );

EXTERN_C_END

/*!
 * Paging IO Support IRP : IRP_MJ_READ, IRP_MJ_WRITE, IRP_MJ_QUERY_INFORMATION, IRP_MJ_SET_INFORMATION
 *
 *
 * IRP_MJ_SET_INFORMATION
 *  Paging IO : Change File Size
 *
 *  https://community.osr.com/discussion/238116/fltfl-operation-registration-skip-paging-io-why-is-this-flag-used-here-minifilter
 *  https://community.osr.com/discussion/290835/minifilter-not-able-to-block-paging-io-for-very-small-files
 *  http://bugsfixed.blogspot.com/2017/02/fltfloperationregistrationskippagingio.html
 */
#endif // HDR_CALLBACKS