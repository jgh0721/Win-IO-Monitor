#ifndef HDR_WINIOMONITOR_BUFFERMGR_DEFS
#define HDR_WINIOMONITOR_BUFFERMGR_DEFS

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

const int BUFFER_SWAP_READ_1024_SIZE = 1024;
const int BUFFER_SWAP_READ_4096_SIZE = 4096;
const int BUFFER_SWAP_READ_8192_SIZE = 8192;
const int BUFFER_SWAP_READ_16384_SIZE = 16384;
const int BUFFER_SWAP_READ_65536_SIZE = 65536;

const int BUFFER_SWAP_WRITE_1024_SIZE = 1024;
const int BUFFER_SWAP_WRITE_4096_SIZE = 4096;
const int BUFFER_SWAP_WRITE_8192_SIZE = 8192;
const int BUFFER_SWAP_WRITE_16384_SIZE = 16384;
const int BUFFER_SWAP_WRITE_65536_SIZE = 65536;

enum TyEnBufferType
{
    BUFFER_UNKNOWN,
    BUFFER_IRPCONTEXT,

    BUFFER_FILENAME,
    BUFFER_PROCNAME,

    BUFFER_MSG_SEND,
    BUFFER_MSG_REPLY,

    ///////////////////////////////////////////////////////////////////////////

    BUFFER_SWAP_READ            = 100,
    BUFFER_SWAP_READ_1024,      // 직접 지정하지 말 것
    BUFFER_SWAP_READ_4096,      // 직접 지정하지 말 것
    BUFFER_SWAP_READ_8192,      // 직접 지정하지 말 것
    BUFFER_SWAP_READ_16384,     // 직접 지정하지 말 것
    BUFFER_SWAP_READ_65536,     // 직접 지정하지 말 것

    BUFFER_SWAP_WRITE           = 200,
    BUFFER_SWAP_WRITE_1024,     // 직접 지정하지 말 것
    BUFFER_SWAP_WRITE_4096,     // 직접 지정하지 말 것
    BUFFER_SWAP_WRITE_8192,     // 직접 지정하지 말 것
    BUFFER_SWAP_WRITE_16384,    // 직접 지정하지 말 것
    BUFFER_SWAP_WRITE_65536,    // 직접 지정하지 말 것
};

struct TyBaseBuffer
{
    TyEnBufferType      BufferType;
    BOOLEAN             IsAllocatedFromLookasideList;
    ULONG               BufferSize;

    TyBaseBuffer() : BufferType( BUFFER_UNKNOWN )
        , IsAllocatedFromLookasideList( FALSE )
        , BufferSize( 0 )
    {
    }
};

template< typename T >
struct TyGenericBuffer : public TyBaseBuffer
{
    T* Buffer;

    TyGenericBuffer<T>() : TyBaseBuffer(), Buffer( NULLPTR )
    {
    }

};


#endif // HDR_WINIOMONITOR_BUFFERMGR_DEFS