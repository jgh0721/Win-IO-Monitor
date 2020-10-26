#include "ProcessFilter.hpp"

#include "utilities/contextMgr_Defs.hpp"

#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _PROCESS_FILTER
{
    LONG            RefCount;
    LIST_ENTRY      ListHead;

} PROCESS_FILTER, *PPROCESS_FILTER;

PROCESS_FILTER* ProcessFilter_Ref();
PROCESS_FILTER* ProcessFilter_Clone();
void ProcessFilter_Release( __in PROCESS_FILTER*& ProcessFilter );
void ProcessFilterMaskEntry_Release( __in PROCESS_FILTER_MASK_ENTRY*& ProcessFilterMaskEntry );
void ProcessFilterEntry_Release( __in PROCESS_FILTER_ENTRY*& ProcessFilterEntry );

PROCESS_FILTER* ProcessFilter_Ref()
{
    auto PFilter = ( PROCESS_FILTER* )GlobalContext.ProcessFilter;
    ASSERT( PFilter != NULLPTR );
    InterlockedIncrement( &PFilter->RefCount );
    return PFilter;
}

PROCESS_FILTER* ProcessFilter_Clone()
{
    auto OldFilter = ProcessFilter_Ref();
    auto NewFilter = ( PROCESS_FILTER* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER ), POOL_MAIN_TAG );

    do
    {
        if( NewFilter == NULLPTR )
            break;

        RtlZeroMemory( NewFilter, sizeof( PROCESS_FILTER ) );
        NewFilter->RefCount = 1;
        InitializeListHead( &NewFilter->ListHead );

        auto Head = &OldFilter->ListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto OldItem = CONTAINING_RECORD( Current, PROCESS_FILTER_ENTRY, ListEntry );
            auto NewItem = ( PROCESS_FILTER_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER_ENTRY ), POOL_MAIN_TAG );

            if( NewItem == NULLPTR )
                break;

            RtlZeroMemory( NewItem, sizeof( PROCESS_FILTER_ENTRY ) );
            NewItem->ProcessId = OldItem->ProcessId;
            RtlStringCchCopyW( NewItem->ProcessFilterMask, MAX_PATH, OldItem->ProcessFilterMask );

            InitializeListHead( &NewItem->IncludeListHead );
            InitializeListHead( &NewItem->ExcludeListHead );

            LIST_ENTRY* MaskHead = NULLPTR;

            MaskHead = &OldItem->IncludeListHead;
            for( auto CurrentMask = MaskHead->Flink; CurrentMask != MaskHead; CurrentMask = CurrentMask->Flink )
            {
                auto OldMask = CONTAINING_RECORD( CurrentMask, PROCESS_FILTER_MASK_ENTRY, ListEntry );
                auto NewMask = ( PROCESS_FILTER_MASK_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER_MASK_ENTRY ), POOL_MAIN_TAG );
                if( NewMask == NULLPTR )
                    break;

                RtlZeroMemory( NewMask, sizeof( PROCESS_FILTER_MASK_ENTRY ) );
                RtlStringCchCopyW( NewMask->FilterMask, MAX_PATH, OldMask->FilterMask );
                InsertTailList( &NewItem->IncludeListHead, &NewMask->ListEntry );
            }

            MaskHead = &OldItem->ExcludeListHead;
            for( auto CurrentMask = MaskHead->Flink; CurrentMask != MaskHead; CurrentMask = CurrentMask->Flink )
            {
                auto OldMask = CONTAINING_RECORD( CurrentMask, PROCESS_FILTER_MASK_ENTRY, ListEntry );
                auto NewMask = ( PROCESS_FILTER_MASK_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER_MASK_ENTRY ), POOL_MAIN_TAG );
                if( NewMask == NULLPTR )
                    break;

                RtlZeroMemory( NewMask, sizeof( PROCESS_FILTER_MASK_ENTRY ) );
                RtlStringCchCopyW( NewMask->FilterMask, MAX_PATH, OldMask->FilterMask );
                InsertTailList( &NewItem->ExcludeListHead, &NewMask->ListEntry );
            }

            InsertTailList( &NewFilter->ListHead, &NewItem->ListEntry );
        }

    } while( false );

    ProcessFilter_Release( OldFilter );
    return NewFilter;
}

