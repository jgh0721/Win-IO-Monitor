#include "processFilter.hpp"

#include "utilities/contextMgr_Defs.hpp"
#include "utilities/fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

typedef struct _PROCESS_FILTER_KN
{
    LIST_ENTRY          ListHead;
    LONG                RefCount;
    
} PROCESS_FILTER_KN, *PPROCESS_FILTER_KN;

PROCESS_FILTER_KN* ProcessFilter_Ref();
void ProcessFilter_Release( __in PROCESS_FILTER_KN* ProcessFilter );
PROCESS_FILTER_KN* ProcessFilter_Clone();

PROCESS_FILTER_KN* ProcessFilter_Ref()
{
    PROCESS_FILTER_KN* Filter = ( PROCESS_FILTER_KN* )GlobalContext.ProcessFilter;
    InterlockedIncrement( &Filter->RefCount );
    return Filter;
}

void ProcessFilter_Release( __in PROCESS_FILTER_KN* ProcessFilter )
{
    if( ProcessFilter == NULL )
        return;

    if( InterlockedDecrement( &ProcessFilter->RefCount ) <= 0 )
    {
        ProcessFilter->RefCount = 0;
        if( GlobalContext.ProcessFilter != ProcessFilter )
        {
            // 리스트 해제, 메모리 해제

            LIST_ENTRY* head = &ProcessFilter->ListHead;

            do
            {
                while( !IsListEmpty( head ) )
                {
                    LIST_ENTRY* pCurrentEntry = RemoveHeadList( head );

                    PROCESS_FILTER_ENTRY* entry = CONTAINING_RECORD( pCurrentEntry, PROCESS_FILTER_ENTRY, ListEntry );
                    // TODO: need to free list of entry 
                    ExFreePool( entry );
                }

            } while( false );

            ExFreePool( ProcessFilter );
        }
    }
}

PROCESS_FILTER_KN* ProcessFilter_Clone()
{
    PROCESS_FILTER_KN* NewFilter = ( PROCESS_FILTER_KN* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER_KN ), 'abcd' );
    if( NewFilter == NULL )
        return NULL;

    RtlZeroMemory( NewFilter, sizeof( PROCESS_FILTER_KN ) );
    InitializeListHead( &NewFilter->ListHead );
    NewFilter->RefCount = 1;

    PROCESS_FILTER_KN* OldFilter = ProcessFilter_Ref();
    LIST_ENTRY* Head = &OldFilter->ListHead;

    for( LIST_ENTRY* current = Head->Flink; current != Head; current = current->Flink )
    {
        PROCESS_FILTER_ENTRY* OldEntry = CONTAINING_RECORD( current, PROCESS_FILTER_ENTRY, ListEntry );
        PROCESS_FILTER_ENTRY* NewEntry = ( PROCESS_FILTER_ENTRY* )ExAllocatePool( NonPagedPool, sizeof( PROCESS_FILTER_ENTRY ) );
        if( NewEntry == NULL )
            break;

        RtlZeroMemory( NewEntry, sizeof( PROCESS_FILTER_ENTRY ) );
        NewEntry->ProcessId = OldEntry->ProcessId;
        RtlStringCchCopyW( NewEntry->ProcessMask, MAX_PATH, OldEntry->ProcessMask );
        NewEntry->Flags = OldEntry->Flags;

        NewEntry->FileIOFlags = OldEntry->FileIOFlags;
        NewEntry->FileNotifyFlags = OldEntry->FileNotifyFlags;

        InitializeListHead( &NewEntry->IncludeMaskListHead );
        InitializeListHead( &NewEntry->ExcludeMaskListHead );

        for( LIST_ENTRY* include = OldEntry->IncludeMaskListHead.Flink; include != OldEntry->IncludeMaskListHead.Flink; include = include->Flink  )
        {
            PF_FILTER_MASK_ENTRY* oldInclude = CONTAINING_RECORD( include, PF_FILTER_MASK_ENTRY, ListEntry );
            PF_FILTER_MASK_ENTRY* newInclude = ( PF_FILTER_MASK_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PF_FILTER_MASK_ENTRY ), 'abcd' );
            if( newInclude == NULL )
                break;

            RtlZeroMemory( newInclude, sizeof( PF_FILTER_MASK_ENTRY ) );
            RtlStringCchCopyW( newInclude->wszFilterMask, MAX_PATH, oldInclude->wszFilterMask );

            InsertTailList( &NewEntry->IncludeMaskListHead, &newInclude->ListEntry );
        }

        for( LIST_ENTRY* exclude = OldEntry->ExcludeMaskListHead.Flink; exclude != OldEntry->ExcludeMaskListHead.Flink; exclude = exclude->Flink )
        {
            PF_FILTER_MASK_ENTRY* oldExclude = CONTAINING_RECORD( exclude, PF_FILTER_MASK_ENTRY, ListEntry );
            PF_FILTER_MASK_ENTRY* newExclude = ( PF_FILTER_MASK_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PF_FILTER_MASK_ENTRY ), 'abcd' );
            if( newExclude == NULL )
                break;

            RtlZeroMemory( newExclude, sizeof( PF_FILTER_MASK_ENTRY ) );
            RtlStringCchCopyW( newExclude->wszFilterMask, MAX_PATH, oldExclude->wszFilterMask );

            InsertTailList( &NewEntry->ExcludeMaskListHead, &newExclude->ListEntry );
        }

        InsertTailList( &NewFilter->ListHead, &NewEntry->ListEntry );
    }

    ProcessFilter_Release( OldFilter );

    return NewFilter;
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS InitializeProcessFilter()
{
    PROCESS_FILTER_KN* Filter = ( PROCESS_FILTER_KN* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER_KN ), 'abcd' );
    RtlZeroMemory( Filter, sizeof( PROCESS_FILTER_KN ) );
    InitializeListHead( &Filter->ListHead );
    Filter->RefCount = 0;
    GlobalContext.ProcessFilter = Filter;
    
    return STATUS_SUCCESS;
}

