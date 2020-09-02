#ifndef HDR_POLICY_PROCESS_FILTER
#define HDR_POLICY_PROCESS_FILTER

#include "fltBase.hpp"

#include "WinIOMonitor_API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

typedef struct _PROCESS_FILTER_ENTRY
{
    ULONG                       ProcessId;
    WCHAR                       ProcessMask[ MAX_PATH ];
    ULONG                       Flags;

    ULONG                       FileIOFlags;
    ULONG                       FileNotifyFlags;

    LIST_ENTRY                  IncludeMaskListHead;
    LIST_ENTRY                  ExcludeMaskListHead;

    LIST_ENTRY                  ListEntry;

} PROCESS_FILTER_ENTRY, *PPROCESS_FILTER_ENTRY;

typedef struct _PROCESS_FILTER_MASK_ENTRY
{
    WCHAR                   wszFilterMask[ MAX_PATH ];

    LIST_ENTRY              ListEntry;
} PF_FILTER_MASK_ENTRY, * PPF_FILTER_MASK_ENTRY;

NTSTATUS InitializeProcessFilter();
NTSTATUS CloseProcessFilter();

NTSTATUS ProcessFilter_Add( __in const PROCESS_FILTER* ProcessFilter );
NTSTATUS ProcessFilter_AddMask( __in LIST_ENTRY* ListHead, __in const WCHAR* wszFilterMask );
NTSTATUS ProcessFilter_Remove( __in ULONG ProcessId, __in WCHAR* ProcessMask );
void ProcessFilter_RemoveMask( __in LIST_ENTRY* ListHead );
DWORD ProcessFilter_Count();
void ProcessFilter_CloseHandle( __in PVOID ProcessFilterHandle );

// include mask Match, none, exclude mask match
// STATUS_SUCCESS, STATUS_NOT_FOUND, STATUS_OBJECT_NAME_NOT_FOUND

NTSTATUS ProcessFilter_Match( __in_opt ULONG ProcessId, __in_opt const WCHAR* ProcessName );
NTSTATUS ProcessFilter_Match( __in_opt ULONG ProcessId, __in_opt const WCHAR* ProcessName, __in const WCHAR* FileFullPath );

NTSTATUS ProcessFilter_Match( __in_opt ULONG ProcessId, __in_opt const WCHAR* ProcessName, __out_opt PVOID* ProcessFilterHandle, __out_opt PVOID* ProcessFilter  );
NTSTATUS ProcessFilter_Match( __in_opt ULONG ProcessId, __in_opt const WCHAR* ProcessName, __in const WCHAR* FileFullPath, __out_opt PVOID* ProcessFilterHandle, __out_opt PVOID* ProcessFilter );

FORCEINLINE NTSTATUS ProcessFilter_MatchMask( __in LIST_ENTRY* ListHead, __in const WCHAR* FileFullPath );

#endif // HDR_POLICY_PROCESS_FILTER