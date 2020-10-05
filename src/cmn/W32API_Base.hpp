#ifndef HDR_W32API_BASE
#define HDR_W32API_BASE

#include "fltBase.hpp"
#include "W32API_LibMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define ABSOLUTE(wait)          (wait)
#define RELATIVE(wait)          (-(wait))
#define NANOSECONDS(nanos)      (((signed __int64)(nanos)) / 100L)
#define MICROSECONDS(micros)    (((signed __int64)(micros)) * NANOSECONDS(1000L))
#define MILLISECONDS(milli)     (((signed __int64)(milli)) * MICROSECONDS(1000L))
#define SECONDS(seconds)        (((signed __int64)(seconds)) * MILLISECONDS(1000L))

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE) -1)
#endif

#define IRP_UM_DRIVER_INITIATED_IO 0x00400000

#define FILE_OPEN_REQUIRING_OPLOCK              0x00010000
#define FILE_DISALLOW_EXCLUSIVE                 0x00020000
// #if (NTDDI_VERSION >= NTDDI_WIN7)
#define IRP_MN_DEVICE_ENUMERATED                0x19

// #if (_WIN32_WINNT >= 0x0600)
#define FSCTL_MAKE_MEDIA_COMPATIBLE         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 76, METHOD_BUFFERED, FILE_WRITE_DATA) // UDFS R/W
#define FSCTL_SET_DEFECT_MANAGEMENT         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 77, METHOD_BUFFERED, FILE_WRITE_DATA) // UDFS R/W
#define FSCTL_QUERY_SPARING_INFO            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 78, METHOD_BUFFERED, FILE_ANY_ACCESS) // UDFS R/W
#define FSCTL_QUERY_ON_DISK_VOLUME_INFO     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 79, METHOD_BUFFERED, FILE_ANY_ACCESS) // C/UDFS
#define FSCTL_SET_VOLUME_COMPRESSION_STATE  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 80, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // VOLUME_COMPRESSION_STATE
// decommissioned fsctl value                                                 80
#define FSCTL_TXFS_MODIFY_RM                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 81, METHOD_BUFFERED, FILE_WRITE_DATA) // TxF
#define FSCTL_TXFS_QUERY_RM_INFORMATION     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 82, METHOD_BUFFERED, FILE_READ_DATA)  // TxF
// decommissioned fsctl value                                                 83
#define FSCTL_TXFS_ROLLFORWARD_REDO         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 84, METHOD_BUFFERED, FILE_WRITE_DATA) // TxF
#define FSCTL_TXFS_ROLLFORWARD_UNDO         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 85, METHOD_BUFFERED, FILE_WRITE_DATA) // TxF
#define FSCTL_TXFS_START_RM                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 86, METHOD_BUFFERED, FILE_WRITE_DATA) // TxF
#define FSCTL_TXFS_SHUTDOWN_RM              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 87, METHOD_BUFFERED, FILE_WRITE_DATA) // TxF
#define FSCTL_TXFS_READ_BACKUP_INFORMATION  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 88, METHOD_BUFFERED, FILE_READ_DATA)  // TxF
#define FSCTL_TXFS_WRITE_BACKUP_INFORMATION CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 89, METHOD_BUFFERED, FILE_WRITE_DATA) // TxF
#define FSCTL_TXFS_CREATE_SECONDARY_RM      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 90, METHOD_BUFFERED, FILE_WRITE_DATA) // TxF
#define FSCTL_TXFS_GET_METADATA_INFO        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 91, METHOD_BUFFERED, FILE_READ_DATA)  // TxF
#define FSCTL_TXFS_GET_TRANSACTED_VERSION   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 92, METHOD_BUFFERED, FILE_READ_DATA)  // TxF
// decommissioned fsctl value                                                 93
#define FSCTL_TXFS_SAVEPOINT_INFORMATION    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 94, METHOD_BUFFERED, FILE_WRITE_DATA) // TxF
#define FSCTL_TXFS_CREATE_MINIVERSION       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 95, METHOD_BUFFERED, FILE_WRITE_DATA) // TxF
// decommissioned fsctl value                                                 96
// decommissioned fsctl value                                                 97
// decommissioned fsctl value                                                 98
#define FSCTL_TXFS_TRANSACTION_ACTIVE       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 99, METHOD_BUFFERED, FILE_READ_DATA)  // TxF
#define FSCTL_SET_ZERO_ON_DEALLOCATION      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 101, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_SET_REPAIR                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 102, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_REPAIR                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 103, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_WAIT_FOR_REPAIR               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 104, METHOD_BUFFERED, FILE_ANY_ACCESS)
// decommissioned fsctl value                                                 105
#define FSCTL_INITIATE_REPAIR               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 106, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSC_INTERNAL                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 107, METHOD_NEITHER, FILE_ANY_ACCESS) // CSC internal implementation
#define FSCTL_SHRINK_VOLUME                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 108, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // SHRINK_VOLUME_INFORMATION
#define FSCTL_SET_SHORT_NAME_BEHAVIOR       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 109, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFSR_SET_GHOST_HANDLE_STATE   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 110, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//  Values 111 - 119 are reserved for FSRM.
//

