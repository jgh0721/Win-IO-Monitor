#ifndef HDR_CIPHER
#define HDR_CIPHER

#if defined(USE_ON_KERNEL)
#include "fltBase.hpp"
#endif

#include "Cipher_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

void EncryptBuffer( __in CIPHER_ID CipherId, __in LONGLONG Offset, __in PVOID Buffer, __in ULONG BufferSize,
                    __in_opt PVOID Key, __in_opt ULONG KeyLength,
                    __in_opt PVOID IV, __in_opt ULONG IVLength );


void DecryptBuffer( __in CIPHER_ID CipherId, __in LONGLONG Offset, __in PVOID Buffer, __in ULONG BufferSize,
                    __in_opt PVOID Key, __in_opt ULONG KeyLength,
                    __in_opt PVOID IV, __in_opt ULONG IVLength );

#endif // HDR_CIPHER