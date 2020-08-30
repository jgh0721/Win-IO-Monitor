#ifndef HDR_WINIOMONITOR_W32API
#define HDR_WINIOMONITOR_W32API

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsW32API
{
    //  This helps us deal with ReFS 128-bit file IDs and NTFS 64-bit file IDs.
    typedef union _DF_FILE_REFERENCE
    {

        struct
        {
            ULONGLONG   Value;          //  The 64-bit file ID lives here.
            ULONGLONG   UpperZeroes;    //  In a 64-bit file ID this will be 0.
        } FileId64;

        UCHAR           FileId128[ 16 ];  //  The 128-bit file ID lives here.

    } DF_FILE_REFERENCE, * PDF_FILE_REFERENCE;

#define DfSizeofFileId(FID) (               \
    ((FID).FileId64.UpperZeroes == 0ll) ?   \
        sizeof((FID).FileId64.Value)    :   \
        sizeof((FID).FileId128)             \
    )

#define VOLUME_GUID_NAME_SIZE 48

    typedef NTSTATUS( FLTAPI* PFLT_TRANSACTION_NOTIFICATION_CALLBACK )
        ( __in PCFLT_RELATED_OBJECTS FltObjects,
          __in PFLT_CONTEXT TransactionContext,
          __in ULONG NotificationMask );

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

    typedef enum _FILE_INFORMATION_CLASS
    {
        FileDirectoryInformation = 1,
        FileFullDirectoryInformation,
        FileBothDirectoryInformation,
        FileBasicInformation,
        FileStandardInformation,
        FileInternalInformation,
        FileEaInformation,
        FileAccessInformation,
        FileNameInformation,
        FileRenameInformation,
        FileLinkInformation,
        FileNamesInformation,
        FileDispositionInformation,
        FilePositionInformation,
        FileFullEaInformation,
        FileModeInformation,
        FileAlignmentInformation,
        FileAllInformation,
        FileAllocationInformation,
        FileEndOfFileInformation,
        FileAlternateNameInformation,
        FileStreamInformation,
        FilePipeInformation,
        FilePipeLocalInformation,
        FilePipeRemoteInformation,
        FileMailslotQueryInformation,
        FileMailslotSetInformation,
        FileCompressionInformation,
        FileObjectIdInformation,
        FileCompletionInformation,
        FileMoveClusterInformation,
        FileQuotaInformation,
        FileReparsePointInformation,
        FileNetworkOpenInformation,
        FileAttributeTagInformation,
        FileTrackingInformation,
        FileIdBothDirectoryInformation,
        FileIdFullDirectoryInformation,
        FileValidDataLengthInformation,
        FileShortNameInformation,
        FileIoCompletionNotificationInformation,
        FileIoStatusBlockRangeInformation,
        FileIoPriorityHintInformation,
        FileSfioReserveInformation,
        FileSfioVolumeInformation,
        FileHardLinkInformation,
        FileProcessIdsUsingFileInformation,
        FileNormalizedNameInformation,
        FileNetworkPhysicalNameInformation,
        FileIdGlobalTxDirectoryInformation,
        FileIsRemoteDeviceInformation,
        FileAttributeCacheInformation,
        FileNumaNodeInformation,
        FileStandardLinkInformation,
        FileRemoteProtocolInformation,

        FileRenameInformationBypassAccessCheck,
        FileLinkInformationBypassAccessCheck,
        FileVolumeNameInformation,
        FileIdInformation,
        FileidExtdDirectoryInformation,
        FileReplaceCompletionInformation,
        FileHardLinkFullIdInformation,
        FileIdExtdBothDirectoryInformation,
        FileDispositionInformationEx,
        FileRenameInformationEx,
        FileRenameInformationExBypassAccessCheck,
        FileDesiredStorageClassInformation,
        FileStatInformation,
        FileMemoryPartitionInformation,
        FileStatLxInformation,
        FileCaseSensitiveInformation,
        FileLinkInformationEx,
        FileLinkInformationExBypassAccessCheck,
        FileStorageReserveIdInformation,
        FileCaseSensitiveInformationForceAccessCheck,

    } FILE_INFORMATION_CLASS;

    typedef struct _FILE_RENAME_INFORMATION
    {
        BOOLEAN ReplaceIfExists;
        HANDLE RootDirectory;
        ULONG FileNameLength;
        WCHAR FileName[ 1 ];
    } FILE_RENAME_INFORMATION, * PFILE_RENAME_INFORMATION;

    // _WIN32_WINNT >= _WIN32_WINNT_WIN10_RS1
    typedef struct _FILE_RENAME_INFORMATION_EX
    {
        union
        {
            BOOLEAN ReplaceIfExists;  // FileRenameInformation
            ULONG Flags;              // FileRenameInformationEx
        } DUMMYUNIONNAME;
        HANDLE RootDirectory;
        ULONG FileNameLength;
        WCHAR FileName[ 1 ];
    } FILE_RENAME_INFORMATION_EX, * PFILE_RENAME_INFORMATION_EX;

    #define FILE_RENAME_REPLACE_IF_EXISTS                       0x00000001 	// If a file with the given name already exists, it should be replaced with the given file.Equivalent to the ReplaceIfExists field used with the FileRenameInformation information class.
    #define FILE_RENAME_POSIX_SEMANTICS                         0x00000002 	// If FILE_RENAME_REPLACE_IF_EXISTS is also specified, allow replacing a file even if there are existing handles to it.Existing handles to the replaced file continue to be valid for operations such as readand write.Any subsequent opens of the target name will open the renamed file, not the replaced file.
    #define FILE_RENAME_SUPPRESS_PIN_STATE_INHERITANCE          0x00000004 	// When renaming a file to a new directory, suppress any inheritance rules related to the FILE_ATTRIBUTE_PINNEDand FILE_ATTRIBUTE_UNPINNED attributes of the file.
    #define FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE    0x00000008 	// When renaming a file to a new directory, suppress any inheritance rules related to the storage reserve ID property of the file.
    #define FILE_RENAME_NO_INCREASE_AVAILABLE_SPACE             0x00000010 	// If FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when renaming a file to a new directory, automatically resize affected storage reserve areas as needed to prevent the user visible free space on the volume from increasing.Requires manage volume access.
    #define FILE_RENAME_NO_DECREASE_AVAILABLE_SPACE             0x00000020 	// If FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when renaming a file to a new directory, automatically resize affected storage reserve areas as needed to prevent the user visible free space on the volume from decreasing.Requires manage volume access.
    #define FILE_RENAME_PRESERVE_AVAILABLE_SPACE                0x00000030 	// Equivalent to specifying both FILE_RENAME_NO_INCREASE_AVAILABLE_SPACE and FILE_RENAME_NO_DECREASE_AVAILABLE_SPACE.
    #define FILE_RENAME_IGNORE_READONLY_ATTRIBUTE               0x00000040 	// If FILE_RENAME_REPLACE_IF_EXISTS is also specified, allow replacing a file even if it is read - only.Requires WRITE_ATTRIBUTES access to the replaced file.
    #define FILE_RENAME_FORCE_RESIZE_TARGET_SR                  0x00000080 	// If FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when renaming a file to a new directory that is part of a different storage reserve area, always grow the target directory's storage reserve area by the full size of the file being renamed. Requires manage volume access.
    #define FILE_RENAME_FORCE_RESIZE_SOURCE_SR                  0x00000100 	// If FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when renaming a file to a new directory that is part of a different storage reserve area, always shrink the source directory's storage reserve area by the full size of the file being renamed. Requires manage volume access.
    #define FILE_RENAME_FORCE_RESIZE_SR                         0x00000180  // 

    typedef struct _FILE_DISPOSITION_INFORMATION
    {
        BOOLEAN DeleteFile;
    } FILE_DISPOSITION_INFORMATION, * PFILE_DISPOSITION_INFORMATION;

    typedef struct _FILE_DISPOSITION_INFORMATION_EX
    {
        ULONG Flags;
    } FILE_DISPOSITION_INFORMATION_EX, * PFILE_DISPOSITION_INFORMATION_EX;

    #define FILE_DISPOSITION_DO_NOT_DELETE              0x00000000      // Specifies the system should not delete a file.
    #define FILE_DISPOSITION_DELETE	                    0x00000001      // Specifies the system should delete a file.
    #define FILE_DISPOSITION_POSIX_SEMANTICS            0x00000002      // Specifies the system should perform a POSIX-style delete. See more info in Remarks.
    #define FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK  0x00000004      // Specifies the system should force an image section check.
    #define FILE_DISPOSITION_ON_CLOSE                   0x00000008      // Specifies if the system sets or clears the on-close state.
    #define FILE_DISPOSITION_IGNORE_READONLY_ATTRIBUTE  0x00000010      // Allows read-only files to be deleted. See more info in Remarks.

    typedef struct _FILE_END_OF_FILE_INFORMATION
    {
        LARGE_INTEGER EndOfFile;
    } FILE_END_OF_FILE_INFORMATION, * PFILE_END_OF_FILE_INFORMATION;

    typedef struct _FILE_LINK_INFORMATION
    {
        BOOLEAN ReplaceIfExists;

        HANDLE RootDirectory;
        ULONG FileNameLength;
        WCHAR FileName[ 1 ];
    } FILE_LINK_INFORMATION, * PFILE_LINK_INFORMATION;

    // #if (_WIN32_WINNT >= _WIN32_WINNT_WIN10_RS5)
    typedef struct _FILE_LINK_INFORMATION_EX
    {
        union
        {
            BOOLEAN ReplaceIfExists;  // FileLinkInformation
            ULONG Flags;              // FileLinkInformationEx
        } DUMMYUNIONNAME;

        HANDLE RootDirectory;
        ULONG FileNameLength;
        WCHAR FileName[ 1 ];
    } FILE_LINK_INFORMATION_EX, * PFILE_LINK_INFORMATION_EX;

    #define FILE_LINK_REPLACE_IF_EXISTS                     0x00000001  // If a file with the given name already exists, it should be replaced with the new link. Equivalent to the ReplaceIfExists field used with the FileLinkInformation information class.
    #define FILE_LINK_POSIX_SEMANTICS                       0x00000002  // If FILE_LINK_REPLACE_IF_EXISTS is also specified, allow replacing a file even if there are existing handles to it. Existing handles to the replaced file continue to be valid for operations such as read and write. Any subsequent opens of the target name will open the new link, not the replaced file.
    #define FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE  0x00000008  // When creating a link in a new directory, suppress any inheritance rules related to the storage reserve ID property of the file.
    #define FILE_LINK_NO_INCREASE_AVAILABLE_SPACE           0x00000010  // If FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when creating a link in a new directory, automatically resize affected storage reserve areas as needed to prevent the user visible free space on the volume from increasing. Requires manage volume access.
    #define FILE_LINK_NO_DECREASE_AVAILABLE_SPACE           0x00000020  // If FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when creating a link in a new directory, automatically resize affected storage reserve areas as needed to prevent the user visible free space on the volume from decreasing. Requires manage volume access.
    #define FILE_LINK_PRESERVE_AVAILABLE_SPACE              0x00000030  // Equivalent to specifying both FILE_LINK_NO_INCREASE_AVAILABLE_SPACE and FILE_LINK_NO_DECREASE_AVAILABLE_SPACE.
    #define FILE_LINK_IGNORE_READONLY_ATTRIBUTE             0x00000040  // If FILE_LINK_REPLACE_IF_EXISTS is also specified, allow replacing a file even if it is read-only. Requires WRITE_ATTRIBUTES access to the replaced file.
    #define FILE_LINK_FORCE_RESIZE_TARGET_SR                0x00000080  // If FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when creating a link in a new directory that is part of a different storage reserve area, always grow the target directory's storage reserve area by the full size of the file being linked. Requires manage volume access.
    #define FILE_LINK_FORCE_RESIZE_SOURCE_SR                0x00000100  // If FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when creating a link in a new directory that is part of a different storage reserve area, always shrink the source directory's storage reserve area by the full size of the file being linked. Requires manage volume access.
    #define FILE_LINK_FORCE_RESIZE_SR                       0x00000180  // Equivalent to specifying both FILE_LINK_FORCE_RESIZE_TARGET_SR and FILE_LINK_FORCE_RESIZE_SOURCE_SR.

    typedef struct _FILE_ALLOCATION_INFORMATION
    {
        LARGE_INTEGER AllocationSize;
    } FILE_ALLOCATION_INFORMATION, * PFILE_ALLOCATION_INFORMATION;

    typedef struct _FILE_ID_128
    {
        BYTE Identifier[ 16 ];
    } FILE_ID_128, * PFILE_ID_128;

    typedef struct _FILE_ID_INFORMATION
    {
        ULONGLONG VolumeSerialNumber;
        FILE_ID_128 FileId;
    } FILE_ID_INFORMATION, * PFILE_ID_INFORMATION;

    const LONGLONG FILE_INVALID_FILE_ID = -1;

    ///////////////////////////////////////////////////////////////////////////

    typedef struct _NtOsKrnlAPI
    {
        // The PsSetLoadImageNotifyRoutine routine registers a driver-supplied callback that is subsequently notified whenever an image is loaded (or mapped into memory).
        typedef NTSTATUS( NTAPI* PsSetLoadImageNotifyRoutine )( IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine );

        // The PsSetCreateProcessNotifyRoutine routine adds a driver-supplied callback routine to, or removes it from, a list of routines to be called whenever a process is created or deleted.
        typedef NTSTATUS( NTAPI* PsSetCreateProcessNotifyRoutine )( IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine, IN BOOLEAN Remove );

        // The PsSetCreateProcessNotifyRoutineEx routine registers or removes a callback routine that notifies the caller when a process is created or exits.
        typedef NTSTATUS( NTAPI* PsSetCreateProcessNotifyRoutineEx )( IN PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine, IN BOOLEAN Remove );

        // The IoGetTransactionParameterBlock routine returns the transaction parameter block for a transacted file operation.
        typedef PTXN_PARAMETER_BLOCK( NTAPI* IoGetTransactionParameterBlock )( __in  PFILE_OBJECT FileObject );

        /*!
         *   This routine returns the ImageFileName information from the process, if available.  This is a "lazy evaluation" wrapper 
             around SeInitializeProcessAuditName.  If the image file name information has already been computed, then this call simply
             allocates and returns a UNICODE_STRING with this information.  Otherwise, the function determines the name, stores the name in the 
             EPROCESS structure, and then allocates and returns a UNICODE_STRING.  Caller must free the memory returned in pImageFileName.
         */
        typedef NTSTATUS( NTAPI* SeLocateProcessImageName )( __in PEPROCESS Process, __deref_out PUNICODE_STRING* pImageFileName );

        typedef NTSTATUS( NTAPI* ZwQueryInformationProcess ) ( __in HANDLE ProcessHandle,
                                                               __in PROCESSINFOCLASS ProcessInformationClass,
                                                               __out_bcount( ProcessInformationLength ) PVOID ProcessInformation,
                                                               __in ULONG ProcessInformationLength,
                                                               __out_opt PULONG ReturnLength );

        PsSetLoadImageNotifyRoutine             pfnPsSetLoadImageNotifyRoutine;
        PsSetCreateProcessNotifyRoutine         pfnPsSetCreateProcessNotifyRoutine;
        PsSetCreateProcessNotifyRoutineEx       pfnPsSetCreateProcessNotifyRoutineEx;

        IoGetTransactionParameterBlock          pfnIoGetTransactionParameterBlock;

        SeLocateProcessImageName                pfnSeLocateProcessImageName;
        ZwQueryInformationProcess               pfnZwQueryInformationProcess;

    } NtOsKrnlAPI;

    typedef struct _FltMgrAPI
    {
        /**
         * @brief
         *
         * Windows XP SP3 ~ 
        */
        typedef NTSTATUS( FLTAPI* FltCreateFileEx )( __in PFLT_FILTER Filter,
                                                     __in_opt PFLT_INSTANCE Instance,
                                                     __out PHANDLE FileHandle,
                                                     __deref_opt_out PFILE_OBJECT* FileObject,
                                                     __in ACCESS_MASK DesiredAccess,
                                                     __in POBJECT_ATTRIBUTES ObjectAttributes,
                                                     __out PIO_STATUS_BLOCK IoStatusBlock,
                                                     __in_opt PLARGE_INTEGER AllocationSize,
                                                     __in ULONG FileAttributes,
                                                     __in ULONG ShareAccess,
                                                     __in ULONG CreateDisposition,
                                                     __in ULONG CreateOptions,
                                                     __in_bcount_opt( EaLength ) PVOID EaBuffer,
                                                     __in ULONG EaLength,
                                                     __in ULONG Flags );

        /**
         * @brief Minifilter drivers call FltCreateFileEx2 to create a new file or open an existing file. This routine also includes an optional create context parameter.
         *
         * Available in starting with Windows Vista
        */
        typedef NTSTATUS( NTAPI* FltCreateFileEx2 )( __in       PFLT_FILTER Filter,
                                                     __in_opt   PFLT_INSTANCE Instance,
                                                     __out      PHANDLE FileHandle,
                                                     __out_opt  PFILE_OBJECT* FileObject,
                                                     __in       ACCESS_MASK DesiredAccess,
                                                     __in       POBJECT_ATTRIBUTES ObjectAttributes,
                                                     __out      PIO_STATUS_BLOCK IoStatusBlock,
                                                     __in_opt   PLARGE_INTEGER AllocationSize,
                                                     __in       ULONG FileAttributes,
                                                     __in       ULONG ShareAccess,
                                                     __in       ULONG CreateDisposition,
                                                     __in       ULONG CreateOptions,
                                                     __in_opt   PVOID EaBuffer,
                                                     __in       ULONG EaLength,
                                                     __in       ULONG Flags,
                                                     __in_opt   PIO_DRIVER_CREATE_CONTEXT DriverContext );

        typedef NTSTATUS( NTAPI* FltGetFileContext )( __in PFLT_INSTANCE Instance,
                                                      __in PFILE_OBJECT FileObject,
                                                      __out PFLT_CONTEXT* Context );

        typedef NTSTATUS( NTAPI* FltSetFileContext )( __in PFLT_INSTANCE Instance,
                                                      __in PFILE_OBJECT FileObject,
                                                      __in FLT_SET_CONTEXT_OPERATION Operation,
                                                      __in PFLT_CONTEXT NewContext,
                                                      __out PFLT_CONTEXT* OldContext );

        typedef NTSTATUS( NTAPI* FltGetTransactionContext )( __in PFLT_INSTANCE Instance,
                                                             __in PKTRANSACTION Transaction,
                                                             __out PFLT_CONTEXT* Context );

        // The FltSetTransactionContext routine sets a context on a transaction.
        typedef NTSTATUS( NTAPI* FltSetTransactionContext )( __in PFLT_INSTANCE             Instance,
                                                             __in PKTRANSACTION             Transaction,
                                                             __in FLT_SET_CONTEXT_OPERATION Operation,
                                                             __in PFLT_CONTEXT              NewContext,
                                                             __out_opt PFLT_CONTEXT* OldContext );

        // The FltEnlistInTransaction routine enlists a minifilter driver in a given transaction.
        typedef NTSTATUS( NTAPI* FltEnlistInTransaction )( __in  PFLT_INSTANCE Instance,
                                                           __in  PKTRANSACTION Transaction,
                                                           __in  PFLT_CONTEXT TransactionContext,
                                                           __in  NOTIFICATION_MASK NotificationMask );

        // The FltIsVolumeWritable routine determines whether the disk device that corresponds to a volume or minifilter driver instance is writable.
        typedef NTSTATUS( NTAPI* FltIsVolumeWritable )( __in PVOID FltObject, __out PBOOLEAN IsWritable );

        typedef NTSTATUS( FLTAPI* FltGetFileSystemType )( __in PVOID FltObject,
                                                          __out PFLT_FILESYSTEM_TYPE FileSystemType );

        FltCreateFileEx                     pfnFltCreateFileEx;
        FltCreateFileEx2                    pfnFltCreateFileEx2;

        FltGetFileContext                   pfnFltGetFileContext;
        FltSetFileContext                   pfnFltSetFileContext;
        FltGetTransactionContext            pfnFltGetTransactionContext;
        FltSetTransactionContext            pfnFltSetTransactionContext;
        FltEnlistInTransaction              pfnFltEnlistInTransaction;

        FltIsVolumeWritable                 pfnFltIsVolumeWritable;
        FltGetFileSystemType                pfnFltGetFileSystemType;
    } FltMgrAPI;

    const char* ConvertFileInformationClassTo( __in const FILE_INFORMATION_CLASS FileInformationClass );

    const char* ConvertIRPMajorFunction( __in UCHAR MajorFunction );

    const char* ConvertCreateShareAccess( __in ULONG ShareAccess );
    const char* ConvertCreateDisposition( __in ULONG CreateDisposition );

    ///////////////////////////////////////////////////////////////////////////

    void InitializeNtOsKrnlAPI( __in NtOsKrnlAPI* ntOsKrnlAPI );
    void InitializeFltMgrAPI( __in FltMgrAPI* fltMgrAPI );

    extern nsW32API::NtOsKrnlAPI NtOsKrnlAPIMgr;
    extern nsW32API::FltMgrAPI FltMgrAPIMgr;

    ///////////////////////////////////////////////////////////////////////////

#ifndef IOCTL_DISK_BASE
#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK
#endif

#ifndef IOCTL_DISK_IS_WRITABLE
#define IOCTL_DISK_IS_WRITABLE          CTL_CODE(IOCTL_DISK_BASE, 0x0009, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

    /**
     * @brief The FltIsVolumeWritable routine determines whether the disk device that corresponds to a volume or minifilter driver instance is writable. 
     * @param FltObject  Volume or Instance Object
     * @param IsWritable 
     * @return
     *
     * Support WinXP
    */
    NTSTATUS IsVolumeWritable( __in PVOID FltObject, __out PBOOLEAN IsWritable );

} // nsW32API

#define IsClassContainDst( FileInformationClass ) \
    ( ( FileInformationClass == nsW32API::FileRenameInformation ) || \
      ( FileInformationClass == nsW32API::FileRenameInformationEx ) || \
      ( FileInformationClass == nsW32API::FileLinkInformation ) || \
      ( FileInformationClass == nsW32API::FileLinkInformationEx ) )

#endif // HDR_WINIOMONITOR_W32API