void ProcessFilter_Release( __in PROCESS_FILTER*& ProcessFilter )
{
    if( ProcessFilter == NULLPTR )
        return;


    auto Ret = InterlockedDecrement( &ProcessFilter->RefCount );
    if( Ret > 0 )
        return;

    ProcessFilter->RefCount = 0;
    if( GlobalContext.ProcessFilter == ProcessFilter )
        return;

    auto Head = &ProcessFilter->ListHead;
    while( !IsListEmpty( Head ) )
    {
        auto List = RemoveHeadList( Head );
        if( List == Head )
            break;

        auto Item = CONTAINING_RECORD( List, PROCESS_FILTER_ENTRY, ListEntry );
        ProcessFilterEntry_Release( Item );
    }

    ExFreePoolWithTag( ProcessFilter, POOL_MAIN_TAG );
    ProcessFilter = NULLPTR;
}

void ProcessFilterMaskEntry_Release( __in PROCESS_FILTER_MASK_ENTRY*& ProcessFilterMaskEntry )
{
    if( ProcessFilterMaskEntry == NULLPTR )
        return;

    // Do Nothing, for future...

    ExFreePoolWithTag( ProcessFilterMaskEntry, POOL_MAIN_TAG );
}

void ProcessFilterEntry_Release( __in PROCESS_FILTER_ENTRY*& ProcessFilterEntry )
{
    if( ProcessFilterEntry == NULLPTR )
        return;

    LIST_ENTRY* Head = NULLPTR;

    Head = &ProcessFilterEntry->IncludeListHead;
    while( !IsListEmpty( Head ) )
    {
        auto Item = RemoveHeadList( Head );
        if( Item == Head )
            break;

        auto Mask = CONTAINING_RECORD( Item, PROCESS_FILTER_MASK_ENTRY, ListEntry );
        ProcessFilterMaskEntry_Release( Mask );
    }

    Head = &ProcessFilterEntry->ExcludeListHead;
    while( !IsListEmpty( Head ) )
    {
        auto Item = RemoveHeadList( Head );
        if( Item == Head )
            break;

        auto Mask = CONTAINING_RECORD( Item, PROCESS_FILTER_MASK_ENTRY, ListEntry );
        ProcessFilterMaskEntry_Release( Mask );
    }

    ExFreePoolWithTag( ProcessFilterEntry, POOL_MAIN_TAG );
    ProcessFilterEntry = NULLPTR;
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS InitializeProcessFilter()
{
    auto Filter = ( PROCESS_FILTER* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER ), POOL_MAIN_TAG );
    RtlZeroMemory( Filter, sizeof( PROCESS_FILTER ) );
    Filter->RefCount = 0;
    InitializeListHead( &Filter->ListHead );

    GlobalContext.ProcessFilter = Filter;

    return STATUS_SUCCESS;
}

NTSTATUS UninitializeProcessFilter()
{
    PROCESS_FILTER* Filter = ProcessFilter_Ref();
    // if want to reset ProcessFilter, create emptry ProcessFilter then exchange 
    InterlockedExchangePointer( &GlobalContext.ProcessFilter, NULLPTR );
    ProcessFilter_Release( Filter );

    return STATUS_SUCCESS;
}

NTSTATUS ProcessFilter_Add( PROCESS_FILTER_ENTRY* PFilterItem )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    auto PFilter = ProcessFilter_Clone();

    do
    {
        if( PFilterItem == NULLPTR )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        auto Head = &PFilter->ListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, PROCESS_FILTER_ENTRY, ListEntry );

            if( nsUtils::IsEqualUUID( PFilterItem->Id, Item->Id ) == false )
                continue;

            Status = STATUS_OBJECTID_EXISTS;
            break;
        }

        if( Status != STATUS_NOT_FOUND )
            break;

        InsertTailList( Head, &PFilterItem->ListEntry );
        InterlockedExchangePointer( &GlobalContext.ProcessFilter, PFilter );

        Status = STATUS_SUCCESS;
    } while( false );

    ProcessFilter_Release( PFilter );
    return Status;
}

