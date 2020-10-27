#ifndef HDR_WINIOISOLATION_PROCESS_FILTER
#define HDR_WINIOISOLATION_PROCESS_FILTER

#include "fltBase.hpp"

#include "utilities/bufferMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*!
    Process Filter

    Select Target : Process Id Or Process Name Mask( *, ? )

    Include Or Exclude Files



    Match, STATUS_SUCCESS;
    Not Match, STATUS_NOT_FOUND
    Specified Type Is Empty, STATUS_NO_DATA_DETECTED
*/

typedef struct _PROCESS_FILTER_ENTRY
{
    UUID            Id;
    ULONG           ProcessId;
    WCHAR           ProcessFilterMask[ MAX_PATH ];
    ULONG           ProcessFilter;                  // PROCESS_FILTER_TYPE( WinIOIsolation_Event.hpp ) 의 조합 
    
    LIST_ENTRY      IncludeListHead;
    LIST_ENTRY      ExcludeListHead;

    LIST_ENTRY      ListEntry;

} PROCESS_FILTER_ENTRY, *PPROCESS_FILTER_ENTRY;

typedef struct _PROCESS_FILTER_MASK_ENTRY
{
    UUID            Id;
    ULONG           FilterCategory;                 // MSG_CATEGORY
    ULONGLONG       FilterType;                     // MSG_FS_TYPE or MSG_FS_NOTIFY_TYPE or MSG_PROC_TYPE
    WCHAR           FilterMask[ MAX_PATH ];
    BOOLEAN         IsManagedFile;

    LIST_ENTRY      ListEntry;
    
} PROCESS_FILTER_MASK_ENTRY, *PPROCESS_FILTER_MASK_ENTRY;

NTSTATUS InitializeProcessFilter();
NTSTATUS UninitializeProcessFilter();

/*!
    Must allocate from NonPagedPool
*/
NTSTATUS ProcessFilter_Add( __in PROCESS_FILTER_ENTRY* PFilterItem );
NTSTATUS ProcessFilter_AddEntry( __in UUID* Id, __in PROCESS_FILTER_MASK_ENTRY* Entry, __in BOOLEAN IsInclude );
NTSTATUS ProcessFilter_Remove( __in UUID* Id );
NTSTATUS ProcessFilter_Remove( __in_opt ULONG ProcessId, __in_z_opt const WCHAR* ProcessNameMask );
NTSTATUS ProcessFilter_RemoveEntry( __in UUID* ParentId, __in UUID* EntryId, __in BOOLEAN IsInclude );

/*!
    Must call ProcessFilter_Close after use 
*/
NTSTATUS ProcessFilter_Match( __in_opt ULONG ProcessId, __in_opt TyGenericBuffer<WCHAR>* ProcessFilePath, __out_opt HANDLE* ProcessFilter, __out_opt PROCESS_FILTER_ENTRY** MatchItem );
/*!

    @param IsIncludeMatch if criteria was matched, this variable set match type value
*/
NTSTATUS ProcessFilter_SubMatch( __in PROCESS_FILTER_ENTRY* PFilterItem, __in TyGenericBuffer<WCHAR>* FilePath, __out bool* IsIncludeMatch, __out_opt PPROCESS_FILTER_MASK_ENTRY* MatchItem );

void ProcessFilter_Close( __in HANDLE ProcessFilterHandle );
NTSTATUS ProcessFilter_Reset();

#endif // HDR_WINIOISOLATION_PROCESS_FILTER