#include "GlobalFilter.hpp"

#include "pool.hpp"
#include "utilities/contextMgr_Defs.hpp"

#include "fltCmnLibs.hpp"
#include "WinIOIsolation_Event.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

typedef struct _GLOBAL_FILTER
{
    LONG                RefCount;

    LIST_ENTRY          IncludeListHead;
    LIST_ENTRY          ExcludeListHead;
    
} GLOBAL_FILTER, *PGOLBAL_FILTER;

GLOBAL_FILTER* GlobalFilter_Ref();
GLOBAL_FILTER* GlobalFilter_Clone();
void GlobalFilter_Release( __in GLOBAL_FILTER*& GlobalFilter );

GLOBAL_FILTER* GlobalFilter_Ref()
{
    auto GFilter = ( GLOBAL_FILTER* )GlobalContext.GlobalFilter;
    ASSERT( GFilter != NULLPTR );
    InterlockedIncrement( &GFilter->RefCount );
    return GFilter;
}

GLOBAL_FILTER* GlobalFilter_Clone()
{
    auto OldFilter = GlobalFilter_Ref();
    auto NewFilter = ( GLOBAL_FILTER* )ExAllocatePoolWithTag( NonPagedPool, sizeof( GLOBAL_FILTER ), POOL_MAIN_TAG );

    do
    {
        if( NewFilter == NULLPTR )
            break;

        RtlZeroMemory( NewFilter, sizeof( GLOBAL_FILTER ) );
        NewFilter->RefCount = 1;
        InitializeListHead( &NewFilter->IncludeListHead );
        InitializeListHead( &NewFilter->ExcludeListHead );

        auto Head = &OldFilter->IncludeListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, GLOBAL_FILTER_ENTRY, ListEntry );
            auto NewItem = ( GLOBAL_FILTER_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( GLOBAL_FILTER_ENTRY ), POOL_MAIN_TAG );

            if( NewItem == NULLPTR )
                break;

            RtlZeroMemory( NewItem, sizeof( GLOBAL_FILTER_ENTRY ) );
            RtlStringCchCopyW( NewItem->FilterMask, MAX_PATH, Item->FilterMask );

            InsertTailList( &NewFilter->IncludeListHead, &NewItem->ListEntry );
        }

        Head = &OldFilter->ExcludeListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, GLOBAL_FILTER_ENTRY, ListEntry );
            auto NewItem = ( GLOBAL_FILTER_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( GLOBAL_FILTER_ENTRY ), POOL_MAIN_TAG );

            if( NewItem == NULLPTR )
                break;

            RtlZeroMemory( NewItem, sizeof( GLOBAL_FILTER_ENTRY ) );
            RtlStringCchCopyW( NewItem->FilterMask, MAX_PATH, Item->FilterMask );

            InsertTailList( &NewFilter->ExcludeListHead, &NewItem->ListEntry );
        }

    } while( false );

    GlobalFilter_Release( OldFilter );
    return NewFilter;
}

void GlobalFilter_Release( __in GLOBAL_FILTER*& GlobalFilter )
{
    if( GlobalFilter == NULLPTR )
        return;

    auto Ret = InterlockedDecrement( &GlobalFilter->RefCount );
    if( Ret > 0 )
        return;

    GlobalFilter->RefCount = 0;
    if( GlobalContext.GlobalFilter == GlobalFilter )
        return;

    auto Head = &GlobalFilter->IncludeListHead;
    while( !IsListEmpty( Head ) )
    {
        auto List = RemoveHeadList( Head );
        if( List == Head )
            break;

        auto Item = CONTAINING_RECORD( List , GLOBAL_FILTER_ENTRY, ListEntry );
        if( Item != NULLPTR )
            ExFreePoolWithTag( Item, POOL_MAIN_TAG );
    }

    Head = &GlobalFilter->ExcludeListHead;
    while( !IsListEmpty( Head ) )
    {
        auto List = RemoveHeadList( Head );
        if( List == Head )
            break;

        auto Item = CONTAINING_RECORD( List, GLOBAL_FILTER_ENTRY, ListEntry );
        if( Item != NULLPTR )
            ExFreePoolWithTag( Item, POOL_MAIN_TAG );
    }

    ExFreePoolWithTag( GlobalFilter, POOL_MAIN_TAG );
    GlobalFilter = NULLPTR;
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS InitializeGlobalFilter()
{
    auto Filter = ( GLOBAL_FILTER* )ExAllocatePoolWithTag( NonPagedPool, sizeof( GLOBAL_FILTER ), POOL_MAIN_TAG );
    RtlZeroMemory( Filter, sizeof( GLOBAL_FILTER ) );
    Filter->RefCount = 0;
    InitializeListHead( &Filter->IncludeListHead );
    InitializeListHead( &Filter->ExcludeListHead );

    GlobalContext.GlobalFilter = Filter;

    return STATUS_SUCCESS;
}

NTSTATUS UninitializeGlobalFilter()
{
    GLOBAL_FILTER* Filter = GlobalFilter_Ref();
    // if want to reset GlobalFilter, create emptry globalfilter then exchange 
    InterlockedExchangePointer( &GlobalContext.GlobalFilter, NULLPTR );
    GlobalFilter_Release( Filter );

    return STATUS_SUCCESS;
}

NTSTATUS GlobalFilter_Add( const WCHAR* FilterMask, bool isInclude )
{
    NTSTATUS Status = STATUS_SUCCESS;
    GLOBAL_FILTER* NewFilter = NULLPTR;
    GLOBAL_FILTER_ENTRY* Item = NULLPTR;

    do
    {
        if( FilterMask == NULLPTR || nsUtils::strlength( FilterMask ) == 0 )
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        NewFilter = GlobalFilter_Clone();
        if( NewFilter == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        Item = ( GLOBAL_FILTER_ENTRY*)ExAllocatePoolWithTag( NonPagedPool, sizeof( GLOBAL_FILTER_ENTRY ), POOL_MAIN_TAG );
        if( Item == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlStringCchCopyW( Item->FilterMask, MAX_PATH, FilterMask );
        InsertTailList( isInclude == true ? &NewFilter->IncludeListHead : &NewFilter->ExcludeListHead, &Item->ListEntry );

        InterlockedExchangePointer( &GlobalContext.GlobalFilter, NewFilter );

        Status = STATUS_SUCCESS;

    } while( false );

    GlobalFilter_Release( NewFilter );
    return Status;
}

NTSTATUS GlobalFilter_Remove( const WCHAR* FilterMask )
{
    NTSTATUS Status = STATUS_SUCCESS;
    GLOBAL_FILTER* NewFilter = NULLPTR;

    do
    {
        if( FilterMask == NULLPTR || nsUtils::strlength( FilterMask ) == 0 )
        {
            Status = STATUS_NOT_FOUND;
            break;
        }

        NewFilter = GlobalFilter_Clone();
        if( NewFilter == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ULONG ChangeCount = 0;
        auto Head = &NewFilter->IncludeListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, GLOBAL_FILTER_ENTRY, ListEntry );

            if( _wcsicmp( FilterMask, Item->FilterMask ) == 0 )
            {
                RemoveEntryList( &Item->ListEntry );
                ExFreePool( Item );
                ChangeCount++;
            }
        }

        Head = &NewFilter->ExcludeListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, GLOBAL_FILTER_ENTRY, ListEntry );

            if( _wcsicmp( FilterMask, Item->FilterMask ) == 0 )
            {
                RemoveEntryList( &Item->ListEntry );
                ExFreePool( Item );
                ChangeCount++;
            }
        }

        if( ChangeCount > 0 )
            InterlockedExchangePointer( &GlobalContext.GlobalFilter, NewFilter );

    } while( false );

    GlobalFilter_Release( NewFilter );
    return Status;
}

