#ifndef HDR_CIPHER_AES
#define HDR_CIPHER_AES

#include "Cipher_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsDetail
{
    typedef struct
    {
        uint32_t erk[ 64 ];     /* encryption round keys */
        uint32_t drk[ 64 ];     /* decryption round keys */
        int nr;                 /* number of rounds */
    } aes_context;

    int  aes_set_key( aes_context* ctx, uint8_t* key, int nbits );
    void aes_encrypt_ecb( aes_context* ctx, uint8_t input[ 16 ], uint8_t output[ 16 ] );
    void aes_decrypt_ecb( aes_context* ctx, uint8_t input[ 16 ], uint8_t output[ 16 ] );

}

void EncryptBufferByAES( __in LONGLONG Offset, __in PVOID Buffer, __in ULONG BufferSize,
                         __in_opt PVOID Key, __in_opt ULONG KeyLength,
                         __in_opt PVOID IV, __in_opt ULONG IVLength );

void DecryptBufferByAES( __in LONGLONG Offset, __in PVOID Buffer, __in ULONG BufferSize,
                         __in_opt PVOID Key, __in_opt ULONG KeyLength,
                         __in_opt PVOID IV, __in_opt ULONG IVLength );



#endif // HDR_CIPHER_AES