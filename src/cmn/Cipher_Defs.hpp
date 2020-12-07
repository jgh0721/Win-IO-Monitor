#ifndef HDR_CIPHER_DEFS
#define HDR_CIPHER_DEFS

#if defined(USE_ON_KERNEL)
#include "fltBase.hpp"
#else
#include "iMonFSD_API.hpp"
#include <cassert>
#include <assert.h>
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef enum _CIPHER_ID
{
    CIPHER_NONE             = 0,
    CIPHER_XOR              = 1,
    CIPHER_XTEA             = 2,
    CIPHER_AES128_ECB       = 3,
    CIPHER_AES256_ECB       = 4,

    CIPHER_MAX              = CIPHER_AES256_ECB,
} CIPHER_ID;

#endif // HDR_CIPHER_DEFS