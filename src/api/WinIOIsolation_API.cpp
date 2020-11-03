#include "WinIOIsolation_API.hpp"

#include "WinIOIsolation_IOCTL.hpp"
#include "WinIOIsolation_Names.hpp"

#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <atomic>

#pragma comment( lib, "fltlib" )

#pragma warning( disable: 4996 )

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

HRESULT CWinIOIsolator::ConnectToIo()
{
    Context* context = new Context;
    memset( context, '0', sizeof( Context ) );

    context->self = this;
    
    HRESULT hRet = FilterConnectCommunicationPort( PORT_NAME, 0,
                                                   NULL, 0, NULL,
                                                   &context->hPort );

    if( FAILED( context->hPort ) )
        return hRet;

    context->hWorker = ( HANDLE )_beginthreadex( NULL, 0,
                                                &messageWorker, context,
                                                0, 0 );

    vecIOWorkContext.push_back( context );
    return hRet;
}

HRESULT CWinIOIsolator::ConnectToProc()
{
    Context* context = new Context;
    memset( context, '0', sizeof( Context ) );

    context->self = this;

    HRESULT hRet = FilterConnectCommunicationPort( PORT_PROC_NAME, 0,
                                                   NULL, 0, NULL,
                                                   &context->hPort );

    if( FAILED( context->hPort ) )
        return hRet;

    context->hWorker = ( HANDLE )_beginthreadex( NULL, 0,
                                                 &messageWorker, context,
                                                 0, 0 );

    vecProcWorkContext.push_back( context );
    return hRet;
}

void CWinIOIsolator::DisconnectIo()
{
}

void CWinIOIsolator::DisconnectProc()
{
}

DWORD CWinIOIsolator::SetDriverConfig( DRIVER_CONFIG* DriverConfig, ULONG Size )
{
    DWORD dwRet = ERROR_INVALID_ACCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        if( DriverConfig == NULL )
            break;

        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice,
                                         IOCTL_SET_DRIVER_CONFIG,
                                         DriverConfig,
                                         Size,
                                         NULL,
                                         0,
                                         ( LPDWORD )&dwRetLen,
                                         NULL );

        if( !bSuccess )
        {
            dwRet = GetLastError();
            break;
        }

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::GetDriverConfig( DRIVER_CONFIG* DriverConfig, ULONG Size )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice,
                                         IOCTL_GET_DRIVER_CONFIG,
                                         NULL,
                                         0,
                                         DriverConfig,
                                         Size,
                                         ( LPDWORD )&dwRetLen,
                                         NULL );

        if( !bSuccess )
        {
            dwRet = GetLastError();
            break;
        }

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

bool CWinIOIsolator::RegisterMessageCallback( ULONG ThreadCount, PVOID Context, MessageCallbackRoutine MessageCallback, DisconnectCallbackRoutine DisconnectCallback )
{
    MessageCallbackFn.first = MessageCallback;
    MessageCallbackFn.second = Context;

    DisconnectCallbackFn.first = DisconnectCallback;
    DisconnectCallbackFn.second = Context;

    if( ThreadCount == 0 )
        ThreadCount = std::thread::hardware_concurrency();

    if( ThreadCount < 4 )
        ThreadCount = 4;

    if( ThreadCount >= MAX_CLIENT_CONNECTION )
        ThreadCount = MAX_CLIENT_CONNECTION - 1;

    if( vecIOWorkContext.empty() == false )
    {
        for( int idx = 0; idx < ThreadCount; ++idx )
            ConnectToIo();
    }

    if( vecProcWorkContext.empty() == false )
    {
        if( ThreadCount > 4 )
            ThreadCount = 4;

        for( int idx = 0; idx < ThreadCount; ++idx )
            ConnectToProc();
    }

    return true;
}

DWORD CWinIOIsolator::SetDriverStatus( BOOLEAN IsRunning )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice,
                                         IOCTL_SET_DRIVER_STATUS,
                                         &IsRunning,
                                         sizeof( BOOLEAN ),
                                         NULL,
                                         0,
                                         ( LPDWORD )&dwRetLen,
                                         NULL );

        if( !bSuccess )
        {
            dwRet = GetLastError();
            break;
        }

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::GetDriverStatus( BOOLEAN* IsRunning )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice,
                                         IOCTL_GET_DRIVER_STATUS,
                                         NULL,
                                         0,
                                         IsRunning,
                                         sizeof( BOOLEAN ),
                                         ( LPDWORD )&dwRetLen,
                                         NULL );

        if( !bSuccess )
        {
            dwRet = GetLastError();
            break;
        }

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

