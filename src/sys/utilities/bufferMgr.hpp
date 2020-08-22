#ifndef HDR_WINIOMONITOR_BUFFERMGR
#define HDR_WINIOMONITOR_BUFFERMGR

#include "fltBase.hpp"

#include "contextMgr_Defs.hpp"

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

#define POOL_FILENAME_TAG   'fnTG'
#define POOL_PROCNAME_TAG   'pnTG'
#define POOL_MSG_SEND_TAG   'snTG'
#define POOL_MSG_REPLY_TAG  'rpTG'

#define POOL_FILENAME_SIZE      1024
#define POOL_PROCNAME_SIZE      1024
#define POOL_MSG_SEND_SIZE      2048
#define POOL_MSG_REPLY_SIZE     1024

struct TyBaseBuffer
{
    TyEnBufferType      BufferType;
    BOOLEAN             IsAllocatedFromLookasideList;
    ULONG               BufferSize;
};

template< typename T >
struct TyGenericBuffer : public TyBaseBuffer
{
    T*                  Buffer;
};

NTSTATUS AllocateGenericBuffer( __inout TyGenericBuffer<VOID>* GenericBuffer, __in ULONG RequiredSize, __in PNPAGED_LOOKASIDE_LIST LookasideList, __in ULONG PoolSize, __in ULONG PoolTag );
void DeallocateGenericBuffer( __inout TyGenericBuffer<VOID>* GenericBuffer, __in PNPAGED_LOOKASIDE_LIST LookasideList );

template< typename T >
TyGenericBuffer<T> AllocateBuffer( TyEnBufferType eBufferType, ULONG uRequiredSize = 0 )
{
    TyGenericBuffer<T> tyGenericBuffer;

    RtlZeroMemory( &tyGenericBuffer, sizeof( TyGenericBuffer<T> ) );

    tyGenericBuffer.BufferType = eBufferType;
    tyGenericBuffer.IsAllocatedFromLookasideList = FALSE;
    tyGenericBuffer.Buffer = NULLPTR;
    tyGenericBuffer.BufferSize = 0;

    switch( eBufferType )
    {
        case BUFFER_FILENAME: {
            AllocateGenericBuffer( (TyGenericBuffer<VOID>*)&tyGenericBuffer, uRequiredSize, 
                                   &GlobalContext.FileNameLookasideList, 
                                   POOL_FILENAME_SIZE, POOL_FILENAME_TAG );
        } break;
        case BUFFER_PROCNAME: {
            AllocateGenericBuffer( ( TyGenericBuffer<VOID>* ) & tyGenericBuffer, uRequiredSize, 
                                   &GlobalContext.ProcNameLookasideList, 
                                   POOL_PROCNAME_SIZE, POOL_PROCNAME_TAG );
        } break;
    }

    return tyGenericBuffer;
}

template< typename T >
void DeallocateBuffer( TyGenericBuffer<T>* tyGenericBuffer )
{
    ASSERT( tyGenericBuffer != nullptr );
    if( tyGenericBuffer == NULL || tyGenericBuffer->buffer == NULL )
        return;

    switch( tyGenericBuffer->eBufferType )
    {
        case BUFFER_FILENAME: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.FileNameLookasideList );
        } break;
        case BUFFER_PROCNAME: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.ProcNameLookasideList );
        } break;
    }
}

#endif // HDR_WINIOMONITOR_BUFFERMGR