#ifndef HDR_ISOLATION_DRIVER_MGMT
#define HDR_ISOLATION_DRIVER_MGMT

#include "fltBase.hpp"

#include "Cipher_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _ENCRYPT_CONTEXT
{
    // TODO: 향후 AES 암호화의 경우, AES_CONTEXT 를 키를 설정할 때 미리 계산하도록 하여,
    // 암복호화를 수행할 때 마다 생성하는 것을 하지 않도록 한다
    CIPHER_ID                       CipherID;

    ULONG                           IVSize;
    UCHAR                           IVKey[ 16 ];                // 16 * 8, 128bits
    ULONG                           KeySize;
    UCHAR                           EncryptionKey[ 32 ];        // max 256bits, 32 * 8

} ENCRYPT_CONTEXT;

typedef struct _FEATURE_CONTEXT
{
    LONG                            IsRunning;
    ULONG                           CntlProcessId;      // Connected Process Id

    LARGE_INTEGER                   TimeOutMs;

    ENCRYPT_CONTEXT                 EncryptContext[ CIPHER_MAX ];

} FEATURE_CONTEXT, *PFEATURE_CONTEXT;

extern FEATURE_CONTEXT FeatureContext;

NTSTATUS InitializeFeatureMgr();
NTSTATUS UninitializeFeatureMgr();

void SetTimeOutMs( __in ULONG TimeOutMs );

#endif // HDR_ISOLATION_DRIVER_MGMT