///////////////////////////////////////////////////////////////////////////
/// Global Filter Mgmt

DWORD CWinIOIsolator::AddGlobalFilterMask( const wchar_t* wszFilterMask, bool isInclude )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    WCHAR* wszBuffer = NULL;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        size_t RequiredSize = 0;
        RequiredSize += sizeof( WCHAR );        // 0 = Exclude, 1 = Include
        RequiredSize += sizeof( WCHAR );        // separator '|'
        RequiredSize += wcslen( wszFilterMask ) * sizeof( WCHAR );
        RequiredSize += sizeof( WCHAR );        // NULL CHAR

        wszBuffer = ( WCHAR* )malloc( RequiredSize );
        memset( wszBuffer, '\0', RequiredSize );
        swprintf( wszBuffer, L"%d|%s", isInclude == true ? TRUE : FALSE, wszFilterMask );

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_ADD_GLOBAL_POLICY,
                                         wszBuffer, ( DWORD )RequiredSize,
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( wszBuffer != NULL )
        free( wszBuffer );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::DelGlobalFilterMask( const wchar_t* wszFilterMask, bool isInclude )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    WCHAR* wszBuffer = NULL;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        size_t RequiredSize = 0;
        RequiredSize += sizeof( WCHAR );        // 0 = Exclude, 1 = Include
        RequiredSize += sizeof( WCHAR );        // separator '|'
        RequiredSize += wcslen( wszFilterMask ) * sizeof( WCHAR );
        RequiredSize += sizeof( WCHAR );        // NULL CHAR

        wszBuffer = ( WCHAR* )malloc( RequiredSize );
        memset( wszBuffer, '\0', RequiredSize );
        swprintf( wszBuffer, L"%d|%s", isInclude == true ? TRUE : FALSE, wszFilterMask );

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_DEL_GLOBAL_POLICY_BY_MASK,
                                         wszBuffer, ( DWORD )RequiredSize,
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( wszBuffer != NULL )
        free( wszBuffer );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::GetGlobalFilterMaskCnt( bool isInclude, ULONG* Count )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOLEAN IsInclude = isInclude == true ? TRUE : FALSE;
        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_GET_GLOBAL_POLICY_COUNT,
                                         &IsInclude, sizeof( BOOLEAN ),
                                         Count, sizeof( ULONG ),
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::ResetGlobalFilter()
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_RST_GLOBAL_POLICY,
                                         NULL, 0,
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );


        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::GetGlobalFilterMask( PVOID Buffer, ULONG BufferSize, bool isInclude )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOLEAN IsInclude = isInclude == true ? TRUE : FALSE;
        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_GET_GLOBAL_POLICY,
                                         &IsInclude, sizeof( BOOLEAN ),
                                         Buffer, BufferSize,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

///////////////////////////////////////////////////////////////////////////
/// Process Filter Mgmt

