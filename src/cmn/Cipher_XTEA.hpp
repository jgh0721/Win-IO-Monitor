#ifndef HDR_CIPHER_XTEA
#define HDR_CIPHER_XTEA

#include "Cipher_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

void EncryptBufferByXTEA( __in PVOID Buffer, __in ULONG BufferSize,
                          __in_opt PVOID Key, __in_opt ULONG KeyLength );

void DecryptBufferByXTEA( __in PVOID Buffer, __in ULONG BufferSize,
                          __in_opt PVOID Key, __in_opt ULONG KeyLength );



#endif // HDR_CIPHER_XTEA