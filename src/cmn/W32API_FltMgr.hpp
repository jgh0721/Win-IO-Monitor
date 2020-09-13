#ifndef HDR_W32API_FILTER
#define HDR_W32API_FILTER

#include "fltBase.hpp"
#include "W32API_Base.hpp"
#include "W32API_LibMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsW32API
{
    // Win8 ~ 
    #define FLT_SECTION_CONTEXT 0x0040
    // Win8 ~
    #define FLT_REGISTRATION_VERSION_0203  0x0203
    //
    //  If set, this filter is aware of named pipe and mailslot filtering
    //  and would like filter manager to give it the option of filtering
    //  these file systems.
    //

    #define FLTFL_REGISTRATION_SUPPORT_NPFS_MSFS            0x00000002

    //
    //  If set, this filter is aware of DAX volumes i.e. volumes that support
    //  mapping a file directly on the persistent memory device.  For such
    //  volumes, cached and memory mapped IO to user files wouldn't generate
    //  paging IO.
    //

    #define FLTFL_REGISTRATION_SUPPORT_DAX_VOLUME           0x00000004

    typedef enum _FLT_FILESYSTEM_TYPE
    {
        FLT_FSTYPE_UNKNOWN,
        FLT_FSTYPE_RAW,
        FLT_FSTYPE_NTFS,
        FLT_FSTYPE_FAT,
        FLT_FSTYPE_CDFS,
        FLT_FSTYPE_UDFS,
        FLT_FSTYPE_LANMAN,
        FLT_FSTYPE_WEBDAV,
        FLT_FSTYPE_RDPDR,
        FLT_FSTYPE_NFS,
        FLT_FSTYPE_MS_NETWARE,
        FLT_FSTYPE_NETWARE,
        FLT_FSTYPE_BSUDF,
        FLT_FSTYPE_MUP,
        FLT_FSTYPE_RSFX,
        FLT_FSTYPE_ROXIO_UDF1,
        FLT_FSTYPE_ROXIO_UDF2,
        FLT_FSTYPE_ROXIO_UDF3,
        FLT_FSTYPE_TACIT,
        FLT_FSTYPE_FS_REC,
        FLT_FSTYPE_INCD,
        FLT_FSTYPE_INCD_FAT,
        FLT_FSTYPE_EXFAT,
        FLT_FSTYPE_PSFS,
        FLT_FSTYPE_GPFS,
        FLT_FSTYPE_NPFS,
        FLT_FSTYPE_MSFS,
        FLT_FSTYPE_CSVFS,
        FLT_FSTYPE_REFS,
        FLT_FSTYPE_OPENAFS,
        FLT_FSTYPE_CIMFS
    } FLT_FILESYSTEM_TYPE, * PFLT_FILESYSTEM_TYPE;

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

    typedef NTSTATUS( FLTAPI* PFLT_TRANSACTION_NOTIFICATION_CALLBACK )
        ( __in PCFLT_RELATED_OBJECTS FltObjects,
          __in PFLT_CONTEXT TransactionContext,
          __in ULONG NotificationMask );

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

    typedef NTSTATUS
    ( FLTAPI* PFLT_SECTION_CONFLICT_NOTIFICATION_CALLBACK ) (
        __in PFLT_INSTANCE Instance,
        __in PFLT_CONTEXT SectionContext,
        __in PFLT_CALLBACK_DATA Data
        );

    typedef struct _FLT_REGISTRATION_WIN8
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

        //
        //  This is called for IO failures due to the existence of sections
        //  when those sections are created through FltCreateSectionForDatascan.
        //

        PFLT_SECTION_CONFLICT_NOTIFICATION_CALLBACK SectionNotificationCallback;

    } FLT_REGISTRATION_WIN8, * PFLT_REGISTRATION_WIN8;
    
    ///////////////////////////////////////////////////////////////////////////////
    //
    // Flags that can be specified in Flt* APIs to indicate the nature of the
    // i/o operation
    //
    // FltReadFile/FltWriteFile will accept these flags for example
    //
    ///////////////////////////////////////////////////////////////////////////////

    //
    //  If set, the given read/write request will have the
    //  IRP_SYNCHRONOUS_PAGING_IO flag set
    //

    #define FLTFL_IO_OPERATION_SYNCHRONOUS_PAGING           0x00000008

    DYNLOAD_BEGIN_CLASS( CFltMgrAPI, L"FltMgr.exe" )

        /**
         * @brief
         *
         * Windows XP SP3 ~
        */
        DYNLOAD_FUNC_WITH_15( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, FLTAPI, FltCreateFileEx, __in PFLT_FILTER, Filter, __in_opt PFLT_INSTANCE, Instance, __out PHANDLE, FileHandle, __deref_opt_out PFILE_OBJECT*, FileObject, __in ACCESS_MASK, DesiredAccess, __in POBJECT_ATTRIBUTES, ObjectAttributes, __out PIO_STATUS_BLOCK, IoStatusBlock, __in_opt PLARGE_INTEGER, AllocationSize, __in ULONG, FileAttributes, __in ULONG, ShareAccess, __in ULONG, CreateDisposition, __in ULONG, CreateOptions, __in_bcount_opt( EaLength ) PVOID, EaBuffer, __in ULONG, EaLength, __in ULONG, Flags );

        /**
         * @brief Minifilter drivers call FltCreateFileEx2 to create a new file or open an existing file. This routine also includes an optional create context parameter.
         *
         * Available in starting with Windows Vista
        */
        DYNLOAD_FUNC_WITH_16( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, NTAPI, FltCreateFileEx2, __in PFLT_FILTER, Filter, __in_opt PFLT_INSTANCE, Instance, __out PHANDLE, FileHandle, __out_opt  PFILE_OBJECT*, FileObject, __in ACCESS_MASK, DesiredAccess, __in POBJECT_ATTRIBUTES, ObjectAttributes, __out PIO_STATUS_BLOCK, IoStatusBlock, __in_opt   PLARGE_INTEGER, AllocationSize, __in ULONG, FileAttributes, __in ULONG, ShareAccess, __in ULONG, CreateDisposition, __in ULONG, CreateOptions, __in_opt PVOID, EaBuffer, __in ULONG, EaLength, __in ULONG, Flags, __in_opt PIO_DRIVER_CREATE_CONTEXT, DriverContext );

        DYNLOAD_FUNC_WITH_03( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, NTAPI, FltGetFileContext, __in PFLT_INSTANCE, Instance, __in PFILE_OBJECT, FileObject, __out PFLT_CONTEXT*, Context );

        DYNLOAD_FUNC_WITH_05( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, NTAPI, FltSetFileContext, __in PFLT_INSTANCE, Instance, __in PFILE_OBJECT, FileObject, __in FLT_SET_CONTEXT_OPERATION, Operation, __in PFLT_CONTEXT, NewContext, __out PFLT_CONTEXT*, OldContext );

        DYNLOAD_FUNC_WITH_03( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, NTAPI, FltGetTransactionContext, __in PFLT_INSTANCE, Instance, __in PKTRANSACTION, Transaction, __out PFLT_CONTEXT*, Context );

        // The FltSetTransactionContext routine sets a context on a transaction.
        DYNLOAD_FUNC_WITH_05( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, NTAPI, FltSetTransactionContext, __in PFLT_INSTANCE, Instance, __in PKTRANSACTION, Transaction, __in FLT_SET_CONTEXT_OPERATION, Operation, __in PFLT_CONTEXT, NewContext, __out_opt PFLT_CONTEXT*, OldContext );

        // The FltEnlistInTransaction routine enlists a minifilter driver in a given transaction.
        DYNLOAD_FUNC_WITH_04( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, NTAPI, FltEnlistInTransaction, __in  PFLT_INSTANCE, Instance, __in PKTRANSACTION, Transaction, __in PFLT_CONTEXT, TransactionContext, __in NOTIFICATION_MASK, NotificationMask );

        // The FltIsVolumeWritable routine determines whether the disk device that corresponds to a volume or minifilter driver instance is writable.
        DYNLOAD_FUNC_WITH_02( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, NTAPI, FltIsVolumeWritable, __in PVOID, FltObject, __out PBOOLEAN, IsWritable );

        DYNLOAD_FUNC_WITH_02( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, FLTAPI, FltGetFileSystemType, __in PVOID, FltObject, __out PFLT_FILESYSTEM_TYPE, FileSystemType );
        /**
         * @brief The FltQueryDirectoryFile routine returns various kinds of information about files in the directory specified by a given file object.

            Windows Vista~
        */
        DYNLOAD_FUNC_WITH_09( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, FLTAPI, FltQueryDirectoryFile, __in PFLT_INSTANCE, Instance, __in PFILE_OBJECT, FileObject, __out PVOID, FileInformation, __in ULONG, Length, __in FILE_INFORMATION_CLASS, FileInformationClass, __in BOOLEAN, ReturnSingleEntry, __in_opt PUNICODE_STRING, FileName, __in BOOLEAN, RestartScan, __out_opt PULONG, LengthReturned );

    DYNLOAD_END_CLASS();

} // nsW32API

extern nsW32API::CFltMgrAPI GlobalFltMgr;

#endif // HDR_W32API_FILTER