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
    MSG_CATE_PROCESS
} MSG_CATEGORY;

typedef enum _MSG_TYPE
{
    FS_PRE_CREATE = 0x1,
    FS_POST_CREATE = 0x2,

    FS_PRE_CLEANUP,
    FS_POST_CLEANUP,

    FS_PRE_CLOSE,
    FS_POST_CLOSE, 
};

//typedef enum _MessageType
//{
//    PRE_CREATE = 0x00000001,
//    POST_CREATE = 0x00000002,
//    PRE_FASTIO_READ = 0x00000004,
//    POST_FASTIO_READ = 0x00000008,
//    PRE_CACHE_READ = 0x00000010,
//    POST_CACHE_READ = 0x00000020,
//    PRE_NOCACHE_READ = 0x00000040,
//    POST_NOCACHE_READ = 0x00000080,
//    PRE_PAGING_IO_READ = 0x00000100,
//    POST_PAGING_IO_READ = 0x00000200,
//    PRE_FASTIO_WRITE = 0x00000400,
//    POST_FASTIO_WRITE = 0x00000800,
//    PRE_CACHE_WRITE = 0x00001000,
//    POST_CACHE_WRITE = 0x00002000,
//    PRE_NOCACHE_WRITE = 0x00004000,
//    POST_NOCACHE_WRITE = 0x00008000,
//    PRE_PAGING_IO_WRITE = 0x00010000,
//    POST_PAGING_IO_WRITE = 0x00020000,
//    PRE_QUERY_INFORMATION = 0x00040000,
//    POST_QUERY_INFORMATION = 0x00080000,
//    PRE_SET_INFORMATION = 0x00100000,
//    POST_SET_INFORMATION = 0x00200000,
//    PRE_DIRECTORY = 0x00400000,
//    POST_DIRECTORY = 0x00800000,
//    PRE_QUERY_SECURITY = 0x01000000,
//    POST_QUERY_SECURITY = 0x02000000,
//    PRE_SET_SECURITY = 0x04000000,
//    POST_SET_SECURITY = 0x08000000,
//    PRE_CLEANUP = 0x10000000,
//    POST_CLEANUP = 0x20000000,
//    PRE_CLOSE = 0x40000000,
//    POST_CLOSE = 0x80000000UL,
//
//}MessageType;

typedef enum TyEnFileNotifyType
{
    FILE_WAS_CREATED = 0x00000020,
    FILE_WAS_WRITTEN = 0x00000040,
    FILE_WAS_RENAMED = 0x00000080,
    FILE_WAS_DELETED = 0x00000100,
    FILE_SECURITY_CHANGED = 0x00000200,
    FILE_INFO_CHANGED = 0x00000400,
    FILE_WAS_READ = 0x00000800,

} FileNotifyType, * PFileNotifyType;

typedef struct _MSG_SEND_PACKET
{
    ULONG                           MessageSize;
    ULONG                           MessageCategory;
    ULONG                           MessageType;
    BOOLEAN                         IsNotified;
    LARGE_INTEGER                   EventTime;

    ULONG                           ProcessId;

    union
    {
        struct
        {
            ACCESS_MASK DesiredAccess;
            ULONG FileAttributes;
            ULONG ShareAccess;
            ULONG CreateDisposition;
            ULONG CreateOptions;
        } Create;
        
    } Parameters;

    ULONG LengthOfSrcFileFullPath;                      // BYTE
    ULONG OffsetOfSrcFileFullPath;                      // BYTE
    ULONG LengthOfDstFileFullPath;                      // BYTE
    ULONG OffsetOfDstFileFullPath;                      // BYTE
    ULONG LengthOfProcessFullPath;                      // BYTE    
    ULONG OffsetOfProcessFullPath;                      // BYTE
    ULONG LengthOfContents;                             // BYTE
    ULONG OffsetOfContents;                             // BYTE

} MSG_SEND_PACKET, *PMSG_SEND_PACKET;

typedef struct _MSG_REPLY_PACKET
{
    ULONG                           ReturnStatus;

} MSG_REPLY_PACKET, *PMSG_REPLY_PACKET;

#endif // HDR_WINIOMONITOR_EVENT