DWORD CWinIOIsolator::AddProcessFilter( const USER_PROCESS_FILTER& ProcessFilter )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_ADD_PROCESS_POLICY,
                                         ( LPVOID )&ProcessFilter, sizeof( USER_PROCESS_FILTER ),
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::DelProcessFilterByID( const USER_PROCESS_FILTER& ProcessFilter )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_DEL_PROCESS_POLICY_BY_ID,
                                         ( LPVOID )&ProcessFilter, sizeof( USER_PROCESS_FILTER ),
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::DelProcessFilterByPID( ULONG ProcessId )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_DEL_PROCESS_POLICY_BY_PID,
                                         ( LPVOID )&ProcessId, sizeof( ULONG ),
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::DelProcessFilterByFilterMask( const wchar_t* wszFilterMask )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_DEL_PROCESS_POLICY_BY_MASK,
                                         ( LPVOID )&wszFilterMask, ( wcslen( wszFilterMask ) + 1 ) * sizeof( WCHAR ),
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::AddProcessFilterItem( const GUID& Id, const USER_PROCESS_FILTER_ENTRY& ProcessFilterItem )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    PBYTE Buffer = NULL;
    ULONG BufferSize = sizeof( GUID ) + sizeof( USER_PROCESS_FILTER_ENTRY );

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        Buffer = ( PBYTE )malloc( BufferSize );
        RtlZeroMemory( Buffer, BufferSize );

        RtlCopyMemory( Buffer, &Id, sizeof( GUID ) );
        RtlCopyMemory( Buffer + sizeof( GUID ), &ProcessFilterItem, sizeof( USER_PROCESS_FILTER_ENTRY ) );

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_ADD_PROCESS_POLICY_ITEM,
                                         Buffer, BufferSize,
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( Buffer != NULL )
        free( Buffer );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::DelProcessFilterItem( const GUID& Id, const USER_PROCESS_FILTER_ENTRY& ProcessFilterItem )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    PBYTE Buffer = NULL;
    ULONG BufferSize = sizeof( GUID ) + sizeof( USER_PROCESS_FILTER_ENTRY );

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        Buffer = ( PBYTE )malloc( BufferSize );
        RtlZeroMemory( Buffer, BufferSize );

        RtlCopyMemory( Buffer, &Id, sizeof( GUID ) );
        RtlCopyMemory( Buffer + sizeof( GUID ), &ProcessFilterItem, sizeof( USER_PROCESS_FILTER_ENTRY ) );

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_DEL_PROCESS_POLICY_ITEM,
                                         Buffer, BufferSize,
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( Buffer != NULL )
        free( Buffer );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::ResetProcessFilter()
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_RST_PROCESS_POLICY,
                                         NULL, 0,
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

///////////////////////////////////////////////////////////////////////////////
/// File Management

DWORD CWinIOIsolator::GetFileType( const wchar_t* wszFileFullPath, ULONG* FileType )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_FILE_GET_FILE_TYPE,
                                         ( LPVOID )wszFileFullPath, ( wcslen( wszFileFullPath ) + 1 ) * sizeof( WCHAR ),
                                         FileType, sizeof( ULONG ),
                                         ( LPDWORD )&dwRetLen, NULL );


        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::SetFileSolutionMetaData( const wchar_t* wszFileFullPath, PVOID Buffer, ULONG* BufferSize )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    USER_FILE_SOLUTION_DATA* UserFileSolutionData = NULL;

    do
    {
        if( wszFileFullPath == NULL || Buffer == NULL || BufferSize == NULL )
        {
            dwRet = STATUS_INVALID_PARAMETER;
            break;
        }

        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        ULONG RequiredSize = sizeof( USER_FILE_SOLUTION_DATA );
        RequiredSize += ( wcslen( wszFileFullPath ) + 1 ) * sizeof( WCHAR );
        RequiredSize += *BufferSize;

        UserFileSolutionData = ( USER_FILE_SOLUTION_DATA* )malloc( RequiredSize );
        memset( UserFileSolutionData, '\0', RequiredSize );

        UserFileSolutionData->SizeOfStruct = RequiredSize;

        UserFileSolutionData->LengthOfFileName = ( wcslen( wszFileFullPath ) + 1 ) * sizeof( WCHAR );
        UserFileSolutionData->OffsetOfFileName = sizeof( USER_FILE_SOLUTION_DATA );
        swprintf( ( PWCH )( ( PBYTE )( UserFileSolutionData )+UserFileSolutionData->OffsetOfFileName ),
                  L"%s", wszFileFullPath );

        UserFileSolutionData->LengthOfSolutionData = *BufferSize;
        UserFileSolutionData->OffsetOfSolutionData = UserFileSolutionData->OffsetOfFileName + UserFileSolutionData->LengthOfFileName;
        memcpy( ( ( PBYTE )( UserFileSolutionData )+UserFileSolutionData->OffsetOfSolutionData ),
                Buffer, *BufferSize );

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_FILE_SET_SOLUTION_DATA,
                                         ( LPVOID )UserFileSolutionData, RequiredSize,
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

        *BufferSize = dwRetLen;

    } while( false );

    if( UserFileSolutionData != NULL )
        free( UserFileSolutionData );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::GetFileSolutionMetaData( const wchar_t* wszFileFullPath, PVOID Buffer, ULONG* BufferSize )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_FILE_GET_SOLUTION_DATA,
                                         ( LPVOID )wszFileFullPath, ( wcslen( wszFileFullPath ) + 1 ) * sizeof( WCHAR ),
                                         Buffer, *BufferSize,
                                         ( LPDWORD )&dwRetLen, NULL );


        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

        *BufferSize = dwRetLen;

    } while( false );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::GetFileTypeSelf( const wchar_t* wszFileFullPath, ULONG* FileType )
{
    DWORD dwRet = ERROR_SUCCESS;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    do
    {

    } while( false );

    CloseHandle( hFile );
    return dwRet;
}

