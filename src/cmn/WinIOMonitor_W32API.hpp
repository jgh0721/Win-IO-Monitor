#ifndef HDR_WINIOMONITOR_W32API
#define HDR_WINIOMONITOR_W32API

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsW32API
{
    typedef NTSTATUS( FLTAPI* PFLT_TRANSACTION_NOTIFICATION_CALLBACK )
        ( __in PCFLT_RELATED_OBJECTS FltObjects,
          __in PFLT_CONTEXT TransactionContext,
          __in ULONG NotificationMask );

    typedef struct _FLT_REGISTRATION_XP
    {
        //
        //  The size, in bytes, of this registration structure.
        //

        USHORT Size;
        USHORT Version;

        //
        //  Flag values
        //

        FLT_REGISTRATION_FLAGS Flags;

        //
        //  Variable length array of routines that are used to manage contexts in
        //  the system.
        //

        CONST FLT_CONTEXT_REGISTRATION* ContextRegistration;

        //
        //  Variable length array of routines used for processing pre- and post-
        //  file system operations.
        //

        CONST FLT_OPERATION_REGISTRATION* OperationRegistration;

        //
        //  This is called before a filter is unloaded.  If an ERROR or WARNING
        //  status is returned then the filter is NOT unloaded.  A mandatory unload
        //  can not be failed.
        //
        //  If a NULL is specified for this routine, then the filter can never be
        //  unloaded.
        //

        PFLT_FILTER_UNLOAD_CALLBACK FilterUnloadCallback;

        //
        //  This is called to see if a filter would like to attach an instance
        //  to the given volume.  If an ERROR or WARNING status is returned, an
        //  attachment is not made.
        //
        //  If a NULL is specified for this routine, the attachment is always made.
        //

        PFLT_INSTANCE_SETUP_CALLBACK InstanceSetupCallback;

        //
        //  This is called to see if the filter wants to detach from the given
        //  volume.  This is only called for manual detach requests.  If an
        //  ERROR or WARNING status is returned, the filter is not detached.
        //
        //  If a NULL is specified for this routine, then instances can never be
        //  manually detached.
        //

        PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK InstanceQueryTeardownCallback;

        //
        //  This is called at the start of a filter detaching from a volume.
        //
        //  It is OK for this field to be NULL.
        //

        PFLT_INSTANCE_TEARDOWN_CALLBACK InstanceTeardownStartCallback;

        //
        //  This is called at the end of a filter detaching from a volume.  All
        //  outstanding operations have been completed by the time this routine
        //  is called.
        //
        //  It is OK for this field to be NULL.
        //

        PFLT_INSTANCE_TEARDOWN_CALLBACK InstanceTeardownCompleteCallback;

        //
        //  The following callbacks are provided by a filter only if it is
        //  interested in modifying the name space.
        //
        //  If NULL is specified for these callbacks, it is assumed that the
        //  filter would not affect the name being requested.
        //

        PFLT_GENERATE_FILE_NAME GenerateFileNameCallback;

        PFLT_NORMALIZE_NAME_COMPONENT NormalizeNameComponentCallback;

        PFLT_NORMALIZE_CONTEXT_CLEANUP NormalizeContextCleanupCallback;

        //
        //  The PFLT_NORMALIZE_NAME_COMPONENT_EX callback is also a name
        //  provider callback. It is not included here along with the
        //  other name provider callbacks to take care of the registration
        //  structure versioning issues.
        //

    } FLT_REGISTRATION_XP, * PFLT_REGISTRATION_XP;

    typedef struct _FLT_REGISTRATION_VISTA
    {
        //
        //  The size, in bytes, of this registration structure.
        //

        USHORT Size;
        USHORT Version;

        //
        //  Flag values
        //

        FLT_REGISTRATION_FLAGS Flags;

        //
        //  Variable length array of routines that are used to manage contexts in
        //  the system.
        //

        CONST FLT_CONTEXT_REGISTRATION* ContextRegistration;

        //
        //  Variable length array of routines used for processing pre- and post-
        //  file system operations.
        //

        CONST FLT_OPERATION_REGISTRATION* OperationRegistration;

        //
        //  This is called before a filter is unloaded.  If an ERROR or WARNING
        //  status is returned then the filter is NOT unloaded.  A mandatory unload
        //  can not be failed.
        //
        //  If a NULL is specified for this routine, then the filter can never be
        //  unloaded.
        //

        PFLT_FILTER_UNLOAD_CALLBACK FilterUnloadCallback;

        //
        //  This is called to see if a filter would like to attach an instance
        //  to the given volume.  If an ERROR or WARNING status is returned, an
        //  attachment is not made.
        //
        //  If a NULL is specified for this routine, the attachment is always made.
        //

        PFLT_INSTANCE_SETUP_CALLBACK InstanceSetupCallback;

        //
        //  This is called to see if the filter wants to detach from the given
        //  volume.  This is only called for manual detach requests.  If an
        //  ERROR or WARNING status is returned, the filter is not detached.
        //
        //  If a NULL is specified for this routine, then instances can never be
        //  manually detached.
        //

        PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK InstanceQueryTeardownCallback;

        //
        //  This is called at the start of a filter detaching from a volume.
        //
        //  It is OK for this field to be NULL.
        //

        PFLT_INSTANCE_TEARDOWN_CALLBACK InstanceTeardownStartCallback;

        //
        //  This is called at the end of a filter detaching from a volume.  All
        //  outstanding operations have been completed by the time this routine
        //  is called.
        //
        //  It is OK for this field to be NULL.
        //

        PFLT_INSTANCE_TEARDOWN_CALLBACK InstanceTeardownCompleteCallback;

        //
        //  The following callbacks are provided by a filter only if it is
        //  interested in modifying the name space.
        //
        //  If NULL is specified for these callbacks, it is assumed that the
        //  filter would not affect the name being requested.
        //

        PFLT_GENERATE_FILE_NAME GenerateFileNameCallback;

        PFLT_NORMALIZE_NAME_COMPONENT NormalizeNameComponentCallback;

        PFLT_NORMALIZE_CONTEXT_CLEANUP NormalizeContextCleanupCallback;

        //
        //  The PFLT_NORMALIZE_NAME_COMPONENT_EX callback is also a name
        //  provider callback. It is not included here along with the
        //  other name provider callbacks to take care of the registration
        //  structure versioning issues.
        //

        //
        //  This is called for transaction notifications received from the KTM
        //  when a filter has enlisted on that transaction.
        //

        PFLT_TRANSACTION_NOTIFICATION_CALLBACK TransactionNotificationCallback;

        //
        //  This is the extended normalize name component callback
        //  If a mini-filter provides this callback, then  this callback
        //  will be used as opposed to using PFLT_NORMALIZE_NAME_COMPONENT
        //
        //  The PFLT_NORMALIZE_NAME_COMPONENT_EX provides an extra parameter
        //  (PFILE_OBJECT) in addition to the parameters provided to
        //  PFLT_NORMALIZE_NAME_COMPONENT. A mini-filter may use this parameter
        //  to get to additional information like the TXN_PARAMETER_BLOCK.
        //
        //  A mini-filter that has no use for the additional parameter may
        //  only provide a PFLT_NORMALIZE_NAME_COMPONENT callback.
        //
        //  A mini-filter may provide both a PFLT_NORMALIZE_NAME_COMPONENT
        //  callback and a PFLT_NORMALIZE_NAME_COMPONENT_EX callback. The
        //  PFLT_NORMALIZE_NAME_COMPONENT_EX callback will be used by fltmgr
        //  versions that understand this callback (Vista RTM and beyond)
        //  and PFLT_NORMALIZE_NAME_COMPONENT callback will be used by fltmgr
        //  versions that do not understand the PFLT_NORMALIZE_NAME_COMPONENT_EX
        //  callback (prior to Vista RTM). This allows the same mini-filter
        //  binary to run with all versions of fltmgr.
        //

        PFLT_NORMALIZE_NAME_COMPONENT_EX NormalizeNameComponentExCallback;

    } FLT_REGISTRATION_VISTA, * PFLT_REGISTRATION_VISTA;

    typedef struct _NtOsKrnlAPI
    {
        // The IoGetTransactionParameterBlock routine returns the transaction parameter block for a transacted file operation.
        typedef PTXN_PARAMETER_BLOCK( NTAPI* IoGetTransactionParameterBlock )( __in  PFILE_OBJECT FileObject );

        IoGetTransactionParameterBlock pfnIoGetTransactionParameterBlock;

    } NtOsKrnlAPI;

    typedef struct _FltMgrAPI
    {
        typedef NTSTATUS( NTAPI* FltGetFileContext )( __in PFLT_INSTANCE Instance,
                                                      __in PFILE_OBJECT FileObject,
                                                      __out PFLT_CONTEXT* Context );

        typedef NTSTATUS( NTAPI* FltSetFileContext )( __in PFLT_INSTANCE Instance,
                                                      __in PFILE_OBJECT FileObject,
                                                      __in FLT_SET_CONTEXT_OPERATION Operation,
                                                      __in PFLT_CONTEXT NewContext,
                                                      __out PFLT_CONTEXT* OldContext );

        FltGetFileContext                   pfnFltGetFileContext;
        FltSetFileContext                   pfnFltSetFileContext;

    } FltMgrAPI;

    void InitializeFltMgrAPI( __in FltMgrAPI* fltMgrAPI );

    extern nsW32API::NtOsKrnlAPI NtOsKrnlAPIMgr;
    extern nsW32API::FltMgrAPI FltMgrAPIMgr;

} // nsW32API

#endif // HDR_WINIOMONITOR_W32API