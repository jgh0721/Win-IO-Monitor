#include "bufferMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS AllocateGenericBuffer( TyGenericBuffer<VOID>* GenericBuffer, ULONG RequiredSize, __in PNPAGED_LOOKASIDE_LIST LookasideList, ULONG PoolSize, ULONG PoolTag )
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
            GenericBuffer->Buffer = ExAllocateFromNPagedLookasideList( LookasideList );
            GenericBuffer->BufferSize = PoolSize;
        }
        else
        {
            GenericBuffer->IsAllocatedFromLookasideList = FALSE;
            GenericBuffer->Buffer = ExAllocatePoolWithTag( NonPagedPool, RequiredSize, PoolTag );
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

void DeallocateGenericBuffer( TyGenericBuffer<void>* GenericBuffer, PNPAGED_LOOKASIDE_LIST LookasideList )
{
    ASSERT( GenericBuffer != NULLPTR );
    ASSERT( LookasideList != NULLPTR );

    if( GenericBuffer->IsAllocatedFromLookasideList != FALSE )
    {
        ExFreeToNPagedLookasideList( LookasideList, GenericBuffer->Buffer );
    }
    else
    {
        ExFreePool( GenericBuffer->Buffer );
    }

    GenericBuffer->Buffer = NULLPTR;
    GenericBuffer->BufferSize = 0;
}
