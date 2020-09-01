#ifndef HDR_WINIOMONITOR_API
#define HDR_WINIOMONITOR_API

#if defined(USE_ON_KERNEL)
#include "fltBase.hpp"
#else

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winternl.h>
#include <fltUser.h>
#include <fltUserStructures.h>
#endif

#include "WinIOMonitor_Event.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*!
 *
 * 2020-08-23
 *
 *      SetTimeOut / GetTimeOut 
 */

#define FILTER_MASK_MAX_LENGTH 8192

enum TyEnFilterType
{
    FILTER_NONE                 = 0x0,
    FILTER_FS_CONTROL           = 0x1,
    FILTER_FS_MONITOR           = 0x2,

    FILTER_PROC_CONTROL         = 0x10,
    FILTER_PROC_MONITOR         = 0x20,

    FILTER_REG_CONTROL          = 0x100,
    FILTER_REG_MONITOR          = 0x200
};

typedef enum TyEnMessageCategory
{
    MESSAGE_UNKNOWN             ,
    MESSAGE_FILESYSTEM          ,
    MESSAGE_PROCESS             ,
    MESSAGE_REGISTRY
};

typedef BOOL( WINAPI* MessageCallback )( __in MSG_SEND_PACKET* msg, __inout MSG_REPLY_PACKET* reply );
typedef void( WINAPI* DisconnectCallback )( );

///////////////////////////////////////////////////////////////////////////////

DWORD ConnectTo();
DWORD Disconnect();

DWORD SetTimeOut( __in DWORD TimeOutMs );
DWORD GetTimeOut( __out DWORD* TimeOutMs );

// combinations of TyEnFilterType
DWORD SetFilterType( __in ULONG FilterTypeFlags );

DWORD RegisterCallback( __in ULONG ThreadCount, __in MessageCallback pfnMessageCallback, __in DisconnectCallback pfnDisconnectCallback );

DWORD SetStartFiltering( __in BOOLEAN IsFiltering );
DWORD GetStartFiltering( __out BOOLEAN* IsFiltering );

DWORD AddGlobalFileFilterMask( __in const wchar_t* wszFilterMask, __in bool isInclude );
DWORD RemoveGlobalFileFilterMask( __in const wchar_t* wszFilterMask, __in bool isInclude );

typedef struct _PROCESS_FILTER
{
    ULONG                   uProcessId;                         // uProcessId or wszProcessMask 
    WCHAR                   wszProcessMask[ MAX_PATH ];         // mask can use wildcard(*)

    ULONG                   uFileIOTypes;
    ULONG                   uFileNotifyTypes;                   // combinations of TyEnFileNotifyType 

    BOOLEAN                 isChildRecursive;

    WCHAR                   wszIncludeMask[ FILTER_MASK_MAX_LENGTH ];   // mask can use wildcard(*), separator '|', must be ended with sep
    WCHAR                   wszExcludeMask[ FILTER_MASK_MAX_LENGTH ];   // mask can use wildcard(*), separator '|', must be ended with sep

} PROCESS_FILTER, *PPROCESS_FILTER;

DWORD AddProcessFileFilterMask( __in const PROCESS_FILTER* ProcessFilter );
DWORD GetProcessFileFilterCount( __out DWORD* FilterCount );
DWORD RemoveProcessFileFilterMask( __in const wchar_t* wszFilterMask );
DWORD RemoveProcessFileFilterMask( __in DWORD ProcessId );
DWORD ResetProcessFileFilterMask();

#endif // HDR_WINIOMONITOR_API