#include "notifyMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

namespace nsDetail
{
    LIST_ENTRY          ListHead;
    KSPIN_LOCK          ListLock;

    volatile LONG       CurrentItemCount = 0;
    unsigned int        MaxNotifyItemCount = 8192;

}

NTSTATUS InitializeNotifyMgr()
{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    do
    {
        InitializeListHead( &nsDetail::ListHead );
        KeInitializeSpinLock( &nsDetail::ListLock );

        Status = STATUS_SUCCESS;
    } while( false );

    return Status;
}

void CloseNotifyMgr()
{
    PLIST_ENTRY ListItem = NULLPTR;

    do
    {
        ListItem = ExInterlockedRemoveHeadList( &nsDetail::ListHead, &nsDetail::ListLock );
        if( ListItem == NULLPTR )
            break;

        InterlockedDecrement( &nsDetail::CurrentItemCount );
        auto Item = CONTAINING_RECORD( ListItem, NOTIFY_ITEM, ListEntry );
        FreeNotifyItem( Item );

    } while( true );
}

NOTIFY_ITEM* AllocateNotifyItem( ULONG SendPacketSize )
{
    NOTIFY_ITEM* Item = NULLPTR;
    LONG Count = InterlockedIncrement( &nsDetail::CurrentItemCount );

    do
    {
        if( Count >= nsDetail::MaxNotifyItemCount )
        {
            PLIST_ENTRY ListItem = ExInterlockedRemoveHeadList( &nsDetail::ListHead, &nsDetail::ListLock );
            InterlockedDecrement( &nsDetail::CurrentItemCount );
            auto Old = CONTAINING_RECORD( ListItem, NOTIFY_ITEM, ListEntry );
            FreeNotifyItem( Old );
        }

        Item = (NOTIFY_ITEM*)ExAllocatePool( NonPagedPool, sizeof( NOTIFY_ITEM ) );
        if( Item == NULLPTR )
            break;

        Item->SendPacket = ( MSG_SEND_PACKET* )ExAllocatePool( NonPagedPool, SendPacketSize );
        if( Item->SendPacket == NULLPTR )
        {
            ExFreePool( Item );
            Item = NULLPTR;
            break;
        }

        AppendNotifyItem( Item );

    } while( false );

    return Item;
}

void FreeNotifyItem( NOTIFY_ITEM* NotifyItem )
{
    if( NotifyItem == NULLPTR )
        return;

    ExFreePool( NotifyItem->SendPacket );
    ExFreePool( NotifyItem );
}

void AppendNotifyItem( NOTIFY_ITEM* NotifyItem )
{
    ExInterlockedInsertTailList( &nsDetail::ListHead, &NotifyItem->ListEntry, &nsDetail::ListLock );
}

NTSTATUS CollectNotifyItem( PVOID Buffer, ULONG BufferSize, __out ULONG* WrittenBytes, ULONG* NotifyItemCount )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    __try
    {
        if( Buffer == NULLPTR || NotifyItemCount == NULLPTR )
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        if( BufferSize <= sizeof( MSG_SEND_PACKET ) )
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            __leave;
        }

        *WrittenBytes = 0;
        *NotifyItemCount = 0;
        auto Item = ( MSG_SEND_PACKET* )Buffer;
        PLIST_ENTRY ListItem = NULLPTR;

        do
        {
            ListItem = ExInterlockedRemoveHeadList( &nsDetail::ListHead, &nsDetail::ListLock );
            if( ListItem == NULLPTR )
                break;

            InterlockedDecrement( &nsDetail::CurrentItemCount );

            auto NotifyItem = CONTAINING_RECORD( ListItem, NOTIFY_ITEM, ListEntry );
            if( NotifyItem->SendPacket == NULLPTR )
                break;

            if( NotifyItem->SendPacket->MessageSize > BufferSize )
                break;

            ( *NotifyItemCount )++;
            WrittenBytes += NotifyItem->SendPacket->MessageSize;
            memcpy( Item, NotifyItem->SendPacket, NotifyItem->SendPacket->MessageSize );
            BufferSize -= NotifyItem->SendPacket->MessageSize;

            // Prepare Next Entry
            Item = (MSG_SEND_PACKET*)Add2Ptr( Item, NotifyItem->SendPacket->MessageSize );

            FreeNotifyItem( NotifyItem );

        } while( BufferSize > sizeof( MSG_SEND_PACKET ) );
    }
    __finally
    {
        
    }

    return Status;
}
