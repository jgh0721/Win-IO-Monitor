#ifndef HDR_WINIOMONITOR_EVENT
#define HDR_WINIOMONITOR_EVENT

#if defined(USE_ON_KERNEL)
#include "base.hpp"
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

constexpr int CONTAINOR_SUFFIX_MAX = 12;

enum class FILE_TYPE
{
    FILE_TYPE_NORMAL                    = 0x0,
    FILE_TYPE_ISOLATED                  = 0x1,
    FILE_TYPE_ENCRYPTED                 = 0x2,
    FILE_TYPE_CONTAINOR                 = 0x4
};

enum class ENCRYPTION_METHOD
{
    ENCRYPTION_NONE,
    ENCRYPTION_XOR,
    ENCRYPTION_XTEA,
    ENCRYPTION_AES128,
    ENCRYPTION_AES192,
    ENCRYPTION_AES256
};

enum class MSG_CATEGORY
{
    MSG_CATE_NONE,
    MSG_CATE_PROCESS,
    MSG_CATE_FILESYSTEM,
    MSG_CATE_FILESYSTEM_NOTIFY,
    MSG_CATE_DEVICE,
    MSG_CATE_REGISTRY
};

enum class MSG_FILESYSTEM
{
    FS_PRE_CREATE,
    FS_POST_CREATE,

    FS_PRE_READ,
    FS_POST_READ,

    FS_PRE_WRITE,
    FS_POST_WRITE,

    FS_PRE_QUERY_INFORMATION,
    FS_POST_QUERY_INFORMATION,

    FS_PRE_SET_INFORMATION,
    FS_POST_SET_INFORMATION,

    FS_PRE_DIRECTORY_INFORMATION,
    FS_POST_DIRECTORY_INFORMATION,

    FS_PRE_QUERY_SECURITY,
    FS_POST_QUERY_SECURITY,

    FS_PRE_SET_SECURITY,
    FS_POST_SET_SECURITY,

    FS_PRE_CLEANUP,
    FS_POST_CLEANUP,

    FS_PRE_CLOSE,
    FS_POST_CLOSE
};

enum class MSG_FILESYSTEM_NOTIFY : uint32_t
{
    FILE_WAS_CREATED,
    FILE_WAS_WRITTEN,
    FILE_WAS_RENAMED,
    FILE_WAS_DELETED,
    FILE_INFO_WAS_CHANGED,
    FILE_SECURITY_WAS_CHANGED,
    FILE_WAS_READ
};

enum class MSG_PROCESS
{
    PROC_WAS_CREATED,
    PROC_WAS_TERMINATED
};

enum class MSG_REGISTRY
{
    
};

typedef struct _DRIVER_CONFIG
{
    ULONG                               SizeOfStruct        = 0;

    ULONG                               TimeOutMs           = 0;

    ULONG                               FeatureSet          = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// File

    ULONG                               SolutionHDRSize     = 0;

} DRIVER_CONFIG, *PDRIVER_CONFIG;

enum class PROCESS_FILTER_TYPE
{
    PROCESS_FILTER_NONE                 = 0x0,

    PROCESS_CONTROL_CREATION            = 0x1,
    PROCESS_CONTROL_TERMINATION         = 0x2,

    PROCESS_APPLY_CHILDREN              = 0x4
};

typedef struct _USER_PROCESS_FILTER
{
    UUID                                Id;

    ULONG                               ProcessId;
    WCHAR                               ProcessFilterMask[ MAX_PATH ];
    ULONG                               ProcessFilter;

} USER_PROCESS_FILTER;

typedef struct _USER_GLOBAL_FS_FILTER
{
    UUID                                Id;

    WCHAR                               FilterMask[ MAX_PATH ];
    BOOLEAN                             IsInclude;

} USER_GLOBAL_FS_FILTER;

typedef struct _USER_PROC_FS_FILTER
{
    UUID                                Id;

    ULONG                               ProcessId;
    WCHAR                               ProcessFilterMask[ MAX_PATH ];


} USER_PROC_FS_FILTER;

