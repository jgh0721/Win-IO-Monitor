#ifndef HDR_WINIOISOLATION_EVENT
#define HDR_WINIOISOLATION_EVENT

#if defined(USE_ON_KERNEL )
#include "fltBase.hpp"
#else
#include <Rpc.h>
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#ifndef CONTAINOR_SUFFIX_MAX
#define CONTAINOR_SUFFIX_MAX 12
#endif

#ifndef MAX_SOLUTION_METADATA_SIZE
#define MAX_SOLUTION_METADATA_SIZE 16384
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

    ULONG                                   IVSize;
    UCHAR                                   IVKey[ 16 ];                // 16 * 8, 128bits
    ULONG                                   KeySize;
    UCHAR                                   EncryptionKey[ 32 ];        // max 256bits, 32 * 8

} ENCRYPT_CONFIG;

typedef struct _DRIVER_CONFIG
{
    ULONG                                   SizeOfStruct;
    // 이벤트 전송 후 대기 시간(ms)
    ULONG                                   TimeOutMs;

    ULONG                                   EncryptConfigCount;
    ENCRYPT_CONFIG                          EncryptConfig[ ENCRYPTION_MAX ];

    ULONG                                   LengthOfStubCodeX86;        // BYTE
    ULONG                                   OffsetOfStubCodeX86;        // BYTE
    ULONG                                   LengthOfStubCodeX64;        // BYTE
    ULONG                                   OffsetOfStubCodeX64;        // BYTE

    ULONG                                   SolutionMetaDataSize;       // BYTE, Must be multiply by Sector Size( Generally, Sector size is 512 )           

} DRIVER_CONFIG;

typedef struct _USER_FILE_SOLUTION_DATA
{
    ULONG                                   SizeOfStruct;

    ULONG                                   LengthOfFileName;           // BYTE
    ULONG                                   OffsetOfFileName;

    ULONG                                   LengthOfSolutionData;       // BYTE
    ULONG                                   OffsetOfSolutionData;
    
} USER_FILE_SOLUTION_DATA, *PUSER_SOLUTION_DATA;

typedef struct _USER_FILE_ENCRYPT
{
    ULONG                                   SizeOfStruct;

    ENCRYPT_CONFIG                          EncryptConfig;
    BOOLEAN                                 IsUseStubCodeX86;
    BOOLEAN                                 IsUseStubCodeX64;
    // 반드시 . 으로 시작
    WCHAR                                   ContainorSuffix[ CONTAINOR_SUFFIX_MAX ];

    ULONG                                   LengthOfSrcFileFullPath;
    ULONG                                   OffsetOfSrcFileFullPath;
    ULONG                                   LengthOfDstFileFullPath;
    ULONG                                   OffsetOfDstFileFullPath;
    ULONG                                   LengthOfSolutionData;
    ULONG                                   OffsetOfSolutionData;
    
} USER_FILE_ENCRYPT, *PUSER_FILE_ENCRYPT;