NTSTATUS ProcessFilter_AddEntry( UUID* Id, PROCESS_FILTER_MASK_ENTRY* Entry, __in BOOLEAN IsInclude )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    auto PFilter = ProcessFilter_Clone();

    do
    {
        if( Id == NULLPTR || Entry == NULLPTR )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        PROCESS_FILTER_ENTRY* Item = NULLPTR;
        auto Head = &PFilter->ListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            Item = CONTAINING_RECORD( Current, PROCESS_FILTER_ENTRY, ListEntry );

            if( nsUtils::IsEqualUUID( *Id, Item->Id ) == false )
            {
                Item = NULLPTR;
                continue;
            }

            Status = STATUS_SUCCESS;
            break;
        }

        if( Status != STATUS_SUCCESS )
            break;

        if( IsInclude != FALSE )
            Head = &Item->IncludeListHead;
        else
            Head = &Item->ExcludeListHead;

        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Child = CONTAINING_RECORD( Current, PROCESS_FILTER_MASK_ENTRY, ListEntry );

            if( nsUtils::IsEqualUUID( Child->Id, Entry->Id ) == true )
            {
                Status = STATUS_OBJECTID_EXISTS;
                break;
            }
        }

        if( Status == STATUS_OBJECTID_EXISTS )
            break;

        InsertTailList( Head, &Entry->ListEntry );
        InterlockedExchangePointer( &GlobalContext.ProcessFilter, PFilter );
        Status = STATUS_SUCCESS;

    } while( false );

    ProcessFilter_Release( PFilter );
    return Status;
}

NTSTATUS ProcessFilter_Remove( UUID* Id )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    auto PFilter = ProcessFilter_Clone();

    do
    {
        if( Id == NULLPTR )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        // first, delete same item if it is exists
        auto Head = &PFilter->ListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, PROCESS_FILTER_ENTRY, ListEntry );

            if( nsUtils::IsEqualUUID( *Id, Item->Id ) == false )
                continue;

            RemoveEntryList( &Item->ListEntry );
            ProcessFilterEntry_Release( Item );
            Status = STATUS_SUCCESS;
        }

        if( Status == STATUS_SUCCESS )
            InterlockedExchangePointer( &GlobalContext.ProcessFilter, PFilter );

    } while( false );

    ProcessFilter_Release( PFilter );
    return Status;
}

NTSTATUS ProcessFilter_Remove( ULONG ProcessId, const WCHAR* ProcessNameMask )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    auto PFilter = ProcessFilter_Clone();

    do
    {
        if( ProcessId == 0 && ProcessNameMask == NULLPTR )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        // first, delete same item if it is exists
        auto Head = &PFilter->ListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, PROCESS_FILTER_ENTRY, ListEntry );

            if( ( ( ProcessId != 0 ) && ( ProcessId == Item->ProcessId ) ) ||
                ( _wcsicmp( ProcessNameMask, Item->ProcessFilterMask ) == 0 ) )
            {
                RemoveEntryList( &Item->ListEntry );
                ProcessFilterEntry_Release( Item );
                Status = STATUS_SUCCESS;
            }
        }

        if( Status == STATUS_SUCCESS )
            InterlockedExchangePointer( &GlobalContext.ProcessFilter, PFilter );

    } while( false );

    ProcessFilter_Release( PFilter );
    return Status;
}

NTSTATUS ProcessFilter_RemoveEntry( UUID* ParentId, UUID* EntryId, __in BOOLEAN IsInclude )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    auto PFilter = ProcessFilter_Clone();

    do
    {
        if( ParentId == NULLPTR || EntryId == NULLPTR )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        PROCESS_FILTER_ENTRY* Item = NULLPTR;
        auto Head = &PFilter->ListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            Item = CONTAINING_RECORD( Current, PROCESS_FILTER_ENTRY, ListEntry );

            if( nsUtils::IsEqualUUID( *ParentId, Item->Id ) == false )
            {
                Item = NULLPTR;
                continue;
            }

            Status = STATUS_SUCCESS;
            break;
        }

        if( Status != STATUS_SUCCESS )
            break;

        if( IsInclude != FALSE )
            Head = &Item->IncludeListHead;
        else
            Head = &Item->ExcludeListHead;

        Status = STATUS_NOT_FOUND;

        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto ChildItem = CONTAINING_RECORD( Current, PROCESS_FILTER_MASK_ENTRY, ListEntry );

            if( nsUtils::IsEqualUUID( *EntryId, ChildItem->Id ) == false )
                continue;

            RemoveEntryList( &ChildItem->ListEntry );
            ProcessFilterMaskEntry_Release( ChildItem );
            Status = STATUS_SUCCESS;
            break;
        }

        if( Status == STATUS_SUCCESS )
            InterlockedExchangePointer( &GlobalContext.ProcessFilter, PFilter );

    } while( false );

    ProcessFilter_Release( PFilter );
    return Status;
}

