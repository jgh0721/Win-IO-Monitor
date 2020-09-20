#ifndef HDR_WINIOMONITOR_BUFFERMGR
#define HDR_WINIOMONITOR_BUFFERMGR

#include "fltBase.hpp"
#include "bufferMgr_Defs.hpp"
#include "contextMgr_Defs.hpp"
#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define POOL_FILENAME_SIZE      1024
#define POOL_PROCNAME_SIZE      1024
#define POOL_MSG_SEND_SIZE      2048
#define POOL_MSG_REPLY_SIZE     1024

NTSTATUS AllocateGenericBuffer( __inout TyGenericBuffer<VOID>* GenericBuffer, __in ULONG RequiredSize, __in PVOID LookasideList, __in ULONG PoolSize, __in ULONG PoolTag, _POOL_TYPE ePoolType = NonPagedPool );
void DeallocateGenericBuffer( __inout TyGenericBuffer<VOID>* GenericBuffer, __in PVOID LookasideList, _POOL_TYPE ePoolType = NonPagedPool );

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
        case BUFFER_IRPCONTEXT: {
            AllocateGenericBuffer( ( TyGenericBuffer<VOID>* ) & tyGenericBuffer, uRequiredSize,
                                   &GlobalContext.IrpContextLookasideList,
                                   sizeof( IRP_CONTEXT ), POOL_IRPCONTEXT_TAG );
        } break;
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
        default: ASSERT( false );
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
        case BUFFER_IRPCONTEXT: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.IrpContextLookasideList );
        } break;
        case BUFFER_FILENAME: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.FileNameLookasideList );
        } break;
        case BUFFER_PROCNAME: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.ProcNameLookasideList );
        } break;

        case BUFFER_SWAP_READ: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, NULLPTR );
        } break;
        case BUFFER_SWAP_READ_1024: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.SwapReadLookasideList_1024 );
        } break;
        case BUFFER_SWAP_READ_4096: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.SwapReadLookasideList_4096 );
        } break;
        case BUFFER_SWAP_READ_8192: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.SwapReadLookasideList_8192 );
        } break;
        case BUFFER_SWAP_READ_16384: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.SwapReadLookasideList_16384 );
        } break;
        case BUFFER_SWAP_READ_65536: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.SwapReadLookasideList_65536 );
        } break;

        case BUFFER_SWAP_WRITE: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, NULLPTR );
        } break;
        case BUFFER_SWAP_WRITE_1024: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.SwapWriteLookasideList_1024 );
        } break;
        case BUFFER_SWAP_WRITE_4096: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.SwapWriteLookasideList_4096 );
        } break;
        case BUFFER_SWAP_WRITE_8192: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.SwapWriteLookasideList_8192 );
        } break;
        case BUFFER_SWAP_WRITE_16384: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.SwapWriteLookasideList_16384 );
        } break;
        case BUFFER_SWAP_WRITE_65536: {
            DeallocateGenericBuffer( ( TyGenericBuffer<VOID>* )tyGenericBuffer, &GlobalContext.SwapWriteLookasideList_65536 );
        } break;

        default: ASSERT( false );
    }
}

template< typename T >
TyGenericBuffer<T> CloneBuffer( TyGenericBuffer<T>* tyGenericBuffer )
{
    TyGenericBuffer<T> ret;
    RtlZeroMemory( &ret, sizeof( TyGenericBuffer<T> ) );

    if( tyGenericBuffer == NULLPTR )
        return ret;

    ret = AllocateBuffer<T>( tyGenericBuffer->BufferType, tyGenericBuffer->BufferSize );
    if( ret.Buffer == NULLPTR )
        return ret;

    RtlCopyMemory( ret.Buffer, tyGenericBuffer->Buffer, ret.BufferSize );

    return ret;
}

TyGenericBuffer<BYTE> AllocateSwapBuffer( TyEnBufferType eBufferType, ULONG uRequiredSize );


#endif // HDR_WINIOMONITOR_BUFFERMGR