typedef enum _MSG_CATEGORY
{
    MSG_CATE_FILESYSTEM                     = 0x1,
    MSG_CATE_FILESYSTEM_NOTIFY              = 0x2,
    MSG_CATE_PROCESS                        = 0x4,
    MSG_CATE_DEVICE                         = 0x8,
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

typedef enum _MSG_DEVICE_TYPE
{
    DEV_VOLUME_MOUNT                         = 0x00000004,
    DEV_VOLUME_DISMOUNT                      = 0x00000008
} MSG_VOLUME_TYPE;

typedef enum
{
    PROCESS_NOTIFY_CREATION_TERMINATION = 0x1,
    PROCESS_DENY_CREATION = 0x2,
    PROCESS_DENY_TERMINATION = 0x4,
    PROCESS_APPLY_CHILD_PROCESS = 0x8

} PROCESS_FILTER_TYPE;

typedef struct _USER_GLOBAL_FILTER
{
    WCHAR                                   FilterMask[ MAX_PATH ];
    BOOLEAN                                 IsInclude;

} USER_GLOBAL_FILTER, *PUSER_GLOBAL_FILTER;

typedef struct _USER_PROCESS_FILTER
{
    UUID                                    Id;
    ULONG                                   ProcessId;
    WCHAR                                   ProcessFilterMask[ MAX_PATH ];
    ULONG                                   ProcessFilter;      // PROCESS_FILTER_TYPE 의 조합

} USER_PROCESS_FILTER, * PUSER_PROCESS_FILTER;

typedef struct _USER_PROCESS_FILTER_ENTRY
{
    UUID                                    Id;
    MSG_CATEGORY                            FilterCategory;
    ULONGLONG                               FilterType;         // MSG_FS_TYPE or MSG_FS_NOTIFY_TYPE or MSG_PROC_TYPE
    WCHAR                                   FilterMask[ MAX_PATH ];
    BOOLEAN                                 IsManagedFile;      // 만약, FilterMask 가 빈 문자열이고, 이 값이 TRUE 라면, 파일이름 / 확장자 등에 상관없이 관리되는 파일( 암호화된 파일 ) 이 설정된다
    BOOLEAN                                 IsInclude;          // TRUE 포함, FALSE 예외

} USER_PROCESS_FILTER_ENTRY, * PUSER_PROCESS_FILTER_ENTRY;

typedef union TyMsgParameters
{
    struct _Create
    {
        ACCESS_MASK                         DesiredAccess;
        ULONG                               FileAttributes;
        ULONG                               ShareAccess;
        ULONG                               CreateDisposition;
        ULONG                               CreateOptions;

        BOOLEAN                             IsAlreadyExists;
        BOOLEAN                             IsContainSolutionMetaData;
        ULONG                               SolutionMetaDataSize;
        BYTE                                SolutionMetaData[ MAX_SOLUTION_METADATA_SIZE ];
    } Create;

    struct _CreateResult
    {
        // managed by this driver
        BOOLEAN                             IsUseIsolation;         

        BOOLEAN                             IsUseContainor;
        WCHAR                               NameChangeSuffix[ CONTAINOR_SUFFIX_MAX ];

        // only apply if this file is not exists
        BOOLEAN                             IsUseEncryption;
        BOOLEAN                             IsUseGlobalEncryption;
        // if IsUseGlobalEncryption was TRUE, only use EncryptConfig.Method
        ENCRYPT_CONFIG                      EncryptConfig;
        // only apply if this file is already exists
        // caller process has rights for read/write decrypted data on encrypted file
        BOOLEAN                             IsAccessEncryption;

        BOOLEAN                             IsUseSolutionMetaData;
        ULONG                               SolutionMetaDataSize;
        BYTE                                SolutionMetaData[ MAX_SOLUTION_METADATA_SIZE ];
    } CreateResult;

    struct _SetFileInformation
    {
        ULONG                               FileInformationClass;
    } SetFileInformation;

    struct _Clean
    {
        BOOLEAN                             IsModified;
    } Clean;

    struct _Close
    {
        BOOLEAN                             IsModified;
    } Close;

    struct _Process
    {
        BOOLEAN                             IsCreate;
        ULONG                               ParentProcessId;
        ULONG                               ProcessId;
        ULONG                               SessionId;
        // NOTE: 프로세스의 명령행 정보는 패킷의 Contents 로 포함되어 전송
    } Process;

} MSG_PARAMETERS, *PMSG_PARAMETERS;

const unsigned int MSG_PARAMETERS_SIZE = sizeof( MSG_PARAMETERS );

typedef struct _MSG_SEND_PACKET
{
    ULONG                                   MessageSize;
    MSG_CATEGORY                            MessageCate;
    ULONG                                   MessageType;
    LARGE_INTEGER                           EventTime;

    ULONG                                   ProcessId;
    ULONG                                   ThreadId;

    MSG_PARAMETERS                          Parameters;
    ULONG                                   FileType;                   // 0 = Normal, 

    ULONG                                   LengthOfProcessFullPath;    // BYTE    
    ULONG                                   OffsetOfProcessFullPath;    // BYTE
    ULONG                                   LengthOfSrcFileFullPath;    // BYTE
    ULONG                                   OffsetOfSrcFileFullPath;    // BYTE
    ULONG                                   LengthOfDstFileFullPath;    // BYTE
    ULONG                                   OffsetOfDstFileFullPath;    // BYTE
    ULONG                                   LengthOfContents;           // BYTE
    ULONG                                   OffsetOfContents;           // BYTE

} MSG_SEND_PACKET, *PMSG_SEND_PACKET;

#ifndef USE_ON_KERNEL
typedef struct _MSG_SEND_PACKETU
{
    FILTER_MESSAGE_HEADER                   MessageHDR;
    MSG_SEND_PACKET                         MessageBody;

} MSG_SEND_PACKETU, *PMSG_SEND_PACKETU;
#endif

const unsigned int MSG_SEND_PACKET_SIZE = sizeof( MSG_SEND_PACKET );

typedef enum _MSG_JUDGE_RESULT
{
    MSG_JUDGE_NONE                          = 0x00000000,
    MSG_JUDGE_ALLOW                         = 0x00000001,
    MSG_JUDGE_REJECT                        = 0x00000002,
    MSG_JUDGE_APPLY_ACCESS                  = 0x00000004,       // CreateResult.IsAccessEncryption

} MSG_JUDGE_RESULT;

typedef struct _MSG_REPLY_PACKET
{
    // MSG_JUDGE_RESULT
    ULONG                                   Result;
    // If received msg was MSG_CATE_FILESYSTEM_NOTIFY, this status was ignored
    // STATUS_SUCCESS, STATUS_ACCESS_DENIED, ...
    // this value pass down to OS
    NTSTATUS                                Status;

    MSG_PARAMETERS                          Parameters;

} MSG_REPLY_PACKET, *PMSG_REPLY_PACKET;

#ifndef USE_ON_KERNEL
typedef struct _MSG_REPLY_PACKETU
{
    FILTER_REPLY_HEADER                     ReplyHDR;
    MSG_REPLY_PACKET                        ReplyBody;

} MSG_REPLY_PACKETU, *PMSG_REPLY_PACKETU;
#endif

const unsigned int MSG_REPLY_PACKET_SIZE = sizeof( MSG_REPLY_PACKET );

#endif // HDR_WINIOISOLATION_EVENT