#include "WinIOMonitor_API.hpp"

#include "WinIOMonitor_IOCTL.hpp"
#include "WinIOMonitor_Names.hpp"

#include <process.h>

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#pragma comment( lib, "fltLib" )

struct 
{
    HANDLE hPort                    = INVALID_HANDLE_VALUE;

    MessageCallback pfnMessageCallback  = NULL;
    DisconnectCallback pfnDisconnectCallback = NULL;
    unsigned int uThreadCount = 0;

    volatile bool isExit = false;

} GlobalContext;

unsigned int WINAPI worker( PVOID Param );
const unsigned int MAX_PACKET_SIZE = 65535;

///////////////////////////////////////////////////////////////////////////////

DWORD ConnectTo()
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    HRESULT hRet = S_OK;

    do
    {
        if( GlobalContext.hPort != INVALID_HANDLE_VALUE )
        {
            dwRet = ERROR_SUCCESS;
            break;
        }

        hRet = FilterConnectCommunicationPort( PORT_NAME, 0,
                                               NULL, 0, NULL, &GlobalContext.hPort );

        if( FAILED( hRet ) )
            break;

        dwRet = ERROR_SUCCESS;

    } while( false );

    return dwRet;
}

DWORD Disconnect()
{
    GlobalContext.isExit = true;

    if( GlobalContext.hPort != INVALID_HANDLE_VALUE )
        CloseHandle( GlobalContext.hPort );
    GlobalContext.hPort = INVALID_HANDLE_VALUE;

    return ERROR_SUCCESS;
}

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

DWORD RegisterCallback( ULONG ThreadCount, MessageCallback pfnMessageCallback, DisconnectCallback pfnDisconnectCallback )
{
    if( GlobalContext.uThreadCount > 0 )
        return ERROR_ALREADY_INITIALIZED;

    GlobalContext.uThreadCount = ThreadCount;
    GlobalContext.pfnMessageCallback = pfnMessageCallback;
    GlobalContext.pfnDisconnectCallback = pfnDisconnectCallback;

    for( unsigned int idx = 0; idx < GlobalContext.uThreadCount; ++idx )
    {
        _beginthreadex( NULL, 0, &worker, NULL, 0, NULL );
    }

    return ERROR_SUCCESS;
}

DWORD AddProcessFileFilterMask( const PROCESS_FILTER* ProcessFilter )
{
    DWORD dwRet = ERROR_SUCCESS;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        if( ProcessFilter == NULL )
        {
            dwRet = ERROR_INVALID_PARAMETER;
            break;
        }

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
        BOOL isSuccess = DeviceIoControl( hDevice, IOCTL_ADD_PROCESS_POLICY,
                                          ( LPVOID )ProcessFilter, sizeof( PROCESS_FILTER ),
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

DWORD GetProcessFileFilterCount( DWORD* FilterCount )
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
        BOOL isSuccess = DeviceIoControl( hDevice, IOCTL_GET_PROCESS_POLICY_COUNT,
                                          NULL, 0,
                                          ( LPVOID )FilterCount, sizeof( DWORD ), &dwReturnBytes, NULL );

        if( isSuccess != FALSE )
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

    } while( false );

    if( hDevice != INVALID_HANDLE_VALUE )
        CloseHandle( hDevice );

    return dwRet;
}

DWORD RemoveProcessFileFilterMask( const wchar_t* wszFilterMask )
{
    DWORD dwRet = ERROR_SUCCESS;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    do
    {
        if( wszFilterMask == NULL || wcslen( wszFilterMask ) == 0 )
        {
            dwRet = ERROR_INVALID_PARAMETER;
            break;
        }

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
        BOOL isSuccess = DeviceIoControl( hDevice, IOCTL_DEL_PROCESS_POLICY_BY_MASK,
                                          ( LPVOID )wszFilterMask, (DWORD)(( wcslen( wszFilterMask ) + 1 ) * sizeof( WCHAR )),
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

DWORD RemoveProcessFileFilterMask( DWORD ProcessId )
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
        BOOL isSuccess = DeviceIoControl( hDevice, IOCTL_DEL_PROCESS_POLICY_BY_PID,
                                          ( LPVOID )&ProcessId, sizeof( DWORD ),
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

DWORD ResetProcessFileFilterMask()
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
        BOOL isSuccess = DeviceIoControl( hDevice, IOCTL_CLEAR_PROCESS_POLICY,
                                          NULL, 0,
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

///////////////////////////////////////////////////////////////////////////////

#pragma pack(1)
struct TyMessageSendPacketU
{
    FILTER_MESSAGE_HEADER   MessageHeader;
    MSG_SEND_PACKET         MessageBody;
};

struct TyMessageReplyPacketU
{
    FILTER_REPLY_HEADER     ReplyHeader;
    MSG_REPLY_PACKET        ReplyBody;
};
#pragma pack()

unsigned int WINAPI worker( PVOID Param )
{
    if( Param == NULL )
        return 0;

    HRESULT hr = S_OK;
    DWORD dwRet = 0;

    OVERLAPPED ov;
    memset( &ov, 0, sizeof( ov ) );
    HANDLE hEvent = CreateEventW( NULL, FALSE, FALSE, NULL );
    ov.hEvent = hEvent;

    TyMessageSendPacketU* incoming = ( TyMessageSendPacketU* )new BYTE[ MAX_PACKET_SIZE ];
    TyMessageReplyPacketU* outgoing = ( TyMessageReplyPacketU* )new BYTE[ MAX_PACKET_SIZE ];

    while( GlobalContext.isExit == false )
    {
        memset( incoming, 0, MAX_PACKET_SIZE );

        while( GlobalContext.hPort == INVALID_HANDLE_VALUE && GlobalContext.isExit == false )
            Sleep( 1000 );

        if( GlobalContext.isExit == true )
            break;

        hr = FilterGetMessage( GlobalContext.hPort, &incoming->MessageHeader, MAX_PACKET_SIZE, &ov );
        if( hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING ) )
            break;

        if( GlobalContext.isExit == true )
            break;

        do
        {
            dwRet = WaitForSingleObject( ov.hEvent, 100 );

        } while( GlobalContext.isExit == false && dwRet == WAIT_TIMEOUT );

        if( dwRet != WAIT_OBJECT_0 )
            break;

        if( GlobalContext.isExit == true )
            break;

        memset( outgoing, 0, MAX_PACKET_SIZE );

        if( GlobalContext.pfnMessageCallback != NULL && incoming->MessageHeader.MessageId > 0 )
            GlobalContext.pfnMessageCallback( &incoming->MessageBody, &outgoing->ReplyBody );

        outgoing->ReplyHeader.Status = 0;
        outgoing->ReplyHeader.MessageId = incoming->MessageHeader.MessageId;
        if( incoming->MessageHeader.ReplyLength > 0 )
        {
            hr = FilterReplyMessage( GlobalContext.hPort, ( PFILTER_REPLY_HEADER )outgoing, incoming->MessageHeader.ReplyLength );
        }
    }

    if( hEvent != NULL )
        CloseHandle( hEvent );

    if( incoming != NULL )
        delete[] incoming;

    if( outgoing != NULL )
        delete[] outgoing;

    return 0;
}