NTSTATUS GlobalFilter_Match( const WCHAR* FileName, bool IsInclude )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    auto GFilter = GlobalFilter_Ref();

    do
    {
        if( FileName == NULLPTR || nsUtils::strlength( FileName ) == 0 )
            break;

        if( GFilter == NULLPTR )
            break;

        auto Head = IsInclude == true ? &GFilter->IncludeListHead : &GFilter->ExcludeListHead;
        if( IsListEmpty( Head ) )
        {
            Status = STATUS_NO_DATA_DETECTED;
            break;
        }
        
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, GLOBAL_FILTER_ENTRY, ListEntry );

            if( nsUtils::WildcardMatch_straight( FileName, Item->FilterMask, false ) == true )
            {
                Status = STATUS_SUCCESS;
                break;
            }
        }
        
    } while( false );

    GlobalFilter_Release( GFilter );
    return Status;
}

NTSTATUS GlobalFilter_Reset()
{
    auto NewFilter = ( GLOBAL_FILTER* )ExAllocatePoolWithTag( NonPagedPool, sizeof( GLOBAL_FILTER ), POOL_MAIN_TAG );
    if( NewFilter == NULLPTR )
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory( NewFilter, sizeof( GLOBAL_FILTER ) );
    NewFilter->RefCount = 0;
    InitializeListHead( &NewFilter->IncludeListHead );
    InitializeListHead( &NewFilter->ExcludeListHead );

    GLOBAL_FILTER* Filter = GlobalFilter_Ref();
    InterlockedExchangePointer( &GlobalContext.GlobalFilter, NewFilter );
    GlobalFilter_Release( Filter );

    return STATUS_SUCCESS;
}

ULONG GlobalFilter_Count( bool IsInclude )
{
    ULONG Count = 0;
    auto GFilter = GlobalFilter_Ref();

    do
    {
        if( GFilter == NULLPTR )
            break;

        auto Head = IsInclude == true ? &GFilter->IncludeListHead : &GFilter->ExcludeListHead;

        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
            Count++;

    } while( false );

    GlobalFilter_Release( GFilter );
    return Count;
}

NTSTATUS GlobalFilter_Copy( PVOID Buffer, ULONG BufferSize, bool IsInclude )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    auto GFilter = GlobalFilter_Ref();

    do
    {
        if( GFilter == NULLPTR )
            break;

        if( Buffer == NULLPTR || BufferSize == 0 )
            break;

        auto T = ( USER_GLOBAL_FILTER* )Buffer;
        auto Head = IsInclude == true ? &GFilter->IncludeListHead : &GFilter->ExcludeListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, GLOBAL_FILTER_ENTRY, ListEntry );

            RtlCopyMemory( T->FilterMask, Item->FilterMask, sizeof( WCHAR ) * MAX_PATH );
            T->IsInclude = IsInclude;

            T = (USER_GLOBAL_FILTER*)Add2Ptr( T, sizeof( USER_GLOBAL_FILTER ) );
        }

        Status = STATUS_SUCCESS;

    } while( false );

    GlobalFilter_Release( GFilter );
    return Status;
}
