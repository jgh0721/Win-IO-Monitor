#include "volumeNameMgr.hpp"

#include "pool.hpp"
#include "contextMgr_Defs.hpp"

#include "fltCmnLibs.hpp"
#include "policies/GlobalFilter.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

typedef struct _VOLUME_INFO
{
    LONG        RefCount;
    LIST_ENTRY  ListHead;

} VOLUME_INFO;

VOLUME_INFO* VolumeNameMgr_Ref();
VOLUME_INFO* VolumeNameMgr_Clone();
VOID VolumeNameMgr_Release( __in_opt VOLUME_INFO* VolumeNameMgr );

VOLUME_INFO* VolumeNameMgr_Ref()
{
    auto VolumeNameMgr = ( VOLUME_INFO* )GlobalContext.VolumeNameMgr;
    ASSERT( VolumeNameMgr != NULLPTR );
    InterlockedIncrement( &VolumeNameMgr->RefCount );
    return VolumeNameMgr;
}

VOLUME_INFO* VolumeNameMgr_Clone()
{
    auto OldVolumeNameMgr = VolumeNameMgr_Ref();
    auto NewVolumeNameMgr = ( VOLUME_INFO* )ExAllocatePoolWithTag( NonPagedPool, sizeof( VOLUME_INFO ), POOL_MAIN_TAG );

    do
    {
        if( NewVolumeNameMgr == NULLPTR )
            break;

        RtlZeroMemory( NewVolumeNameMgr, sizeof( VOLUME_INFO ) );
        NewVolumeNameMgr->RefCount = 1;
        InitializeListHead( &NewVolumeNameMgr->ListHead );

        auto Head = &OldVolumeNameMgr->ListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, VOLUME_INFO_ENTRY, ListEntry );
            auto NewItem = ( VOLUME_INFO_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( VOLUME_INFO_ENTRY ), POOL_MAIN_TAG );

            if( NewItem == NULLPTR )
                break;

            RtlZeroMemory( NewItem, sizeof( VOLUME_INFO_ENTRY ) );
            RtlStringCchCopyW( NewItem->VolumeNameBuffer, 128, Item->VolumeNameBuffer );
            NewItem->VolumeNameCch = nsUtils::strlength( NewItem->VolumeNameBuffer );
            NewItem->Letter = Item->Letter;

            InsertTailList( &NewVolumeNameMgr->ListHead, &NewItem->ListEntry );
        }

    } while( false );

    VolumeNameMgr_Release( OldVolumeNameMgr );
    return NewVolumeNameMgr;
}

VOID VolumeNameMgr_Release( __in_opt VOLUME_INFO* VolumeNameMgr )
{
    if( VolumeNameMgr == NULLPTR )
        return;

    auto Ret = InterlockedDecrement( &VolumeNameMgr->RefCount );
    if( Ret > 0 )
        return;

    VolumeNameMgr->RefCount = 0;
    if( GlobalContext.GlobalFilter == VolumeNameMgr )
        return;

    auto Head = &VolumeNameMgr->ListHead;
    while( !IsListEmpty( Head ) )
    {
        auto List = RemoveHeadList( Head );
        if( List == Head )
            break;

        auto Item = CONTAINING_RECORD( List, VOLUME_INFO_ENTRY, ListEntry );
        if( Item != NULLPTR )
            ExFreePoolWithTag( Item, POOL_MAIN_TAG );
    }

    ExFreePoolWithTag( VolumeNameMgr, POOL_MAIN_TAG );
    VolumeNameMgr = NULLPTR;
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS InitializeVolumeNameMgr()
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        auto VolumeNameMgr = ( VOLUME_INFO* )ExAllocatePoolWithTag( NonPagedPool, sizeof( VOLUME_INFO ), POOL_MAIN_TAG );
        if( VolumeNameMgr == NULLPTR )
            break;

        RtlZeroMemory( VolumeNameMgr, sizeof( VOLUME_INFO ) );
        InitializeListHead( &VolumeNameMgr->ListHead );
        VolumeNameMgr->RefCount = 0;

        InterlockedExchangePointer( &GlobalContext.VolumeNameMgr, VolumeNameMgr );

        Status = STATUS_SUCCESS;

    } while( false );

    return Status;
}