#define FSCTL_TXFS_LIST_TRANSACTION_LOCKED_FILES \
                                            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 120, METHOD_BUFFERED, FILE_READ_DATA) // TxF
#define FSCTL_TXFS_LIST_TRANSACTIONS        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 121, METHOD_BUFFERED, FILE_READ_DATA) // TxF
#define FSCTL_QUERY_PAGEFILE_ENCRYPTION     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 122, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_RESET_VOLUME_ALLOCATION_HINTS CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 123, METHOD_BUFFERED, FILE_ANY_ACCESS)

// #if (_WIN32_WINNT >= 0x0601)
#define FSCTL_QUERY_DEPENDENT_VOLUME        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 124, METHOD_BUFFERED, FILE_ANY_ACCESS)    // Dependency File System Filter
#define FSCTL_SD_GLOBAL_CHANGE              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 125, METHOD_BUFFERED, FILE_ANY_ACCESS) // Update NTFS Security Descriptors

#define FSCTL_TXFS_READ_BACKUP_INFORMATION2 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 126, METHOD_BUFFERED, FILE_ANY_ACCESS) // TxF

// #if (_WIN32_WINNT >= 0x0601)
#define FSCTL_LOOKUP_STREAM_FROM_CLUSTER    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 127, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_TXFS_WRITE_BACKUP_INFORMATION2 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 128, METHOD_BUFFERED, FILE_ANY_ACCESS) // TxF
#define FSCTL_FILE_TYPE_NOTIFICATION        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 129, METHOD_BUFFERED, FILE_ANY_ACCESS)


//
//  Values 130 - 130 are available
//

//
//  Values 131 - 139 are reserved for FSRM.
//

// #if (_WIN32_WINNT >= 0x0601)
#define FSCTL_GET_BOOT_AREA_INFO            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 140, METHOD_BUFFERED, FILE_ANY_ACCESS) // BOOT_AREA_INFO
#define FSCTL_GET_RETRIEVAL_POINTER_BASE    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 141, METHOD_BUFFERED, FILE_ANY_ACCESS) // RETRIEVAL_POINTER_BASE
#define FSCTL_SET_PERSISTENT_VOLUME_STATE   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 142, METHOD_BUFFERED, FILE_ANY_ACCESS)  // FILE_FS_PERSISTENT_VOLUME_INFORMATION
#define FSCTL_QUERY_PERSISTENT_VOLUME_STATE CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 143, METHOD_BUFFERED, FILE_ANY_ACCESS)  // FILE_FS_PERSISTENT_VOLUME_INFORMATION

#define FSCTL_REQUEST_OPLOCK                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 144, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_CSV_TUNNEL_REQUEST            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 145, METHOD_BUFFERED, FILE_ANY_ACCESS) // CSV_TUNNEL_REQUEST
#define FSCTL_IS_CSV_FILE                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 146, METHOD_BUFFERED, FILE_ANY_ACCESS) // IS_CSV_FILE

