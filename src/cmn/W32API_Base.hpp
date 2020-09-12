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

} // nsW32API

#endif // HDR_W32API_BASE