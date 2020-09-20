#include "bufferMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS AllocateGenericBuffer( TyGenericBuffer<VOID>* GenericBuffer, ULONG RequiredSize, __in PVOID LookasideList, ULONG PoolSize, ULONG PoolTag, _POOL_TYPE ePoolType /* = NonPagedPool */ )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT( GenericBuffer != NULLPTR );
    ASSERT( LookasideList != NULLPTR );
    ASSERT( PoolSize > 0 );

    do
    {
        if( RequiredSize == 0 || RequiredSize <= PoolSize )
        {
            GenericBuffer->IsAllocatedFromLookasideList = TRUE;
            GenericBuffer->Buffer = ePoolType == NonPagedPool ? ExAllocateFromNPagedLookasideList( ( PNPAGED_LOOKASIDE_LIST )LookasideList ) : ExAllocateFromPagedLookasideList( ( PPAGED_LOOKASIDE_LIST )LookasideList );
            GenericBuffer->BufferSize = PoolSize;
        }
        else
        {
            GenericBuffer->IsAllocatedFromLookasideList = FALSE;
            GenericBuffer->Buffer = ExAllocatePoolWithTag( ePoolType, RequiredSize, PoolTag );
            GenericBuffer->BufferSize = RequiredSize;
        }

        if( GenericBuffer->Buffer == NULLPTR )
        {
            RtlZeroMemory( GenericBuffer, sizeof( TyGenericBuffer<VOID> ) );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        if( GenericBuffer->Buffer != NULLPTR )
            RtlZeroMemory( GenericBuffer->Buffer, GenericBuffer->BufferSize );

    } while( false );

    return Status;
}

void DeallocateGenericBuffer( TyGenericBuffer<void>* GenericBuffer, PVOID LookasideList, _POOL_TYPE ePoolType /* = NonPagedPool */ )
{
    ASSERT( GenericBuffer != NULLPTR );
    ASSERT( LookasideList != NULLPTR );

    if( GenericBuffer->IsAllocatedFromLookasideList != FALSE )
    {
        if( ePoolType == NonPagedPool )
            ExFreeToNPagedLookasideList( ( PNPAGED_LOOKASIDE_LIST )LookasideList, GenericBuffer->Buffer );
        else
            ExFreeToPagedLookasideList( ( PPAGED_LOOKASIDE_LIST )LookasideList, GenericBuffer->Buffer );
    }
    else
    {
        ExFreePool( GenericBuffer->Buffer );
    }

    GenericBuffer->Buffer = NULLPTR;
    GenericBuffer->BufferSize = 0;
}

