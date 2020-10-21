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

#define PROCESS_NOTIFY_CREATION_TERMINATION     0x1
#define PROCESS_DENY_CREATION                   0x2
#define PROCESS_DENY_TERMINATION                0x4
#define PROCESS_APPLY_CHILD_PROCESS             0x8

typedef struct _PROCESS_FILTER_ENTRY
{
    ULONG           ProcessId;
    WCHAR           ProcessFilterMask[ MAX_PATH ];
    ULONG           ProcessFilter;                  // PROCESS_XXXX 

    LIST_ENTRY      IncludeListHead;
    LIST_ENTRY      ExcludeListHead;

    LIST_ENTRY      ListEntry;

} PROCESS_FILTER_ENTRY, *PPROCESS_FILTER_ENTRY;

typedef struct _PROCESS_FILTER_MASK_ENTRY
{
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