#define FSCTL_QUERY_FILE_SYSTEM_RECOGNITION CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 147, METHOD_BUFFERED, FILE_ANY_ACCESS) // 
#define FSCTL_CSV_GET_VOLUME_PATH_NAME      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 148, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_GET_VOLUME_NAME_FOR_VOLUME_MOUNT_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 149, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_GET_VOLUME_PATH_NAMES_FOR_VOLUME_NAME CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 150,  METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_IS_FILE_ON_CSV_VOLUME         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 151,  METHOD_BUFFERED, FILE_ANY_ACCESS)


namespace nsW32API
{
    enum TyEnOsBuildNumber
    {
        WIN10_BUILD_TH1_1507 = 10240,
        WIN10_BUILD_TH2_1511 = 10586,
        WIN10_BUILD_RS1_1607 = 14393,
        WIN10_BUILD_RS2_1703 = 15063,
        WIN10_BUILD_RS3_1709 = 16299,
        WIN10_BUILD_RS4_1803 = 17134,
        WIN10_BUILD_RS5_1809 = 17763,
        WIN10_BUILD_19H1_1903 = 18362,
        WIN10_BUILD_19H2_1909 = 18363,
        WIN10_BUILD_20H1_2004 = 19041,
        WIN10_BUILD_20H2_2009 = 19042,
    };

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

    typedef enum _FSINFOCLASS
    {
        FileFsVolumeInformation = 1,
        FileFsLabelInformation,
        FileFsSizeInformation,
        FileFsDeviceInformation,
        FileFsAttributeInformation,
        FileFsControlInformation,
        FileFsFullSizeInformation,
        FileFsObjectIdInformation,
        FileFsDriverPathInformation,
        FileFsVolumeFlagsInformation,
        FileFsSectorSizeInformation,
        FileFsDataCopyInformation,
        FileFsMetadataSizeInformation,
        FileFsFullSizeInformationEx,
        FileFsMaximumInformation
    } FS_INFORMATION_CLASS, * PFS_INFORMATION_CLASS;

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

    //  This helps us deal with ReFS 128-bit file IDs and NTFS 64-bit file IDs.
    typedef union _UNIFIED_FILE_REFERENCE
    {
        struct
        {
            ULONGLONG   Value;          //  The 64-bit file ID lives here.
            ULONGLONG   UpperZeroes;    //  In a 64-bit file ID this will be 0.
        } FileId64;

        UCHAR           FileId128[ 16 ];  //  The 128-bit file ID lives here.

    } UNIFIED_FILE_REFERENCE, * PUNIFIED_FILE_REFERENCE;

#define UnifiedSizeofFileId(FID) (               \
    ((FID).FileId64.UpperZeroes == 0ll) ?   \
        sizeof((FID).FileId64.Value)    :   \
        sizeof((FID).FileId128)             \
    )

    typedef struct _FILE_FS_FULL_SIZE_INFORMATION_EX
    {
        ULONGLONG ActualTotalAllocationUnits;
        ULONGLONG ActualAvailableAllocationUnits;
        ULONGLONG ActualPoolUnavailableAllocationUnits;
        ULONGLONG CallerTotalAllocationUnits;
        ULONGLONG CallerAvailableAllocationUnits;
        ULONGLONG CallerPoolUnavailableAllocationUnits;
        ULONGLONG UsedAllocationUnits;
        ULONGLONG TotalReservedAllocationUnits;
        ULONGLONG VolumeStorageReserveAllocationUnits;
        ULONGLONG AvailableCommittedAllocationUnits;
        ULONGLONG PoolAvailableAllocationUnits;
        ULONG     SectorsPerAllocationUnit;
        ULONG     BytesPerSector;
    } FILE_FS_FULL_SIZE_INFORMATION_EX, * PFILE_FS_FULL_SIZE_INFORMATION_EX;

    typedef enum _FSRTL_CHANGE_BACKING_TYPE
    {
        ChangeDataControlArea,
        ChangeImageControlArea,
        ChangeSharedCacheMap
    } FSRTL_CHANGE_BACKING_TYPE, * PFSRTL_CHANGE_BACKING_TYPE;

