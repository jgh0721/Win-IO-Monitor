#ifndef HDR_WINIOISOLATION_API
#define HDR_WINIOISOLATION_API

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winternl.h>
#include <thread>
#include <memory>
#include <fltUser.h>
#include <fltUserStructures.h>

#include <vector>
#include <string>
#include <atomic>

#include "WinIOIsolation_Event.hpp"

#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

typedef BOOL( *MessageCallbackRoutine )( IN MSG_SEND_PACKET* Incoming, IN OUT MSG_REPLY_PACKET* Outgoing, IN PVOID Context );
typedef VOID( *DisconnectCallbackRoutine )( IN PVOID Context );

/*!
    1. ConnectTo
    2. SetDriverConfig
    3. RegisterMessageCallback
    4. SetDriverStatus

    Set ProcessId(All), Rename, all Encrypted File
*/

class CWinIOIsolator
{
public:
    HRESULT                             ConnectToIo();
    HRESULT                             ConnectToProc();
    void                                DisconnectIo();
    void                                DisconnectProc();

    DWORD                               SetDriverConfig( __in DRIVER_CONFIG* DriverConfig, __in ULONG Size );
    DWORD                               GetDriverConfig( __in DRIVER_CONFIG* DriverConfig, __in ULONG Size );

    bool                                RegisterMessageCallback( ULONG ThreadCount, PVOID Context,
                                                                 MessageCallbackRoutine MessageCallback, DisconnectCallbackRoutine DisconnectCallback );

    DWORD                               SetDriverStatus( __in BOOLEAN IsRunning );
    DWORD                               GetDriverStatus( __in BOOLEAN* IsRunning );

    ///////////////////////////////////////////////////////////////////////////
    /// Global Filter Mgmt

    DWORD                               AddGlobalFilterMask( __in const wchar_t* wszFilterMask, __in bool isInclude );
    DWORD                               DelGlobalFilterMask( __in const wchar_t* wszFilterMask, __in bool isInclude );
    DWORD                               GetGlobalFilterMaskCnt( __in bool isInclude, __out ULONG* Count );
    DWORD                               ResetGlobalFilter();
    DWORD                               GetGlobalFilterMask( __inout PVOID Buffer, __in ULONG BufferSize, __in bool isInclude );

    ///////////////////////////////////////////////////////////////////////////
    /// Process Filter Mgmt

    DWORD                               AddProcessFilter( __in const USER_PROCESS_FILTER& ProcessFilter );
    DWORD                               DelProcessFilterByID( __in const USER_PROCESS_FILTER& ProcessFilter );
    DWORD                               DelProcessFilterByPID( __in ULONG ProcessId );
    DWORD                               DelProcessFilterByFilterMask( __in const wchar_t* wszFilterMask );
    DWORD                               AddProcessFilterItem( __in const GUID& Id, __in const USER_PROCESS_FILTER_ENTRY& ProcessFilterItem );
    DWORD                               DelProcessFilterItem( __in const GUID& Id, __in const USER_PROCESS_FILTER_ENTRY& ProcessFilterItem );
    DWORD                               ResetProcessFilter();

    ///////////////////////////////////////////////////////////////////////////////
    /// File Management

    enum TyEnFileType
    {
        FILETYPE_UNK_TYPE,  // NORMAL FILE
        FILETYPE_NOR_TYPE,  // Type 1
        FILETYPE_RAR_TYPE,  // Type 2
        FILETYPE_STB_TYPE   // Type 3
    };

    /*!
        @param FileType TyEnFileType
    */
    DWORD                               GetFileType( __in const wchar_t* wszFileFullPath, __out ULONG* FileType );
    DWORD                               SetFileSolutionMetaData( __in const wchar_t* wszFileFullPath, __inout PVOID Buffer, __inout ULONG* BufferSize );
    DWORD                               GetFileSolutionMetaData( __in const wchar_t* wszFileFullPath, __inout PVOID Buffer, __inout ULONG* BufferSize );
    /*!
        드라이버에 요청하지 않고, API 에서 직접 파일을 확인
    */
    DWORD                               GetFileTypeSelf( __in const wchar_t* wszFileFullPath, __out ULONG* FileType );
    DWORD                               SetFileSolutionMetaDataSelf( __in const wchar_t* wszFileFullPath, __inout PVOID Buffer, __inout ULONG* BufferSize );
    DWORD                               GetFileSolutionMetaDataSelf( __in const wchar_t* wszFileFullPath, __inout PVOID Buffer, __inout ULONG* BufferSize );

    DWORD                               EncryptFileByDriver( __in const wchar_t* wszSrcFileFullPath, __in const wchar_t* wszDstFileFullPath,
                                                             __in_opt PVOID SolutionMetaData = NULL, __in_opt ULONG* SolutionMetaDataSize = NULL,
                                                             __in_opt ENCRYPT_CONFIG* EncryptConfig = NULL );
    DWORD                               DecryptFileByDriver( __in const wchar_t* wszSrcFileFullPath, __in const wchar_t* wszDstFileFullPath,
                                                             __in_opt ENCRYPT_CONFIG* EncryptConfig = NULL,
                                                             __out_opt PVOID SolutionMetaData = NULL, __out_opt ULONG* SolutionMetaDataSize = NULL );

private:

    HANDLE                              retrieveDevice();
    void                                closeDevice( __in HANDLE hDevice );
    static unsigned int WINAPI          messageWorker( PVOID Param );

    static const int                    MAX_CLIENT_CONNECTION = 16;

    struct Context
    {
        CWinIOIsolator*                 self = NULL;
        HANDLE                          hPort = NULL;
        HANDLE                          hWorker = NULL;
    };

    std::atomic_bool                    isExit{ false };
    std::vector< Context* >             vecIOWorkContext;
    std::vector< Context* >             vecProcWorkContext;

    std::pair< MessageCallbackRoutine, PVOID > MessageCallbackFn{ NULL, NULL };
    std::pair< DisconnectCallbackRoutine, PVOID > DisconnectCallbackFn{ NULL, NULL };
};

DWORD CollectNotifyEventItmes( __inout PVOID Buffer, __in ULONG BufferSize, __out ULONG* WrittenItemCount );

#endif // HDR_WINIOISOLATION_API