#include "WinIOMonitor_API.hpp"

#include "WinIOMonitor_IOCTL.hpp"
#include "WinIOMonitor_Names.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////
DWORD SetTimeOut( DWORD TimeOutMs )
{
    DWORD dwRet = ERROR_SUCCESS;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

        if( hDevice != INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        DWORD dwReturnBytes = 0;
        BOOL isSuccess = DeviceIoControl( hDevice, IOCTL_SET_TIMEOUT,
                                          ( LPVOID )&TimeOutMs, sizeof( DWORD ),
                                          NULL, 0, &dwReturnBytes, NULL );

        if( isSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD GetTimeOut( DWORD* TimeOutMs )
{
    DWORD dwRet = ERROR_SUCCESS;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        hDevice = CreateFileW( L"\\\\." DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

        if( hDevice != INVALID_HANDLE_VALUE )
        {
            dwRet = GetLastError();
            break;
        }

        DWORD dwReturnBytes = 0;
        BOOL isSuccess = DeviceIoControl( hDevice, IOCTL_GET_TIMEOUT,
                                          NULL, 0,
                                          (LPVOID)TimeOutMs, sizeof(DWORD), &dwReturnBytes, NULL );

        if( isSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}
