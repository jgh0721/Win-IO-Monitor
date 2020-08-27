#ifndef HDR_WINIOMONITOR_BUFFERMGR_DEFS
#define HDR_WINIOMONITOR_BUFFERMGR_DEFS

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

enum TyEnBufferType
{
    BUFFER_UNKNOWN,
    BUFFER_FILENAME,
    BUFFER_PROCNAME,

    BUFFER_MSG_SEND,
    BUFFER_MSG_REPLY
};

struct TyBaseBuffer
{
    TyEnBufferType      BufferType;
    BOOLEAN             IsAllocatedFromLookasideList;
    ULONG               BufferSize;
};

template< typename T >
struct TyGenericBuffer : public TyBaseBuffer
{
    T* Buffer;
};


#endif // HDR_WINIOMONITOR_BUFFERMGR_DEFS