TyGenericBuffer<BYTE> AllocateSwapBuffer( TyEnBufferType eBufferType, ULONG uRequiredSize )
{
    TyGenericBuffer<BYTE> tyGenericBuffer;
    RtlZeroMemory( &tyGenericBuffer, sizeof( TyGenericBuffer<BYTE> ) );

    ASSERT( eBufferType == BUFFER_SWAP_READ || eBufferType == BUFFER_SWAP_WRITE );
    if( eBufferType != BUFFER_SWAP_READ && eBufferType != BUFFER_SWAP_WRITE )
        return tyGenericBuffer;

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG uPoolSize = 0;
    PPAGED_LOOKASIDE_LIST pagedLookasideList = NULLPTR;
    TyEnBufferType eAllocateType = eBufferType;
    ULONG poolTag = 0;

    switch( eBufferType )
    {
        case BUFFER_SWAP_READ: {
            poolTag = POOL_READ_TAG;

            if( uRequiredSize <= BUFFER_SWAP_READ_1024_SIZE )
            {
                uPoolSize = BUFFER_SWAP_READ_1024_SIZE;
                pagedLookasideList = &GlobalContext.SwapReadLookasideList_1024;
                eAllocateType = BUFFER_SWAP_READ_1024;
            }
            else if( uRequiredSize <= BUFFER_SWAP_READ_4096_SIZE )
            {
                uPoolSize = BUFFER_SWAP_READ_4096_SIZE;
                pagedLookasideList = &GlobalContext.SwapReadLookasideList_4096;
                eAllocateType = BUFFER_SWAP_READ_4096;
            }
            else if( uRequiredSize <= BUFFER_SWAP_READ_8192_SIZE )
            {
                uPoolSize = BUFFER_SWAP_READ_8192_SIZE;
                pagedLookasideList = &GlobalContext.SwapReadLookasideList_8192;
                eAllocateType = BUFFER_SWAP_READ_8192;
            }
            else if( uRequiredSize <= BUFFER_SWAP_READ_16384_SIZE )
            {
                uPoolSize = BUFFER_SWAP_READ_16384_SIZE;
                pagedLookasideList = &GlobalContext.SwapReadLookasideList_16384;
                eAllocateType = BUFFER_SWAP_READ_16384;
            }
            else if( uRequiredSize <= BUFFER_SWAP_READ_65536_SIZE )
            {
                uPoolSize = BUFFER_SWAP_READ_65536_SIZE;
                pagedLookasideList = &GlobalContext.SwapReadLookasideList_65536;
                eAllocateType = BUFFER_SWAP_READ_65536;
            }
            else
            {
                uPoolSize = 1;  // 밑의 함수에서 uPoolSize 의 최소값을 지정해야한다 
                eAllocateType = BUFFER_SWAP_READ;
            }

            ASSERT( uPoolSize > 0 );

        } break;
        case BUFFER_SWAP_WRITE: {
            poolTag = POOL_WRITE_TAG;

            if( uRequiredSize <= BUFFER_SWAP_WRITE_1024_SIZE )
            {
                uPoolSize = BUFFER_SWAP_WRITE_1024_SIZE;
                pagedLookasideList = &GlobalContext.SwapWriteLookasideList_1024;
                eAllocateType = BUFFER_SWAP_READ_1024;
            }
            else if( uRequiredSize <= BUFFER_SWAP_WRITE_4096_SIZE )
            {
                uPoolSize = BUFFER_SWAP_WRITE_4096_SIZE;
                pagedLookasideList = &GlobalContext.SwapWriteLookasideList_4096;
                eAllocateType = BUFFER_SWAP_READ_4096;
            }
            else if( uRequiredSize <= BUFFER_SWAP_WRITE_8192_SIZE )
            {
                uPoolSize = BUFFER_SWAP_WRITE_8192_SIZE;
                pagedLookasideList = &GlobalContext.SwapWriteLookasideList_8192;
                eAllocateType = BUFFER_SWAP_READ_8192;
            }
            else if( uRequiredSize <= BUFFER_SWAP_WRITE_16384_SIZE )
            {
                uPoolSize = BUFFER_SWAP_WRITE_16384_SIZE;
                pagedLookasideList = &GlobalContext.SwapWriteLookasideList_16384;
                eAllocateType = BUFFER_SWAP_READ_16384;
            }
            else if( uRequiredSize <= BUFFER_SWAP_WRITE_65536_SIZE )
            {
                uPoolSize = BUFFER_SWAP_WRITE_65536_SIZE;
                pagedLookasideList = &GlobalContext.SwapWriteLookasideList_65536;
                eAllocateType = BUFFER_SWAP_READ_65536;
            }
            else
            {
                uPoolSize = 1;  // 밑의 함수에서 uPoolSize 의 최소값을 지정해야한다 
                eAllocateType = BUFFER_SWAP_READ;
            }

            ASSERT( uPoolSize > 0 );

        } break;
        default: ASSERT( false );
    }

    Status = AllocateGenericBuffer( ( TyGenericBuffer<VOID>* ) & tyGenericBuffer, uRequiredSize, pagedLookasideList, uPoolSize, poolTag, PagedPool );

    if( NT_SUCCESS( Status ) )
    {
        tyGenericBuffer.BufferType = eAllocateType;
    }

    return tyGenericBuffer;
}
