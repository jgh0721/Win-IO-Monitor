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

    return dwRet;
}