DWORD CWinIOIsolator::EncryptFileByDriver( const wchar_t* wszSrcFileFullPath, const wchar_t* wszDstFileFullPath, PVOID SolutionMetaData, ULONG* SolutionMetaDataSize, ENCRYPT_CONFIG* EncryptConfig )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    USER_FILE_ENCRYPT* Buffer = NULL;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        ULONG RequiredSize = sizeof( USER_FILE_ENCRYPT );
        RequiredSize += wcslen( wszSrcFileFullPath ) * sizeof( WCHAR ) + sizeof( WCHAR );
        RequiredSize += wcslen( wszDstFileFullPath ) * sizeof( WCHAR ) + sizeof( WCHAR );

        if( SolutionMetaData != NULL && SolutionMetaDataSize != NULL && *SolutionMetaDataSize > 0 )
            RequiredSize += *SolutionMetaDataSize;

        Buffer = ( USER_FILE_ENCRYPT* )malloc( RequiredSize );
        RtlZeroMemory( Buffer, RequiredSize );

        Buffer->SizeOfStruct = RequiredSize;

        Buffer->LengthOfSrcFileFullPath = wcslen( wszSrcFileFullPath ) * sizeof( WCHAR ) + sizeof( WCHAR );
        Buffer->OffsetOfSrcFileFullPath = sizeof( USER_FILE_ENCRYPT );
        wcscpy( ( WCHAR* )Add2Ptr( Buffer, Buffer->OffsetOfSrcFileFullPath ), wszSrcFileFullPath );

        Buffer->LengthOfDstFileFullPath = wcslen( wszDstFileFullPath ) * sizeof( WCHAR ) + sizeof( WCHAR );
        Buffer->OffsetOfDstFileFullPath = Buffer->OffsetOfSrcFileFullPath + Buffer->LengthOfSrcFileFullPath;
        wcscpy( ( WCHAR* )Add2Ptr( Buffer, Buffer->OffsetOfDstFileFullPath ), wszDstFileFullPath );

        if( SolutionMetaData != NULL && SolutionMetaDataSize != NULL && *SolutionMetaDataSize > 0 )
        {
            Buffer->LengthOfSolutionData = *SolutionMetaDataSize;
            Buffer->OffsetOfSolutionData = Buffer->OffsetOfDstFileFullPath + Buffer->LengthOfDstFileFullPath;
            RtlCopyMemory( Add2Ptr( Buffer, Buffer->OffsetOfSolutionData ), SolutionMetaData, *SolutionMetaDataSize );
        }

        if( EncryptConfig != NULL )
            RtlCopyMemory( &Buffer->EncryptConfig, EncryptConfig, sizeof( ENCRYPT_CONFIG ) );

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_FILE_ENCRYPT_FILE,
                                         Buffer, RequiredSize,
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( Buffer != NULL )
        free( Buffer );

    closeDevice( hDevice );
    return dwRet;
}