NTSTATUS ProcessFilter_Match( ULONG ProcessId, TyGenericBuffer<WCHAR>* ProcessFilePath, HANDLE* ProcessFilter, PROCESS_FILTER_ENTRY** MatchItem )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    auto PFilter = ProcessFilter_Ref();

    do
    {
        if( ProcessId == 0 ) 
        {
            if( ProcessFilePath == NULLPTR )
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            if( ProcessFilePath->Buffer == NULLPTR || ProcessFilePath->BufferSize == 0 )
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
        }

        if( ARGUMENT_PRESENT( ProcessFilter ) )
            *ProcessFilter = NULLPTR;
        if( ARGUMENT_PRESENT( MatchItem ) )
            *MatchItem = NULLPTR;

        auto Head = &PFilter->ListHead;
        if( IsListEmpty( Head ) != FALSE )
        {
            Status = STATUS_NO_DATA_DETECTED;
            break;
        }

        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, PROCESS_FILTER_ENTRY, ListEntry );

            if( ( (ProcessId != 0) && (ProcessId == Item->ProcessId) ) ||
                (  nsUtils::WildcardMatch_straight( ProcessFilePath->Buffer, Item->ProcessFilterMask ) == true )
                )
            {
                *ProcessFilter = ProcessFilter_Ref();
                *MatchItem = Item;

                Status = STATUS_SUCCESS;
                break;
            }
        }
        
    } while( false );

    ProcessFilter_Release( PFilter );
    return Status;
}

NTSTATUS ProcessFilter_SubMatch( PROCESS_FILTER_ENTRY* PFilterItem, TyGenericBuffer<WCHAR>* FilePath, bool* IsIncludeMatch )
{
    NTSTATUS Status = STATUS_NOT_FOUND;

    do
    {
        if( PFilterItem == NULLPTR )
            break;

        if( FilePath == NULLPTR )
            break;

        if( FilePath->Buffer == NULLPTR || FilePath->BufferSize == 0 )
            break;

        if( IsListEmpty( &PFilterItem->IncludeListHead ) && IsListEmpty( &PFilterItem->ExcludeListHead ) )
        {
            Status = STATUS_NO_DATA_DETECTED;
            break;
        }

        LIST_ENTRY* Head = NULLPTR;

        Head = &PFilterItem->ExcludeListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, PROCESS_FILTER_MASK_ENTRY, ListEntry );
            if( nsUtils::WildcardMatch_straight( FilePath->Buffer, Item->FilterMask ) == false )
                continue;

            if( ARGUMENT_PRESENT( IsIncludeMatch ) )
                *IsIncludeMatch = false;

            Status = STATUS_SUCCESS;
            break;
        }

        if( Status == STATUS_SUCCESS )
            break;

        Head = &PFilterItem->IncludeListHead;
        for( auto Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            auto Item = CONTAINING_RECORD( Current, PROCESS_FILTER_MASK_ENTRY, ListEntry );
            if( nsUtils::WildcardMatch_straight( FilePath->Buffer, Item->FilterMask ) == false )
                continue;

            if( ARGUMENT_PRESENT( IsIncludeMatch ) )
                *IsIncludeMatch = true;

            Status = STATUS_SUCCESS;
            break;
        }

    } while( false );

    return Status;
}

void ProcessFilter_Close( HANDLE ProcessFilterHandle )
{
    if( ProcessFilterHandle == NULLPTR )
        return;

    PROCESS_FILTER* PFilter = ( PROCESS_FILTER* )ProcessFilterHandle;
    ProcessFilter_Release( PFilter );
}

NTSTATUS ProcessFilter_Reset()
{
    auto NewFilter = ( PROCESS_FILTER* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER ), POOL_MAIN_TAG );
    if( NewFilter == NULLPTR )
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory( NewFilter, sizeof( PROCESS_FILTER ) );
    NewFilter->RefCount = 0;
    InitializeListHead( &NewFilter->ListHead );

    PROCESS_FILTER* Filter = ProcessFilter_Ref();
    InterlockedExchangePointer( &GlobalContext.ProcessFilter, NewFilter );
    ProcessFilter_Release( Filter );

    return STATUS_SUCCESS;
}
