#ifndef HDR_WINIOISOLATION_EVENT
#define HDR_WINIOISOLATION_EVENT

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#ifndef CONTAINOR_SUFFIX_MAX
#define CONTAINOR_SUFFIX_MAX 12
#endif

typedef enum _ENCRYPTION_METHOD
{
    ENCRYPTION_NONE,
    ENCRYPTION_XOR,
    ENCRYPTION_XTEA,
    ENCRYPTION_AES128,
    ENCRYPTION_AES256,
    ENCRYPTION_MAX = ENCRYPTION_AES256
    
} ENCRYPTION_METHOD, *PENCRYPTION_METHOD;

typedef struct _ENCRYPT_CONFIG
{
    ENCRYPTION_METHOD                       Method;

    ULONG                                   KeySize;
    UCHAR                                   EncryptionKey[ 32 ];

} ENCRYPT_CONFIG;

typedef struct
{
    ULONG                                   SizeOfStruct;
    // 이벤트 전송 후 대기 시간(ms)
    ULONG                                   TimeOutMs;

    ULONG                                   EncryptConfigCount;
    ENCRYPT_CONFIG                          EncryptConfig[ ENCRYPTION_MAX ];

    ULONG                                   LengthOfStubCodeX86;         // BYTE
    ULONG                                   OffsetOfStubCodeX86;         // BYTE
    ULONG                                   LengthOfStubCodeX64;         // BYTE
    ULONG                                   OffsetOfStubCodeX64;         // BYTE

} DRIVER_CONFIG;

typedef enum _MSG_CATEGORY
{
    MSG_CATE_FILESYSTEM                     = 0x1,
    MSG_CATE_FILESYSTEM_NOTIFY              = 0x2,
    MSG_CATE_PROCESS                        = 0x4,
} MSG_CATEGORY;

typedef enum _MSG_FS_TYPE
{
    FS_PRE_CREATE                           = 0x00010000,
    FS_POST_CREATE                          = 0x00020000,

    FS_PRE_QUERY_INFORMATION                = 0x00040000,
    FS_POST_QUERY_INFORMATION               = 0x00080000,

    FS_PRE_SET_INFORMATION                  = 0x00100000,
    FS_POST_SET_INFORMATION                 = 0x00200000,

    FS_PRE_DIRECTORY_CONTROL                = 0x00400000,
    FS_POST_DIRECTORY_CONTROL               = 0x00800000,

    FS_PRE_CLEANUP                          = 0x01000000,
    FS_POST_CLEANUP                         = 0x02000000,

    FS_PRE_CLOSE                            = 0x04000000,
    FS_POST_CLOSE                           = 0x08000000,
    
} MSG_FS_TYPE;

typedef enum _MSG_FS_NOTIFY_TYPE
{
    FILE_WAS_CREATED                        = 0x00000020,
    FILE_WAS_WRITTEN                        = 0x00000040,
    FILE_WAS_RENAMED                        = 0x00000080,
    FILE_WAS_DELETED                        = 0x00000100,
    FILE_SECURITY_CHANGED                   = 0x00000200,
    FILE_INFO_CHANGED                       = 0x00000400,
    FILE_WAS_READ                           = 0x00000800,

} MSG_FS_NOTIFY_TYPE;

typedef enum _MSG_PROC_TYPE
{
    PROC_WAS_CREATED                        = 0x00000001,
    PROC_WAS_TERMINATED                     = 0x00000002
} MSG_PROC_TYPE;

typedef union TyMsgParameters
{
    struct
    {
        ACCESS_MASK                         DesiredAccess;
        ULONG                               FileAttributes;
        ULONG                               ShareAccess;
        ULONG                               CreateDisposition;
        ULONG                               CreateOptions;

        BOOLEAN                             IsAlreadyExists;
        BOOLEAN                             IsContainSolutionMetaData;
    } Create;

    struct
    {
        BOOLEAN                             IsCreateMetaData;
        BOOLEAN                             IsApplyToNameChanger;
        BOOLEAN                             IsEncrypt;

        ENCRYPTION_METHOD                   EncryptionMethod;
        WCHAR                               NameChangeSuffix[ CONTAINOR_SUFFIX_MAX ];

    } CreateResult;

    struct
    {
        // caller process has rights for read/write decrypted data on encrypted file
        BOOLEAN                             IsAccessEncryption;
        BOOLEAN                             IsUpdateMetaData;

    } OpenResult;

} MSG_PARAMETERS, *PMSG_PARAMETERS;

const unsigned int MSG_PARAMETERS_SIZE = sizeof( MSG_PARAMETERS );

typedef struct _MSG_SEND_PACKET
{
    ULONG                                   MessageSize;
    MSG_CATEGORY                            MessageCate;
    ULONG                                   MessageType;
    LARGE_INTEGER                           EventTime;

    ULONG                                   ProcessId;

    MSG_PARAMETERS                          Parameters;

    ULONG                                   LengthOfProcessFullPath;  // BYTE    
    ULONG                                   OffsetOfProcessFullPath;  // BYTE
    ULONG                                   LengthOfSrcFileFullPath;  // BYTE
    ULONG                                   OffsetOfSrcFileFullPath;  // BYTE
    ULONG                                   LengthOfDstFileFullPath;  // BYTE
    ULONG                                   OffsetOfDstFileFullPath;  // BYTE
    ULONG                                   LengthOfContents;         // BYTE
    ULONG                                   OffsetOfContents;         // BYTE

} MSG_SEND_PACKET, *PMSG_SEND_PACKET;

const unsigned int MSG_SEND_PACKET_SIZE = sizeof( MSG_SEND_PACKET );

typedef struct _MSG_REPLY_PACKET
{
    // If received msg was MSG_CATE_FILESYSTEM_NOTIFY, this status was ignored
    // STATUS_SUCCESS, STATUS_ACCESS_DENIED, ... 
    NTSTATUS                                Status;

    MSG_PARAMETERS                          Parameters;

    BYTE                                    SolutionMetaData[ 1 ];    // BYTE ARRAY

} MSG_REPLY_PACKET, *PMSG_REPLY_PACKET;

const unsigned int MSG_REPLY_PACKET_SIZE = sizeof( MSG_REPLY_PACKET );

#endif // HDR_WINIOISOLATION_EVENT