NTSTATUS UninitializeVolumeNameMgr()
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        auto Current = VolumeNameMgr_Ref();
        auto VolumeNameMgr = ( VOLUME_INFO* )ExAllocatePoolWithTag( NonPagedPool, sizeof( VOLUME_INFO ), POOL_MAIN_TAG );

        RtlZeroMemory( VolumeNameMgr, sizeof( VOLUME_INFO ) );
        InitializeListHead( &VolumeNameMgr->ListHead );
        VolumeNameMgr->RefCount = 0;

        InterlockedExchangePointer( &GlobalContext.VolumeNameMgr, VolumeNameMgr );
        VolumeNameMgr_Release( Current );

        Status = STATUS_SUCCESS;

    } while( false );

    return Status;
}

NTSTATUS VolumeMgr_Add( const WCHAR* DeviceVolumeName, WCHAR DriveLetter )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    auto VolumeNameMgr = VolumeNameMgr_Clone();

    do
    {
        auto Head = &VolumeNameMgr->ListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, VOLUME_INFO_ENTRY, ListEntry );

            if( _wcsicmp( Item->VolumeNameBuffer, DeviceVolumeName ) == 0 )
            {
                Item->Letter = DriveLetter;
                InterlockedExchangePointer( &GlobalContext.VolumeNameMgr, VolumeNameMgr );
                Status = STATUS_SUCCESS;
                break;
            }
        }

        if( Status == STATUS_SUCCESS )
            break;

        auto Item = ( VOLUME_INFO_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( VOLUME_INFO_ENTRY ), POOL_MAIN_TAG );
        if( Item == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory( Item, sizeof( VOLUME_INFO_ENTRY ) );
        RtlStringCchCopyW( Item->VolumeNameBuffer, _countof( Item->VolumeNameBuffer ), DeviceVolumeName );
        Item->Letter = DriveLetter;
        Item->VolumeNameCch = nsUtils::strlength( Item->VolumeNameBuffer );

        Status = STATUS_SUCCESS;

    } while( false );

    VolumeNameMgr_Release( VolumeNameMgr );
    return Status;
}

NTSTATUS VolumeMgr_Remove( const WCHAR* DeviceVolumeName )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    auto VolumeNameMgr = VolumeNameMgr_Clone();

    do
    {
        auto Head = &VolumeNameMgr->ListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, VOLUME_INFO_ENTRY, ListEntry );

            if( _wcsicmp( Item->VolumeNameBuffer, DeviceVolumeName ) == 0 )
            {
                RemoveEntryList( &Item->ListEntry );
                ExFreePoolWithTag( Item, POOL_MAIN_TAG );
                InterlockedExchangePointer( &GlobalContext.VolumeNameMgr, VolumeNameMgr );
                Status = STATUS_SUCCESS;
                break;
            }
        }

    } while( false );

    VolumeNameMgr_Release( VolumeNameMgr );
    return Status;
}

void VolumeMgr_Replace( WCHAR* Path, ULONG BufferSize )
{
    auto VolumeNameMgr = VolumeNameMgr_Ref();

    do
    {
        ASSERT( Path != NULLPTR );
        ASSERT( BufferSize > 0 );

        if( Path == NULLPTR || BufferSize == 0 )
            break;

        auto Head = &VolumeNameMgr->ListHead;

        for( auto Current = Head->Blink; Current != Head; Current = Current->Blink )
        {
            auto Item = CONTAINING_RECORD( Current, VOLUME_INFO_ENTRY, ListEntry );

            if( nsUtils::StartsWithW( Path, Item->VolumeNameBuffer ) == NULLPTR )
                continue;

            Path[ 0 ] = Item->Letter;
            Path[ 1 ] = L':';
            RtlMoveMemory( &Path[ 2 ], &Path[ Item->VolumeNameCch ], BufferSize - ( Item->VolumeNameCch * sizeof( WCHAR ) ) );

            break;
        }
        
    } while( false );

    VolumeNameMgr_Release( VolumeNameMgr );
}

