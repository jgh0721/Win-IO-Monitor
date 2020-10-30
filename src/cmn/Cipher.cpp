#include "Cipher.hpp"

#include "Cipher_XTEA.hpp"
#include "Cipher_AES.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

void EncryptBuffer( CIPHER_ID CipherId, LONGLONG Offset, PVOID Buffer, ULONG BufferSize, PVOID Key, ULONG KeyLength, PVOID IV, ULONG IVLength )
{
	ASSERT( CipherId <= CIPHER_MAX );

	switch( CipherId )
	{
		case CIPHER_XOR:
			break;
		case CIPHER_XTEA:
			EncryptBufferByXTEA( Buffer, BufferSize, Key, KeyLength );
			break;
		case CIPHER_AES128_ECB:
		case CIPHER_AES256_ECB:
			EncryptBufferByAES( Offset, Buffer, BufferSize, Key, KeyLength, IV, IVLength );
			break;
	}
}

void DecryptBuffer( CIPHER_ID CipherId, LONGLONG Offset, PVOID Buffer, ULONG BufferSize, PVOID Key, ULONG KeyLength, PVOID IV, ULONG IVLength )
{
	ASSERT( CipherId <= CIPHER_MAX );

	switch( CipherId )
	{
		case CIPHER_XOR:
			break;
		case CIPHER_XTEA:
			DecryptBufferByXTEA( Buffer, BufferSize, Key, KeyLength );
			break;
		case CIPHER_AES128_ECB:
		case CIPHER_AES256_ECB:
			DecryptBufferByAES( Offset, Buffer, BufferSize, Key, KeyLength, IV, IVLength );
			break;
	}
}
