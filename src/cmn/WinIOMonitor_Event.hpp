#ifndef HDR_WINIOMONITOR_EVENT
#define HDR_WINIOMONITOR_EVENT

#if defined(USE_ON_KERNEL)
    #include "fltBase.hpp"
#else

    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif

    #include <windows.h>
    #include <winternl.h>
    #include <fltUser.h>
    #include <fltUserStructures.h>
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef enum _MSG_CATEGORY
{
    MSG_CATE_FILESYSTEM,
    MSG_CATE_FILESYSTEM_NOTIFY,
    MSG_CATE_PROCESS,
} MSG_CATEGORY;

typedef enum _MSG_FS_TYPE
{
    FS_PRE_CREATE                       = 0x1,
    FS_POST_CREATE                      = 0x2,

    FS_PRE_READ,
    FS_POST_READ,

    FS_PRE_WRITE,
    FS_POST_WRITE,

    FS_PRE_QUERY_INFORMATION,
    FS_POST_QUERY_INFORMATION,

    FS_PRE_SET_INFORMATION,
    FS_POST_SET_INFORMATION,

    FS_PRE_DIRECTORY_CONTROL,
    FS_POST_DIRECTORY_CONTROL,

    FS_PRE_QUERY_SECURITY,
    FS_POST_QUERY_SECURITY,

    FS_PRE_SET_SECURITY,
    FS_POST_SET_SECURITY,

    FS_PRE_CLEANUP,
    FS_POST_CLEANUP,

    FS_PRE_CLOSE,
    FS_POST_CLOSE, 
} MSG_FS_TYPE;

typedef enum _MSG_FS_NOTIFY_TYPE
{
    FILE_WAS_CREATED = 0x00000020,
    FILE_WAS_WRITTEN = 0x00000040,
    FILE_WAS_RENAMED = 0x00000080,
    FILE_WAS_DELETED = 0x00000100,
    FILE_SECURITY_CHANGED = 0x00000200,
    FILE_INFO_CHANGED = 0x00000400,
    FILE_WAS_READ = 0x00000800,
} MSG_FS_NOTIFY_TYPE;

typedef enum _MSG_PROC_TYPE
{
    PROC_WAS_CREATED = 0x1,
    PROC_WAS_TERMINATED = 0x2
} MSG_PROC_TYPE;

typedef union TyMsgParameters
{
    struct
    {
        ACCESS_MASK DesiredAccess;
        ULONG FileAttributes;
        ULONG ShareAccess;
        ULONG CreateDisposition;
        ULONG CreateOptions;
    } Create;

    union
    {
        struct
        {
            FILE_INFORMATION_CLASS FileInformationClass;

        } QueryDirectory;

    } DirectoryControl;

} MSG_PARAMETERS, *PMSG_PARAMETERS;

typedef struct _MSG_SEND_PACKET
{
    ULONG                           MessageSize;
    ULONG                           MessageCategory;
    ULONG                           MessageType;
    BOOLEAN                         IsNotified;
    LARGE_INTEGER                   EventTime;

    ULONG                           ProcessId;

    MSG_PARAMETERS                  Parameters;

    ULONG LengthOfSrcFileFullPath;                      // BYTE
    ULONG OffsetOfSrcFileFullPath;                      // BYTE
    ULONG LengthOfDstFileFullPath;                      // BYTE
    ULONG OffsetOfDstFileFullPath;                      // BYTE
    ULONG LengthOfProcessFullPath;                      // BYTE    
    ULONG OffsetOfProcessFullPath;                      // BYTE
    ULONG LengthOfContents;                             // BYTE
    ULONG OffsetOfContents;                             // BYTE

} MSG_SEND_PACKET, *PMSG_SEND_PACKET;

////The status return to filter,instruct filter driver what action needs to be done.
//typedef enum _FilterStatus
//{
//    FILTER_MESSAGE_IS_DIRTY = 0x00000001, //Set this flag if the reply message need to be processed.
//    FILTER_COMPLETE_PRE_OPERATION = 0x00000002, //Set this flag if complete the pre operation. 
//    FILTER_DATA_BUFFER_IS_UPDATED = 0x00000004, //Set this flag if the databuffer was updated.
//    FILTER_BLOCK_DATA_WAS_RETURNED = 0x00000008, //Set this flag if return read block databuffer to filter.
//    FILTER_CACHE_FILE_WAS_RETURNED = 0x00000010, //Set this flag if the whole cache file was downloaded.
//    FILTER_REHYDRATE_FILE_VIA_CACHE_FILE = 0x00000020, //Set this flag if the whole cache file was downloaded and stub file needs to be rehydrated.
//
//} FilterStatus, * PFilterStatus;

typedef enum _MSG_FILTER_STATUS
{
    FILTER_MSG_IS_DIRTY = 0x00000001
} MSG_FILTER_STATUS;

typedef struct _MSG_REPLY_PACKET
{
    ULONG                           ReturnStatus;           // IoStatus.Status
    ULONG                           FilterStatus;

    MSG_PARAMETERS                  Parameters;

    ULONG                           LengthOfContents;
    ULONG                           OffsetOfContents;

} MSG_REPLY_PACKET, *PMSG_REPLY_PACKET;

#endif // HDR_WINIOMONITOR_EVENT