#ifndef HDR_UTIL_CONTEXTMGR
#define HDR_UTIL_CONTEXTMGR

#include "fltBase.hpp"
#include "contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*!
 * ContextMgr from Microsoft Delete Sample 
 */

 /*++

 Routine Description:

     This routine allocates and initializes a context of given type.

 Arguments:

     ContextType   - Type of context to be allocated/initialized.

     Context       - Pointer to a context pointer.

 Return Value:

     Returns a status forwarded from FltAllocateContext.

 --*/
NTSTATUS CtxAllocateContext( __in PFLT_FILTER Filter, __in FLT_CONTEXT_TYPE ContextType, __out PFLT_CONTEXT* Context );

/*++

Routine Description:

    This routine sets the given context to the target.

Arguments:

    FltObjects    - Pointer to the FLT_RELATED_OBJECTS data structure containing
                    opaque handles to this filter, instance and its associated volume.

    Target        - Pointer to the target to which we want to attach the
                    context. It will actually be either a FILE_OBJECT or
                    a KTRANSACTION. For instance contexts, it's ignored, as
                    the target is the FLT_INSTANCE itself, obtained from
                    Data->Iopb->TargetInstance.

    ContextType   - Type of context to get/allocate/attach. Also used to
                    disambiguate the target/context type as this minifilter
                    only has one type of context per target.

    NewContext    - Pointer to the context the caller wants to attach.

    OldContext    - Returns the context already attached to the target, if
                    that is the case.

Return Value:

    Returns a status forwarded from FltSetXxxContext.

--*/
NTSTATUS CtxSetContext( __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID Target, __in FLT_CONTEXT_TYPE ContextType, __in PFLT_CONTEXT NewContext, __out_opt PFLT_CONTEXT* OldContext );
/*++

Routine Description:

    This routine gets the given context from the target.

Arguments:

    FltObjects    - Pointer to the FLT_RELATED_OBJECTS data structure containing
                    opaque handles to this filter, instance and its associated volume.

    Target        - Pointer to the target from which we want to obtain the
                    context. It will actually be either a FILE_OBJECT or
                    a KTRANSACTION. For instance contexts, it's ignored, as
                    the target is the FLT_INSTANCE itself, obtained from
                    Data->Iopb->TargetInstance.

    ContextType   - Type of context to get. Also used to disambiguate
                    the target/context type as this minifilter
                    only has one type of context per target.

    Context       - Pointer returning a pointer to the attached context.

Return Value:

    Returns a status forwarded from FltSetXxxContext.

--*/
NTSTATUS CtxGetContext( __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID Target, __in FLT_CONTEXT_TYPE ContextType, __out PFLT_CONTEXT* Context );
/*++

Routine Description:

    This routine obtains a context of type ContextType that is attached to
    Target.

    If a context is already attached to Target, it will be returned in
    *Context. If a context is already attached, but *Context points to
    another context, *Context will be released.

    If no context is attached, and *Context points to a previously allocated
    context, *Context will be attached to the Target.

    Finally, if no previously allocated context is passed to this routine
    (*Context is a NULL pointer), a new Context is created and then attached
    to Target.

    In case of race conditions (or the presence of a previously allocated
    context at *Context), the existing attached context is returned via
    *Context.

    In case of a transaction context, this function will also enlist in the
    transaction.

Arguments:

    FltObjects    - Pointer to the FLT_RELATED_OBJECTS data structure containing
                    opaque handles to this filter, instance and its associated volume.

    Target        - Pointer to the target to which we want to attach the
                    context. It will actually be either a FILE_OBJECT or
                    a KTRANSACTION.  It is NULL for an Instance context.

    Context       - Pointer to a pointer to a context. Used both for
                    returning an allocated/attached context or for receiving
                    a context to attach to the Target.

    ContextType   - Type of context to get/allocate/attach. Also used to
                    disambiguate the target/context type as this minifilter
                    only has one type of context per target.

Return Value:

    Returns a status forwarded from Flt(((Get|Set)Xxx)|Allocate)Context or
    FltEnlistInTransaction.

--*/
NTSTATUS CtxGetOrSetContext( __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID Target, __out PFLT_CONTEXT* Context, __in FLT_CONTEXT_TYPE ContextType );

NTSTATUS CtxReleaseContext( __in PFLT_CONTEXT Context );

///////////////////////////////////////////////////////////////////////////////

VOID FLTAPI CtxInstanceContextCleanupCallback( __in PCTX_INSTANCE_CONTEXT InstanceContext, __in FLT_CONTEXT_TYPE ContextType );
VOID FLTAPI CtxVolumeContextCleanupCallback( __in PCTX_VOLUME_CONTEXT VolumeContext, __in FLT_CONTEXT_TYPE ContextType );
VOID FLTAPI CtxFileContextCleanupCallback( __in PCTX_FILE_CONTEXT FileContext, __in FLT_CONTEXT_TYPE ContextType );
VOID FLTAPI CtxStreamContextCleanupCallback( __in PCTX_STREAM_CONTEXT StreamContext, __in FLT_CONTEXT_TYPE ContextType );
VOID FLTAPI CtxStreamHandleContextCleanupCallback( __in PCTX_STREAMHANDLE_CONTEXT StreamHandleContext, __in FLT_CONTEXT_TYPE ContextType );
VOID FLTAPI CtxTransactionContextCleanupCallback( __in PCTX_TRANSACTION_CONTEXT TransactionContext, __in FLT_CONTEXT_TYPE ContextType );

#endif // HDR_UTIL_CONTEXTMGR