#ifdef __cplusplus
    typedef struct _FSRTL_ADVANCED_FCB_HEADER_XP : FSRTL_COMMON_FCB_HEADER
    {
#else   // __cplusplus

    typedef struct _FSRTL_ADVANCED_FCB_HEADER_XP
    {

        //
        //  Put in the standard FsRtl header fields
        //

        FSRTL_COMMON_FCB_HEADER DUMMYSTRUCTNAME;

#endif  // __cplusplus

        //
        //  The following two fields are supported only if
        //  Flags2 contains FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS
        //

        //
        //  This is a pointer to a Fast Mutex which may be used to
        //  properly synchronize access to the FsRtl header.  The
        //  Fast Mutex must be nonpaged.
        //

        PFAST_MUTEX FastMutex;

        //
        // This is a pointer to a list of stream context structures belonging to
        // filesystem filter drivers that are linked above the filesystem.
        // Each structure is headed by FSRTL_FILTER_CONTEXT.
        //

        LIST_ENTRY FilterContexts;

    } FSRTL_ADVANCED_FCB_HEADER_XP, *PFSRTL_ADVANCED_FCB_HEADER_XP;

#ifdef __cplusplus
    typedef struct _FSRTL_ADVANCED_FCB_HEADER_VISTA : FSRTL_COMMON_FCB_HEADER
    {
#else   // __cplusplus

    typedef struct _FSRTL_ADVANCED_FCB_HEADER_VISTA
    {

        //
        //  Put in the standard FsRtl header fields
        //

        FSRTL_COMMON_FCB_HEADER DUMMYSTRUCTNAME;

#endif  // __cplusplus

        //
        //  The following two fields are supported only if
        //  Flags2 contains FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS
        //

        //
        //  This is a pointer to a Fast Mutex which may be used to
        //  properly synchronize access to the FsRtl header.  The
        //  Fast Mutex must be nonpaged.
        //

        PFAST_MUTEX FastMutex;

        //
        // This is a pointer to a list of stream context structures belonging to
        // filesystem filter drivers that are linked above the filesystem.
        // Each structure is headed by FSRTL_FILTER_CONTEXT.
        //

        LIST_ENTRY FilterContexts;

        //
        //  The following fields are valid only if the Version
        //  field in the FSRTL_COMMON_FCB_HEADER is greater than
        //  or equal to FSRTL_FCB_HEADER_V1
        //  These fields are present in VISTA and beyond
        //

        //
        //  This is a pushlock which is used to properly synchronize access
        //  to the list of stream contexts
        //

        EX_PUSH_LOCK PushLock;

        //
        //  This is a pointer to a blob of information that is
        //  associated with the opened file in the filesystem
        //  corresponding to the structure containing this
        //  FSRTL_ADVANCED_FCB_HEADER.
        //

        PVOID* FileContextSupportPointer;

    } FSRTL_ADVANCED_FCB_HEADER_VISTA, * PFSRTL_ADVANCED_FCB_HEADER_VISTA;

#ifdef __cplusplus
    typedef struct _FSRTL_ADVANCED_FCB_HEADER_WIN8 : FSRTL_COMMON_FCB_HEADER
    {
#else   // __cplusplus

    typedef struct _FSRTL_ADVANCED_FCB_HEADER_WIN8
    {

        //
        //  Put in the standard FsRtl header fields
        //

        FSRTL_COMMON_FCB_HEADER DUMMYSTRUCTNAME;

#endif  // __cplusplus

        //
        //  The following two fields are supported only if
        //  Flags2 contains FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS
        //

        //
        //  This is a pointer to a Fast Mutex which may be used to
        //  properly synchronize access to the FsRtl header.  The
        //  Fast Mutex must be nonpaged.
        //

        PFAST_MUTEX FastMutex;

        //
        // This is a pointer to a list of stream context structures belonging to
        // filesystem filter drivers that are linked above the filesystem.
        // Each structure is headed by FSRTL_FILTER_CONTEXT.
        //

        LIST_ENTRY FilterContexts;

        //
        //  The following fields are valid only if the Version
        //  field in the FSRTL_COMMON_FCB_HEADER is greater than
        //  or equal to FSRTL_FCB_HEADER_V1
        //  These fields are present in VISTA and beyond
        //

        //
        //  This is a pushlock which is used to properly synchronize access
        //  to the list of stream contexts
        //

        EX_PUSH_LOCK PushLock;

        //
        //  This is a pointer to a blob of information that is
        //  associated with the opened file in the filesystem
        //  corresponding to the structure containing this
        //  FSRTL_ADVANCED_FCB_HEADER.
        //

        PVOID* FileContextSupportPointer;

        //
        //  The following fields are valid only if the Version
        //  field in the FSRTL_COMMON_FCB_HEADER is greater than
        //  or equal to FSRTL_FCB_HEADER_V2.  These fields are
        //  present in Windows 8 and beyond.
        //

        //
        //  For local file system this is the oplock field used
        //  by the oplock package to maintain current information
        //  about opportunistic locks on this file/directory.
        //
        //  For remote file systems this field is reserved.
        //

        union
        {

            OPLOCK Oplock;
            PVOID ReservedForRemote;

        };

    } FSRTL_ADVANCED_FCB_HEADER_WIN8, * PFSRTL_ADVANCED_FCB_HEADER_WIN8;

#ifdef __cplusplus
    typedef struct _FSRTL_ADVANCED_FCB_HEADER_WIN10 : FSRTL_COMMON_FCB_HEADER
    {
#else   // __cplusplus

    typedef struct _FSRTL_ADVANCED_FCB_HEADER_WIN10
    {

        //
        //  Put in the standard FsRtl header fields
        //

        FSRTL_COMMON_FCB_HEADER DUMMYSTRUCTNAME;

#endif  // __cplusplus

        //
        //  The following two fields are supported only if
        //  Flags2 contains FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS
        //

        //
        //  This is a pointer to a Fast Mutex which may be used to
        //  properly synchronize access to the FsRtl header.  The
        //  Fast Mutex must be nonpaged.
        //

        PFAST_MUTEX FastMutex;

        //
        // This is a pointer to a list of stream context structures belonging to
        // filesystem filter drivers that are linked above the filesystem.
        // Each structure is headed by FSRTL_FILTER_CONTEXT.
        //

        LIST_ENTRY FilterContexts;

        //
        //  The following fields are valid only if the Version
        //  field in the FSRTL_COMMON_FCB_HEADER is greater than
        //  or equal to FSRTL_FCB_HEADER_V1
        //  These fields are present in VISTA and beyond
        //

        //
        //  This is a pushlock which is used to properly synchronize access
        //  to the list of stream contexts
        //

        EX_PUSH_LOCK PushLock;

        //
        //  This is a pointer to a blob of information that is
        //  associated with the opened file in the filesystem
        //  corresponding to the structure containing this
        //  FSRTL_ADVANCED_FCB_HEADER.
        //

        PVOID* FileContextSupportPointer;

        //
        //  The following fields are valid only if the Version
        //  field in the FSRTL_COMMON_FCB_HEADER is greater than
        //  or equal to FSRTL_FCB_HEADER_V2.  These fields are
        //  present in Windows 8 and beyond.
        //

        //
        //  For local file system this is the oplock field used
        //  by the oplock package to maintain current information
        //  about opportunistic locks on this file/directory.
        //
        //  For remote file systems this field is reserved.
        //

        union
        {

            OPLOCK Oplock;
            PVOID ReservedForRemote;

        };

        //
        //  This field is used internally by the FSRTL to assist in context lookup.
        //

        PVOID ReservedContext;
        
    } FSRTL_ADVANCED_FCB_HEADER_WIN10, * PFSRTL_ADVANCED_FCB_HEADER_WIN10;
    
        // Win8~
#define FSRTL_FCB_HEADER_V2             (0x02)
        // Win10~
#define FSRTL_FCB_HEADER_V3             (0x03)

    //
    //  This will properly initialize the advanced header so that it can be
    //  used with PerStream contexts and PerFile contexts.
    //  Note:  A fast mutex must be placed in an advanced header.  It is the
    //         caller's responsibility to properly create and initialize this
    //         mutex before calling this macro.  The mutex field is only set
    //         if a non-NULL value is passed in.
    //  If the file system supports filter file contexts then it must
    //  initialize the FileContextSupportPointer field to point to a PVOID
    //  embedded in its per-file structure (FCB). If a NULL is passed in,
    //  then the macro assumes that the file system does not support filter
    //  file contexts
    //

    #define FsRtlSetupAdvancedHeaderEx( _advhdr, _fmutx, _fctxptr )                     \
    {                                                                                   \
        FsRtlSetupAdvancedHeader( _advhdr, _fmutx );                                    \
        if ((_fctxptr) != NULL) {                                                       \
            (_advhdr)->FileContextSupportPointer = (_fctxptr);                          \
        }                                                                               \
    }

    //
    //  Structures for FSCTL_REQUEST_OPLOCK
    //

    #define OPLOCK_LEVEL_CACHE_READ         (0x00000001)
    #define OPLOCK_LEVEL_CACHE_HANDLE       (0x00000002)
    #define OPLOCK_LEVEL_CACHE_WRITE        (0x00000004)

    #define REQUEST_OPLOCK_INPUT_FLAG_REQUEST               (0x00000001)
    #define REQUEST_OPLOCK_INPUT_FLAG_ACK                   (0x00000002)
    #define REQUEST_OPLOCK_INPUT_FLAG_COMPLETE_ACK_ON_CLOSE (0x00000004)

    #define REQUEST_OPLOCK_CURRENT_VERSION          1

    typedef struct _REQUEST_OPLOCK_INPUT_BUFFER
    {

        //
        //  This should be set to REQUEST_OPLOCK_CURRENT_VERSION.
        //

        USHORT StructureVersion;

        USHORT StructureLength;

        //
        //  One or more OPLOCK_LEVEL_CACHE_* values to indicate the desired level of the oplock.
        //

        ULONG RequestedOplockLevel;

        //
        //  REQUEST_OPLOCK_INPUT_FLAG_* flags.
        //

        ULONG Flags;

    } REQUEST_OPLOCK_INPUT_BUFFER, * PREQUEST_OPLOCK_INPUT_BUFFER;

    #define REQUEST_OPLOCK_OUTPUT_FLAG_ACK_REQUIRED     (0x00000001)
    #define REQUEST_OPLOCK_OUTPUT_FLAG_MODES_PROVIDED   (0x00000002)

    typedef struct _REQUEST_OPLOCK_OUTPUT_BUFFER
    {

        //
        //  This should be set to REQUEST_OPLOCK_CURRENT_VERSION.
        //

        USHORT StructureVersion;

        USHORT StructureLength;

        //
        //  One or more OPLOCK_LEVEL_CACHE_* values indicating the level of the oplock that
        //  was just broken.
        //

        ULONG OriginalOplockLevel;

        //
        //  One or more OPLOCK_LEVEL_CACHE_* values indicating the level to which an oplock
        //  is being broken, or an oplock level that may be available for granting, depending
        //  on the operation returning this buffer.
        //

        ULONG NewOplockLevel;

        //
        //  REQUEST_OPLOCK_OUTPUT_FLAG_* flags.
        //

        ULONG Flags;

        //
        //  When REQUEST_OPLOCK_OUTPUT_FLAG_MODES_PROVIDED is set, and when the
        //  OPLOCK_LEVEL_CACHE_HANDLE level is being lost in an oplock break, these fields
        //  contain the access mode and share mode of the request that is causing the break.
        //

        ACCESS_MASK AccessMode;

        USHORT ShareMode;

    } REQUEST_OPLOCK_OUTPUT_BUFFER, * PREQUEST_OPLOCK_OUTPUT_BUFFER;


} // nsW32API

#endif // HDR_W32API_BASE