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
#include <thread>
#include <memory>
#include <fltUser.h>
#include <fltUserStructures.h>
#endif

#include "WinIOIsolation_Event.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

#define MAX_CLIENT_CONNECTION (16)

/*!
    1. ConnectTo
    2. SetDriverConfig
    3. SetDriverStatus
    
    Set ProcessId(All), Rename, all Encrypted File
*/

HRESULT ConnectTo( __in unsigned int uThreadCount = 0 );
HRESULT Disconnect();

DWORD SetDriverConfig( __in DRIVER_CONFIG* DriverConfig );
DWORD GetDriverConfig( __in DRIVER_CONFIG* DriverConfig );

// 
DWORD SetDriverStatus( __in BOOLEAN IsRunning );
DWORD GetDriverStatus( __in BOOLEAN* IsRunning );

typedef BOOL( *MessageCallbackRoutine )( IN MSG_SEND_PACKET* Incoming, IN OUT MSG_REPLY_PACKET* Outgoing );
typedef VOID( *DisconnectCallbackRoutine )( );

bool  RegisterMessageCallback( MessageCallbackRoutine MessageCallback, DisconnectCallbackRoutine DisconnectCallback );

DWORD AddGlobalFilterMask( __in const wchar_t* wszFilterMask, __in bool isInclude );
DWORD DelGlobalFilterMask( __in const wchar_t* wszFilterMask, __in bool isInclude );
DWORD GetGlobalFilterMaskCnt( __in bool isInclude, __out ULONG* Count );
DWORD ResetGlobalFilter();

DWORD AddProcessFilter( __in const USER_PROCESS_FILTER& ProcessFilter );
DWORD DelProcessFilterByID( __in const USER_PROCESS_FILTER& ProcessFilter );
DWORD DelProcessFilterByPID( __in ULONG ProcessId );
DWORD DelProcessFilterByFilterMask( __in const wchar_t* wszFilterMask );
DWORD AddProcessFilterItem( __in const GUID& Id, __in const USER_PROCESS_FILTER_ENTRY& ProcessFilterItem );
DWORD DelProcessFilterItem( __in const GUID& Id, __in const USER_PROCESS_FILTER_ENTRY& ProcessFilterItem );
DWORD ResetProcessFilter();

///////////////////////////////////////////////////////////////////////////////
/// File Management

enum TyEnFileType
{
    FILETYPE_UNK_TYPE,
    FILETYPE_NOR_TYPE,
    FILETYPE_RAR_TYPE,
    FILETYPE_STB_TYPE
};

/*!
    @param FileType TyEnFileType
*/
DWORD GetFileType( __in const wchar_t* wszFileFullPath, __out ULONG* FileType );
DWORD SetFileSolutionMetaData( __in const wchar_t* wszFileFullPath, __inout PVOID Buffer, __inout ULONG* BufferSize );
DWORD GetFileSolutionMetaData( __in const wchar_t* wszFileFullPath, __inout PVOID Buffer, __inout ULONG* BufferSize );

DWORD EncryptFileByDriver( __in const wchar_t* wszSrcFileFullPath, __in const wchar_t* wszDstFileFullPath );
DWORD DecryptFileByDriver( __in const wchar_t* wszSrcFileFullPath, __in const wchar_t* wszDstFileFullPath );

DWORD CollectNotifyEventItmes( __inout PVOID Buffer, __in ULONG BufferSize, __out ULONG* WrittenItemCount );

#endif // HDR_WINIOISOLATION_API