DWORD CWinIOIsolator::DecryptFileByDriver( const wchar_t* wszSrcFileFullPath, const wchar_t* wszDstFileFullPath, ENCRYPT_CONFIG* EncryptConfig, PVOID SolutionMetaData, ULONG* SolutionMetaDataSize )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    USER_FILE_ENCRYPT* Buffer = NULL;

    do
    {
        hDevice = retrieveDevice();

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        ULONG RequiredSize = sizeof( USER_FILE_ENCRYPT );
        RequiredSize += wcslen( wszSrcFileFullPath ) * sizeof( WCHAR ) + sizeof( WCHAR );
        RequiredSize += wcslen( wszDstFileFullPath ) * sizeof( WCHAR ) + sizeof( WCHAR );

        if( SolutionMetaData != NULL && SolutionMetaDataSize != NULL && *SolutionMetaDataSize > 0 )
            RequiredSize += *SolutionMetaDataSize;

        Buffer = ( USER_FILE_ENCRYPT* )malloc( RequiredSize );
        RtlZeroMemory( Buffer, RequiredSize );

        Buffer->SizeOfStruct = RequiredSize;

        Buffer->LengthOfSrcFileFullPath = wcslen( wszSrcFileFullPath ) * sizeof( WCHAR ) + sizeof( WCHAR );
        Buffer->OffsetOfSrcFileFullPath = sizeof( USER_FILE_ENCRYPT );
        wcscpy( ( WCHAR* )Add2Ptr( Buffer, Buffer->OffsetOfSrcFileFullPath ), wszSrcFileFullPath );

        Buffer->LengthOfDstFileFullPath = wcslen( wszDstFileFullPath ) * sizeof( WCHAR ) + sizeof( WCHAR );
        Buffer->OffsetOfDstFileFullPath = Buffer->OffsetOfSrcFileFullPath + Buffer->LengthOfSrcFileFullPath;
        wcscpy( ( WCHAR* )Add2Ptr( Buffer, Buffer->OffsetOfDstFileFullPath ), wszDstFileFullPath );

        if( EncryptConfig != NULL )
            RtlCopyMemory( &Buffer->EncryptConfig, EncryptConfig, sizeof( ENCRYPT_CONFIG ) );

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_FILE_DECRYPT_FILE,
                                         Buffer, RequiredSize,
                                         Buffer, RequiredSize,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( Buffer != NULL )
        free( Buffer );

    closeDevice( hDevice );
    return dwRet;
}

///////////////////////////////////////////////////////////////////////////////

HANDLE CWinIOIsolator::retrieveDevice()
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL );

    return hDevice;
}

void CWinIOIsolator::closeDevice( HANDLE hDevice )
{
    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );
}

unsigned CWinIOIsolator::messageWorker( PVOID Param )
{
    if( Param == NULL )
        return 0;

    HRESULT hr = S_OK;
    CWinIOIsolator::Context* context = ( CWinIOIsolator::Context* )Param;
    OVERLAPPED ov;
    memset( &ov, 0, sizeof( OVERLAPPED ) );
    HANDLE hEvent = CreateEventW( NULL, FALSE, FALSE, NULL );
    ov.hEvent = hEvent;

    // TODO: Need Exit Event!
    static const int MAX_PACKET_SIZE = 65535;
    MSG_SEND_PACKETU* incoming = ( MSG_SEND_PACKETU* )new BYTE[ MAX_PACKET_SIZE ];
    MSG_REPLY_PACKETU outgoing;

    DWORD dwRet = 0;
    while( context->self->isExit == false )
    {
        memset( incoming, '\0', MAX_PACKET_SIZE );

        hr = FilterGetMessage( context->hPort, &incoming->MessageHDR, MAX_PACKET_SIZE, &ov );
        if( hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING ) )
            break;

        if( context->self->isExit == true )
            break;

        do
        {
            dwRet = WaitForSingleObject( ov.hEvent, 500 );

        } while( context->self->isExit == false && dwRet == WAIT_TIMEOUT );

        if( context->self->isExit == true )
            break;

        memset( &outgoing, 0, sizeof( outgoing ) );

        if( context->self->MessageCallbackFn.first != NULL && incoming->MessageHDR.MessageId > 0 )
            context->self->MessageCallbackFn.first( &incoming->MessageBody, &outgoing.ReplyBody, context->self->MessageCallbackFn.second );

        outgoing.ReplyHDR.Status = 0;
        outgoing.ReplyHDR.MessageId = incoming->MessageHDR.MessageId;
        if( incoming->MessageHDR.ReplyLength > 0 )
        {
            hr = FilterReplyMessage( context->hPort, ( PFILTER_REPLY_HEADER )&outgoing, sizeof( outgoing ) );
        }
    }

    if( hEvent != NULL )
        CloseHandle( hEvent );

    if( incoming != NULL )
        delete[] incoming;
    return 0;
}

