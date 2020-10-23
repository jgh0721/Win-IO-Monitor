#ifndef HDR_WINIOISOLATION_API
#define HDR_WINIOISOLATION_API

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

#include "WinIOIsolation_Event.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

/*!
    1. ConnectTo
    2. SetDriverConfig
    3. SetDriverStatus
    
    Set ProcessId(All), Rename, all Encrypted File
*/

HRESULT ConnectTo();
HRESULT Disconnect();

DWORD SetDriverConfig( __in DRIVER_CONFIG* DriverConfig );
DWORD GetDriverConfig( __in DRIVER_CONFIG* DriverConfig );

// 
DWORD SetDriverStatus( __in BOOLEAN IsRunning );
DWORD GetDriverStatus( __in BOOLEAN* IsRunning );

DWORD AddGlobalFilterMask( __in const wchar_t* wszFilterMask, __in bool isInclude );
DWORD DelGlobalFilterMask( __in const wchar_t* wszFilterMask, __in bool isInclude );
DWORD GetGlobalFilterMaskCnt( __in bool isInclude, __out ULONG* Count );
DWORD ResetGlobalFilter();


DWORD CollectNotifyEventItmes( __inout PVOID Buffer, __in ULONG BufferSize, __out ULONG* WrittenItemCount );

#endif // HDR_WINIOISOLATION_API