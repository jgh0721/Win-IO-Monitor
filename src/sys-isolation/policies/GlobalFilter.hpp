#ifndef HDR_WINIOISOLATION_GLOBAL_FILTER
#define HDR_WINIOISOLATION_GLOBAL_FILTER

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*!
    Global Filter

    1. Include
    2. Exclude

    * To Apply Priority

        1. Exclude
            If Match Exclude Then Dont Set Target
        2. If Include Is Empty, All is Target
           If Include Is not Empty, Match with Include

    Low                         High
    Include                     Exclude
    Not Set ProcessInfo         Set Process Info

    Match, STATUS_SUCCESS;
    Not Match, STATUS_NOT_FOUND
    Specified Type Is Empty, STATUS_NO_DATA_DETECTED
*/

typedef struct _GLOBAL_FILTER_ENTRY
{
    // *, ? support 
    WCHAR                   FilterMask[ MAX_PATH ];

    LIST_ENTRY              ListEntry;
    
} GLOBAL_FILTER_ENTRY, *PGLOBAL_FILTER_ENTRY;

NTSTATUS InitializeGlobalFilter();
NTSTATUS UninitializeGlobalFilter();

NTSTATUS GlobalFilter_Add( __in const WCHAR* FilterMask, bool isInclude );
NTSTATUS GlobalFilter_Remove( __in const WCHAR* FilterMask );
NTSTATUS GlobalFilter_Match( __in const WCHAR* FileName, bool IsInclude );
NTSTATUS GlobalFilter_Reset();

#endif // HDR_WINIOISOLATION_GLOBAL_FILTER