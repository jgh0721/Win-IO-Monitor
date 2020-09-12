#include "callbacks/callbacks.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreOperationPassThrough( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                                PVOID* CompletionContext )
{
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostOperationPassThrough( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                                  PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    return FLT_POSTOP_FINISHED_PROCESSING;
}

#define DF_NOTIFICATION_MASK            (TRANSACTION_NOTIFY_COMMIT_FINALIZE | \
                                         TRANSACTION_NOTIFY_ROLLBACK)

NTSTATUS FLTAPI FilterTransactionNotificationCallback( PCFLT_RELATED_OBJECTS FltObjects,
                                                       PCTX_TRANSACTION_CONTEXT TransactionContext, ULONG NotificationMask )
{
    BOOLEAN commit = BooleanFlagOn( NotificationMask, TRANSACTION_NOTIFY_COMMIT_FINALIZE );
    PDF_DELETE_NOTIFY deleteNotify = NULL;

    UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    //
    //  There is no such thing as a simultaneous commit and rollback, nor
    //  should we get notifications for events other than a commit or a
    //  rollback.
    //

    ASSERT( ( !FlagOnAll( NotificationMask, ( DF_NOTIFICATION_MASK ) ) ) &&
            FlagOn( NotificationMask, ( DF_NOTIFICATION_MASK ) ) );

    KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] %s %s %s\n",
                 __FUNCTION__, "TransactionNotification : ", commit != FALSE ? "COMMIT" : "ROLLBACK" ) );

    ASSERT( NULL != TransactionContext->Resource );

    FltAcquireResourceExclusive( TransactionContext->Resource );

    while( !IsListEmpty( &TransactionContext->DeleteNotifyList ) )
    {

        deleteNotify = CONTAINING_RECORD( RemoveHeadList( &TransactionContext->DeleteNotifyList ),
                                          DF_DELETE_NOTIFY,
                                          Links );

        ASSERT( NULL != deleteNotify->StreamContext );

        if( !commit )
        {
            InterlockedDecrement( &deleteNotify->StreamContext->IsNotified );
        }

        DfNotifyDeleteOnTransactionEnd( deleteNotify,
                                        commit );

        // release stream context
        FltReleaseContext( deleteNotify->StreamContext );
        ExFreePool( deleteNotify );
    }

    FltReleaseResource( TransactionContext->Resource );

    return STATUS_SUCCESS;
}

void DfNotifyDeleteOnTransactionEnd( PDF_DELETE_NOTIFY DeleteNotify, BOOLEAN Commit )
{
    PAGED_CODE();

    if( DeleteNotify->FileDelete != FALSE )
    {
        if( Commit != FALSE )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, "[WinIOMon] %s %s : %ws(%p)\n" 
                         __FUNCTION__, "File Deleted In Transaction Commit"
                         , DeleteNotify->StreamContext->FileFullPath.Buffer, DeleteNotify->StreamContext 
                         ) );
        }
        else
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, "[WinIOMon] %s %s : %ws(%p)\n"
                         __FUNCTION__, "File Delete Canceled By Transaction Rollback"
                         , DeleteNotify->StreamContext->FileFullPath.Buffer, DeleteNotify->StreamContext
                         ) );
        }
    }
    else
    {
        // alternate data stream
        if( Commit != FALSE )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, "[WinIOMon] %s %s : %ws(%p)\n"
                         __FUNCTION__, "ADS Deleted In Transaction Commit"
                         , DeleteNotify->StreamContext->FileFullPath.Buffer, DeleteNotify->StreamContext
                         ) );
        }
        else
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, "[WinIOMon] %s %s : %ws(%p)\n"
                         __FUNCTION__, "ADS Delete Canceled By Transaction Rollback"
                         , DeleteNotify->StreamContext->FileFullPath.Buffer, DeleteNotify->StreamContext
                         ) );
        }
    }
}