NTSTATUS CloseProcessFilter()
{
    PROCESS_FILTER_KN* Filter = ProcessFilter_Ref();
    PROCESS_FILTER_KN* NewFilter = ( PROCESS_FILTER_KN* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER_KN ), 'abcd' );

    RtlZeroMemory( NewFilter, sizeof( PROCESS_FILTER_KN ) );
    InitializeListHead( &NewFilter->ListHead );
    NewFilter->RefCount = 0;

    InterlockedExchangePointer( &GlobalContext.ProcessFilter, NewFilter );
    ProcessFilter_Release( Filter );

    return STATUS_SUCCESS;
}

NTSTATUS ProcessFilter_Add( __in const PROCESS_FILTER* ProcessFilter )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PROCESS_FILTER_KN* NewFilter = NULL;
    PROCESS_FILTER_ENTRY* FilterItem = NULLPTR;

    do
    {
        if( ProcessFilter == NULLPTR )
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        NewFilter = ProcessFilter_Clone();
        if( NewFilter == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        FilterItem = ( PROCESS_FILTER_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER_ENTRY ), 'abcd' );
        if( FilterItem == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory( FilterItem, sizeof( PROCESS_FILTER_ENTRY ) );

        FilterItem->ProcessId               = ProcessFilter->uProcessId;
        RtlStringCchCopyW( FilterItem->ProcessMask, MAX_PATH, ProcessFilter->wszProcessMask );
        FilterItem->Flags                   = ProcessFilter->isChildRecursive;

        FilterItem->FileIOFlags             = ProcessFilter->uFileIOTypes;
        FilterItem->FileNotifyFlags         = ProcessFilter->uFileNotifyTypes;

        InitializeListHead( &FilterItem->IncludeMaskListHead );
        InitializeListHead( &FilterItem->ExcludeMaskListHead );

        ProcessFilter_AddMask( &FilterItem->IncludeMaskListHead, ProcessFilter->wszIncludeMask );
        ProcessFilter_AddMask( &FilterItem->ExcludeMaskListHead, ProcessFilter->wszExcludeMask );

        InsertTailList( &NewFilter->ListHead, &FilterItem->ListEntry );

        InterlockedExchangePointer( &GlobalContext.ProcessFilter, NewFilter );

    } while( false );

    ProcessFilter_Release( NewFilter );

    return Status;
}

