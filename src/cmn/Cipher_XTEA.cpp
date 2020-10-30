#include "Cipher_XTEA.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

void encipher( unsigned int num_rounds, uint32_t v[ 2 ], uint32_t const key[ 4 ] )
{
	unsigned int i;
	uint32_t v0 = v[ 0 ], v1 = v[ 1 ], sum = 0, delta = 0x9E3779B9;
	for( i = 0; i < num_rounds; i++ )
	{
		v0 += ( ( ( v1 << 4 ) ^ ( v1 >> 5 ) ) + v1 ) ^ ( sum + key[ sum & 3 ] );
		sum += delta;
		v1 += ( ( ( v0 << 4 ) ^ ( v0 >> 5 ) ) + v0 ) ^ ( sum + key[ ( sum >> 11 ) & 3 ] );
	}
	v[ 0 ] = v0; v[ 1 ] = v1;
}

void decipher( unsigned int num_rounds, uint32_t v[ 2 ], uint32_t const key[ 4 ] )
{
	unsigned int i;
	uint32_t v0 = v[ 0 ], v1 = v[ 1 ], delta = 0x9E3779B9, sum = delta * num_rounds;
	for( i = 0; i < num_rounds; i++ )
	{
		v1 -= ( ( ( v0 << 4 ) ^ ( v0 >> 5 ) ) + v0 ) ^ ( sum + key[ ( sum >> 11 ) & 3 ] );
		sum -= delta;
		v0 -= ( ( ( v1 << 4 ) ^ ( v1 >> 5 ) ) + v1 ) ^ ( sum + key[ sum & 3 ] );
	}
	v[ 0 ] = v0; v[ 1 ] = v1;
}

void EncryptBufferByXTEA( PVOID Buffer, ULONG BufferSize, PVOID Key, ULONG KeyLength )
{
	// TODO: add ASSERT
	int count = BufferSize / 8;

    for( auto idx = 0; idx < count; ++idx )
		encipher( 1, (uint32_t*)(Add2Ptr( Buffer, idx * 8 )), ( uint32_t* )Key );
}

void DecryptBufferByXTEA( PVOID Buffer, ULONG BufferSize, PVOID Key, ULONG KeyLength )
{
	// TODO: add ASSERT
	int count = BufferSize / 8;

	for( auto idx = 0; idx < count; ++idx )
		decipher( 1, ( uint32_t* )( Add2Ptr( Buffer, idx * 8 ) ), ( uint32_t* )Key );
}