struct TyCreateOptions
{
    ACCESS_MASK                     DesiredAccess;
    ULONG                           FileAttributes;
    ULONG                           ShareAccess;
    ULONG                           CreateDisposition;
    ULONG                           CreateOptions;
};

typedef union TyMsgParameters
{
    struct _Create
    {
        LONGLONG                        FileSize;
        BOOLEAN                         IsAlreadyExists;
        BOOLEAN                         IsContainSolutionMetaData;

        TyCreateOptions                 CreateOptions;

    } Create;

    struct _CreateResult
    {
        TyCreateOptions                 CreateOptions;

        BOOLEAN                         IsUseIsolation;

        BOOLEAN                         IsUseContainor;
        WCHAR                           ContainorSuffix[ CONTAINOR_SUFFIX_MAX ];

        BOOLEAN                         IsUseEncryption;

        // if this is true, use SolutionHDRSize && SolutionHDROffset
        // but, must same or less than DRIVER_CONFIG.SolutionHDRSize
        BOOLEAN                         IsUseSolutionMetaData;

    } CreateResult;

    struct _SetInformation
    {
        FILE_TYPE                       Type;
        ULONG                           FileInformationClass;

        union
        {
            BOOLEAN ReplaceIfExists;
            BOOLEAN AdvanceOnly;
        };

    } SetInformation;

    struct _QueryInformation
    {
        FILE_TYPE                       Type;
        ULONG                           FileInformationClass;

    } QueryInformation;

    struct _Cleanup
    {
        FILE_TYPE                       Type;
        BOOLEAN                         IsModified;
    } Cleanup;

    struct _Close
    {
        FILE_TYPE                       Type;
        BOOLEAN                         IsModified;

    } Close;

    struct _Process
    {
        ULONG                           ParentProcessId;
        ULONG                           ProcessId;
        ULONG                           SessionId;

        ULONG                           CMDLineSize                 = 0;
        ULONG                           CMDLineOffset               = 0;
    } Process;
    
} MSG_PARAMETERS, *PMSG_PARAMETERS;

typedef struct _MSG_SEND_PACKET
{
    LONG                                EvtID                       = 0;
    ULONG                               MessageSize                 = 0;
    MSG_CATEGORY                        MessageCate                 = MSG_CATEGORY::MSG_CATE_NONE;
    ULONG                               MessageType;                // MSG_FILESYSTEM, MSG_FILESYSTEM_NOTIFY, MSG_PROCESS, MSG_REGISTRY
    LARGE_INTEGER                       MessageTime                 = { 0,0 };

    MSG_PARAMETERS                      Parameters;

    ULONG                               SrcFileFullPathSize         = 0;
    ULONG                               SrcFileFullPathOffset       = 0;
    ULONG                               DstFileFullPathSize         = 0;
    ULONG                               DstFileFullPathOffset       = 0;
    ULONG                               SolutionHDRSize             = 0;
    ULONG                               SolutionHDROffset           = 0;
    ULONG                               ContentsSize                = 0;
    ULONG                               ContentsOffset              = 0;

} MSG_SEND_PACKET, *PMSG_SEND_PACKET;

const unsigned int MSG_SEND_PACKET_SIZE = sizeof( MSG_SEND_PACKET );

enum class PACKET_RESULT
{
    RESULT_NONE             = 0x0,
    RESULT_ALLOW            = 0x1,
    RESULT_REJECT           = 0x2,
};

typedef struct _MSG_REPLY_PACKET
{
    LONG                                EvtID                       = 0;
    ULONG                               MessageSize                 = 0;

    PACKET_RESULT                       Result;

    MSG_PARAMETERS                      Parameters;

    ULONG                               SolutionHDRSize             = 0;
    ULONG                               SolutionHDROffset           = 0;
    ULONG                               ContentsSize                = 0;
    ULONG                               ContentsOffset              = 0;

} MSG_REPLY_PACKET, *PMSG_REPLY_PACKET;

const unsigned int MSG_REPLY_PACKET_SIZE = sizeof( MSG_REPLY_PACKET );

#endif // HDR_WINIOMONITOR_EVENT