NTSTATUS ProcessFilter_AddMask( LIST_ENTRY* ListHead, const WCHAR* wszFilterMask )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    size_t Length = 0;
    WCHAR* Mask = NULL;

    do
    {
        if( ListHead == NULLPTR || wszFilterMask == NULLPTR )
            break;

        if( ( Length = nsUtils::strlength( wszFilterMask ) ) == 0 )
        {
            Status = STATUS_SUCCESS;
            break;
        }

        Mask = ( WCHAR* )ExAllocatePool( NonPagedPool, Length * sizeof( WCHAR ) );
        if( Mask == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory( Mask, Length * sizeof( WCHAR ) );

        size_t uMask = 0;
        for( size_t idx = 0; idx < Length; ++idx )
        {
            if( Mask[ idx ] == L'|' )
            {

                auto entry = ( PF_FILTER_MASK_ENTRY* )ExAllocatePool( NonPagedPool, sizeof( PF_FILTER_MASK_ENTRY ) );
                if( entry == NULL )
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlZeroMemory( entry, sizeof( PF_FILTER_MASK_ENTRY ) );

                RtlStringCchCopyW( entry->wszFilterMask, MAX_PATH, Mask );
                nsUtils::UpperWString( entry->wszFilterMask );
                
                InsertTailList( ListHead, &entry->ListEntry );

                uMask = 0;
                RtlZeroMemory( Mask, Length * sizeof( WCHAR ) );
            }
            else
            {
                Mask[ uMask ] = wszFilterMask[ idx ];
                uMask += 1;
            }
        }

    } while( false );

    if( Mask != NULLPTR )
        ExFreePool( Mask );

    return Status;
}

NTSTATUS ProcessFilter_Remove( __in ULONG ProcessId, __in WCHAR* ProcessMask )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    PROCESS_FILTER_KN* NewFilter = NULL;

    do
    {
        NewFilter = ProcessFilter_Clone();
        if( NewFilter == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        LIST_ENTRY* Head = &NewFilter->ListHead;
        for( LIST_ENTRY* current = Head->Flink; current != Head; current = current->Flink )
        {
            PROCESS_FILTER_ENTRY* entry = CONTAINING_RECORD( current, PROCESS_FILTER_ENTRY, ListEntry );

            if( ( ProcessId > 0 && entry->ProcessId > 0 && entry->ProcessId == ProcessId ) || 
                ( ProcessMask != NULLPTR && _wcsicmp( entry->ProcessMask, ProcessMask ) == 0 ) )
            {
                RemoveEntryList( &entry->ListEntry );

                ProcessFilter_RemoveMask( &entry->IncludeMaskListHead );
                ProcessFilter_RemoveMask( &entry->ExcludeMaskListHead );

                ExFreePool( entry );
                Status = STATUS_SUCCESS;
                break;
            }
        }

        InterlockedExchangePointer( &GlobalContext.ProcessFilter, NewFilter );

    } while( false );

    ProcessFilter_Release( NewFilter );

    return Status;
}

void ProcessFilter_RemoveMask( LIST_ENTRY* ListHead )
{
    LIST_ENTRY* Head = ListHead;
    for( LIST_ENTRY* current = Head->Flink; current != Head; current = current->Flink )
    {
        PF_FILTER_MASK_ENTRY* entry = CONTAINING_RECORD( current, PF_FILTER_MASK_ENTRY, ListEntry );

        RemoveEntryList( &entry->ListEntry );
        ExFreePool( entry );
    }
}

DWORD ProcessFilter_Count()
{
    DWORD ProcessFilterCount = 0;
    auto ProcessFilter = ProcessFilter_Ref();

    do
    {
        if( ProcessFilter == NULLPTR )
            break;

        LIST_ENTRY* Head = &ProcessFilter->ListHead;

        for( LIST_ENTRY* Current = Head->Flink; Current != Head; Current = Current->Flink )
            ProcessFilterCount++;

    } while( false );

    ProcessFilter_Release( ProcessFilter );
    return ProcessFilterCount;
}

void ProcessFilter_CloseHandle( PVOID ProcessFilterHandle )
{
    ProcessFilter_Release( (PROCESS_FILTER_KN*)ProcessFilterHandle );
}

NTSTATUS ProcessFilter_Match( ULONG ProcessId, const WCHAR* ProcessName )
{
    return ProcessFilter_Match( ProcessId, ProcessName, NULLPTR, NULLPTR );
}

