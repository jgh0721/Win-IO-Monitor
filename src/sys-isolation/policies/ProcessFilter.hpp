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
    ULONG           ProcessId;
    WCHAR           ProcessFilterMask[ MAX_PATH ];

    LIST_ENTRY      IncludeListHead;
    LIST_ENTRY      ExcludeListHead;

    LIST_ENTRY      ListEntry;

} PROCESS_FILTER_ENTRY, *PPROCESS_FILTER_ENTRY;

typedef struct _PROCESS_FILTER_MASK_ENTRY
{
    WCHAR           FilterMask[ MAX_PATH ];

    LIST_ENTRY      ListEntry;
    
} PROCESS_FILTER_MASK_ENTRY, *PPROCESS_FILTER_MASK_ENTRY;

NTSTATUS InitializeProcessFilter();
NTSTATUS UninitializeProcessFilter();

/*!
    Must allocate from NonPagedPool
*/
NTSTATUS ProcessFilter_Add( __in PROCESS_FILTER_ENTRY* PFilterItem );
NTSTATUS ProcessFilter_Remove( __in_opt ULONG ProcessId, __in_z_opt const WCHAR* ProcessNameMask );
/*!

    Must call ProcessFilter_Close after use 
*/
NTSTATUS ProcessFilter_Match( __in_opt ULONG ProcessId, __in_opt TyGenericBuffer<WCHAR>* ProcessFilePath, __out_opt HANDLE* ProcessFilter, __out_opt PROCESS_FILTER_ENTRY** MatchItem );
/*!

    @param IsIncludeMatch if criteria was matched, this variable set match type value
*/
NTSTATUS ProcessFilter_SubMatch( __in PROCESS_FILTER_ENTRY* PFilterItem, __in TyGenericBuffer<WCHAR>* FilePath, __out bool* IsIncludeMatch );

void ProcessFilter_Close( __in HANDLE ProcessFilterHandle );

#endif // HDR_WINIOISOLATION_PROCESS_FILTER