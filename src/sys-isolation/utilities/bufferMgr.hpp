#ifndef HDR_WINIOMONITOR_BUFFERMGR
#define HDR_WINIOMONITOR_BUFFERMGR

#include "fltBase.hpp"
#include "bufferMgr_Defs.hpp"
#include "contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define POOL_FILENAME_SIZE      1024
#define POOL_PROCNAME_SIZE      1024
#define POOL_MSG_SEND_SIZE      2048
#define POOL_MSG_REPLY_SIZE     1024

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
    if( tyGenericBuffer == NULL || tyGenericBuffer->Buffer == NULL )
        return;

    switch( tyGenericBuffer->BufferType )
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