NTSTATUS ProcessFilter_Match( ULONG ProcessId, const WCHAR* ProcessName, const WCHAR* FileFullPath )
{
    return ProcessFilter_Match( ProcessId, ProcessName, FileFullPath, NULLPTR, NULLPTR );
}

NTSTATUS ProcessFilter_Match( ULONG ProcessId, const WCHAR* ProcessName, PVOID* ProcessFilterHandle, PVOID* ProcessFilter )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    auto Filter = ProcessFilter_Ref();

    do
    {
        LIST_ENTRY* Head = &Filter->ListHead;

        for( LIST_ENTRY* Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            PROCESS_FILTER_ENTRY* Item = CONTAINING_RECORD( Current, PROCESS_FILTER_ENTRY, ListEntry );

            do
            {
                if( Item->ProcessId > 0 && Item->ProcessId == ProcessId )
                {
                    Status = STATUS_SUCCESS;
                    break;
                }

                if( ProcessName != NULLPTR &&
                    nsUtils::WildcardMatch_straight( ProcessName, Item->ProcessMask ) == true )
                {
                    Status = STATUS_SUCCESS;
                    break;
                }

            } while( false );

            if( Status == STATUS_SUCCESS )
            {
                if( ProcessFilterHandle != NULLPTR && ProcessFilter != NULLPTR )
                {
                    *ProcessFilterHandle = Filter;
                    *ProcessFilter = Item;
                }

                break;
            }
        }

    } while( false );

    // caller must be CloseHandle ProcessFilterHandle
    if( ProcessFilterHandle == NULLPTR )
        ProcessFilter_Release( Filter );

    return Status;
}

NTSTATUS ProcessFilter_Match( ULONG ProcessId, const WCHAR* ProcessName, const WCHAR* FileFullPath, PVOID* ProcessFilterHandle, PVOID* ProcessFilter )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    auto Filter = ProcessFilter_Ref();

    do
    {
        bool isMatch = false;
        LIST_ENTRY* Head = &Filter->ListHead;

        for( LIST_ENTRY* Current = Head->Flink; Current != Head; Current = Current->Flink )
        {
            PROCESS_FILTER_ENTRY* Item = CONTAINING_RECORD( Current, PROCESS_FILTER_ENTRY, ListEntry );

            if( Item->ProcessId > 0 && Item->ProcessId == ProcessId )
                isMatch = true;
            else if( ProcessName != NULLPTR &&
                     nsUtils::WildcardMatch_straight( ProcessName, Item->ProcessMask ) == true )
                isMatch = true;

            if( isMatch == false )
                continue;

            do
            {
                if( ProcessFilter_MatchMask( &Item->ExcludeMaskListHead, FileFullPath ) == STATUS_SUCCESS )
                {
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    break;
                }

                if( ProcessFilter_MatchMask( &Item->IncludeMaskListHead, FileFullPath ) == STATUS_SUCCESS )
                {
                    Status = STATUS_SUCCESS;
                    break;
                }

            } while( false );

            if( Status != STATUS_NOT_FOUND )
            {
                if( ProcessFilterHandle != NULLPTR && ProcessFilter != NULLPTR )
                {
                    *ProcessFilterHandle = Filter;
                    *ProcessFilter = Item;
                }

                break;
            }
        }

    } while( false );

    // caller must be CloseHandle ProcessFilterHandle
    if( ProcessFilterHandle == NULLPTR )
        ProcessFilter_Release( Filter );

    return Status;
}

FORCEINLINE NTSTATUS ProcessFilter_MatchMask( LIST_ENTRY* ListHead, const WCHAR* FileFullPath )
{
    NTSTATUS Status = STATUS_NOT_FOUND;

    do
    {
        for( LIST_ENTRY* Current = ListHead->Flink; Current != ListHead; Current = Current->Flink )
        {
            PF_FILTER_MASK_ENTRY* Item = CONTAINING_RECORD( Current, PF_FILTER_MASK_ENTRY, ListEntry );

            if( nsUtils::WildcardMatch_straight( FileFullPath, Item->wszFilterMask ) == true )
            {
                Status = STATUS_SUCCESS;
                break;
            }
        }

    } while( false );

    return Status;
}
