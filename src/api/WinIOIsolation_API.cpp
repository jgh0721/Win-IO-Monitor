#include "WinIOIsolation_API.hpp"

#include "WinIOIsolation_IOCTL.hpp"
#include "WinIOIsolation_Names.hpp"

#include <process.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment( lib, "fltlib" )

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

struct
{
    HANDLE                          hPort = INVALID_HANDLE_VALUE;
    HANDLE                          hDevice = INVALID_HANDLE_VALUE;

} GlobalContext;

///////////////////////////////////////////////////////////////////////////////

HRESULT ConnectTo()
{
    HRESULT hRet = S_OK;

    do
    {
        if( GlobalContext.hPort != INVALID_HANDLE_VALUE )
        {
            break;
        }

        hRet = FilterConnectCommunicationPort( PORT_NAME, 0,
                                               NULL, 0, NULL,
                                               &GlobalContext.hPort );

        if( FAILED( hRet ) )
            break;

    } while( false );

    return hRet;
}

HRESULT Disconnect()
{
    if( GlobalContext.hPort != INVALID_HANDLE_VALUE )
        CloseHandle( GlobalContext.hPort );
    GlobalContext.hPort = INVALID_HANDLE_VALUE;

    return S_OK;
}

DWORD SetDriverStatus( BOOLEAN IsRunning )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice,
                                    IOCTL_SET_DRIVER_STATUS,
                                    &IsRunning,
                                    sizeof(BOOLEAN),
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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD GetDriverStatus( BOOLEAN* IsRunning )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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
                                         sizeof(BOOLEAN),
                                         ( LPDWORD )&dwRetLen,
                                         NULL );

        if( !bSuccess )
        {
            dwRet = GetLastError();
            break;
        }

    } while( false );

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD AddGlobalFilterMask( const wchar_t* wszFilterMask, bool isInclude )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    WCHAR* wszBuffer = NULL;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD DelGlobalFilterMask( const wchar_t* wszFilterMask, bool isInclude )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    WCHAR* wszBuffer = NULL;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD GetGlobalFilterMaskCnt( bool isInclude, ULONG* Count )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    WCHAR* wszBuffer = NULL;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        size_t RequiredSize = 0;
        RequiredSize += sizeof( WCHAR );        // 0 = Exclude, 1 = Include

        wszBuffer = ( WCHAR* )malloc( RequiredSize );
        memset( wszBuffer, '\0', RequiredSize );
        swprintf( wszBuffer, L"%d", isInclude == true ? TRUE : FALSE );

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_DEL_GLOBAL_POLICY_BY_MASK,
                                         wszBuffer, ( DWORD )RequiredSize,
                                         Count, sizeof(ULONG),
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( wszBuffer != NULL )
        free( wszBuffer );

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD ResetGlobalFilter()
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD AddProcessFilter( const USER_PROCESS_FILTER& ProcessFilter )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD DelProcessFilterByID( const USER_PROCESS_FILTER& ProcessFilter )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD DelProcessFilterByPID( ULONG ProcessId )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD DelProcessFilterByFilterMask( const wchar_t* wszFilterMask )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_DEL_PROCESS_POLICY_BY_MASK,
                                         ( LPVOID )&wszFilterMask, ( wcslen( wszFilterMask ) + 1 ) * sizeof(WCHAR),
                                         NULL, 0,
                                         ( LPDWORD )&dwRetLen, NULL );

        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD AddProcessFilterItem( const GUID& Id, const USER_PROCESS_FILTER_ENTRY& ProcessFilterItem )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    PBYTE Buffer = NULL;
    ULONG BufferSize = sizeof( GUID ) + sizeof( USER_PROCESS_FILTER_ENTRY );

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD DelProcessFilterItem( const GUID& Id, const USER_PROCESS_FILTER_ENTRY& ProcessFilterItem )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    PBYTE Buffer = NULL;
    ULONG BufferSize = sizeof( GUID ) + sizeof( USER_PROCESS_FILTER_ENTRY );

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD ResetProcessFilter()
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

///////////////////////////////////////////////////////////////////////////////
/// File Management

DWORD GetFileType( const wchar_t* wszFileFullPath, ULONG* FileType )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

        if( hDevice == INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        BOOL bSuccess = DeviceIoControl( hDevice, IOCTL_FILE_GET_FILE_TYPE,
                                         (LPVOID)wszFileFullPath, (wcslen( wszFileFullPath ) + 1) * sizeof(WCHAR),
                                         FileType, sizeof(ULONG),
                                         ( LPDWORD )&dwRetLen, NULL );


        if( bSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD SetFileSolutionMetaData( const wchar_t* wszFileFullPath, PVOID Buffer, ULONG* BufferSize )
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

        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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
        swprintf( ( PWCH )( ( PBYTE )( UserFileSolutionData ) + UserFileSolutionData->OffsetOfFileName ),
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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD GetFileSolutionMetaData( const wchar_t* wszFileFullPath, PVOID Buffer, ULONG* BufferSize )
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetLen = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

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

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}
