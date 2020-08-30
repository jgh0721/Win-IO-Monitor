#ifndef HDR_MINIFILTER_CLEANUP
#define HDR_MINIFILTER_CLEANUP

#include "fltBase.hpp"
#include "utilities/contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS
FLTAPI WinIOPreCleanup( __inout PFLT_CALLBACK_DATA Data,
                      __in PCFLT_RELATED_OBJECTS FltObjects,
                      __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS
FLTAPI WinIOPostCleanup( __inout PFLT_CALLBACK_DATA Data,
                       __in PCFLT_RELATED_OBJECTS FltObjects,
                       __in_opt PVOID CompletionContext,
                       __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END


NTSTATUS DfProcessDelete( __inout PFLT_CALLBACK_DATA Data,
                          __in PCFLT_RELATED_OBJECTS FltObjects,
                          __in PCTX_STREAM_CONTEXT StreamContext );

VOID DfNotifyDelete( __in PCTX_STREAM_CONTEXT StreamContext,
                     __in BOOLEAN IsFile,
                     __inout_opt PCTX_TRANSACTION_CONTEXT TransactionContext );

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
NTSTATUS DfAddTransDeleteNotify( __inout PCTX_STREAM_CONTEXT StreamContext,
                                 __inout PCTX_TRANSACTION_CONTEXT TransactionContext,
                                 __in BOOLEAN FileDelete );

#endif // HDR_MINIFILTER_CLEANUP