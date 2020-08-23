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