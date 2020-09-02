#include "deviceCntl.hpp"

#include "notifyMgr.hpp"
#include "policies/processFilter.hpp"

#include "WinIOMonitor_API.hpp"
#include "WinIOMonitor_IOCTL.hpp"
#include "utilities/contextMgr_Defs.hpp"
#include "utilities/fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

BOOLEAN FLTAPI FastIoDeviceControl( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength,
                                    PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus,
                                    PDEVICE_OBJECT DeviceObject )
{
    UNREFERENCED_PARAMETER( FileObject );
    UNREFERENCED_PARAMETER( Wait );

    KdPrintEx(( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] %s|PID=%d|Thread:%p|DeviceObject:%p|IRQL:%d|IOCTL=0x%08x\n", 
               __FUNCTION__, PsGetCurrentProcessId(), PsGetCurrentThread(), DeviceObject, KeGetCurrentIrql(), IoControlCode ) );

    switch( IoControlCode )
    {
        case IOCTL_SET_TIMEOUT: {
            DevIOCntlSetTimeOutMs( FileObject, Wait, 
                                   InputBuffer, InputBufferLength, 
                                   OutputBuffer, OutputBufferLength, 
                                   IoControlCode, IoStatus, DeviceObject );

        } break;
        case IOCTL_GET_TIMEOUT: {
            DevIOCntlGetTimeOutMs( FileObject, Wait,
                                   InputBuffer, InputBufferLength,
                                   OutputBuffer, OutputBufferLength,
                                   IoControlCode, IoStatus, DeviceObject );

        } break;

        case IOCTL_ADD_PROCESS_POLICY: {
            DevIOCntlAddProcessPolicy( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_DEL_PROCESS_POLICY_BY_PID: {
            DevIOCntlDelProcessPolicyByPID( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_DEL_PROCESS_POLICY_BY_MASK: {
            DevIOCntlDelProcessPolicyByMask( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_GET_PROCESS_POLICY_COUNT: {
            DevIOCntlGetProcessPolicyCount( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_CLEAR_PROCESS_POLICY: {
            DevIOCntlClearProcessPolicy( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;

        case IOCTL_GET_NOTIFY_EVENTS: {
            DevIOCntlCollectNotifyEventItems( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
    }
    
    return TRUE;
}

BOOLEAN DevIOCntlSetTimeOutMs( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    BOOLEAN bRet = TRUE;

    do
    {
        if( InputBuffer == NULLPTR || InputBufferLength < sizeof( DWORD ) )
        {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus->Information = sizeof( DWORD );
            break;
        }

        DWORD dwTimeOutMs = *( DWORD* )( InputBuffer );

        if( dwTimeOutMs == 0 )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        GlobalContext.TimeOutMs.QuadPart = RELATIVE( MILLISECONDS( dwTimeOutMs ) );

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;

    } while( false );

    return bRet;
}

BOOLEAN DevIOCntlGetTimeOutMs( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    BOOLEAN bRet = TRUE;

    do
    {
        if( OutputBuffer == NULLPTR || OutputBufferLength != sizeof(DWORD) )
        {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus->Information = sizeof( DWORD );
            break;
        }

        DWORD* dwTimeOutMs = ( DWORD* )OutputBuffer;

        *dwTimeOutMs = ( GlobalContext.TimeOutMs.QuadPart ) / ( MICROSECONDS( 1000L ) );

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof( DWORD );

    } while( false );

    return bRet;
}

BOOLEAN DevIOCntlAddProcessPolicy( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    BOOLEAN bRet = TRUE;

    do
    {
        if( InputBuffer == NULLPTR || InputBufferLength < sizeof( PROCESS_FILTER ) )
        {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus->Information = sizeof( PROCESS_FILTER );
            break;
        }

        PROCESS_FILTER* ProcessFilter = ( PROCESS_FILTER* )InputBuffer;

        IoStatus->Status = ProcessFilter_Add( ProcessFilter );
        IoStatus->Information = 0;

    } while( false );

    return bRet;
}

BOOLEAN DevIOCntlDelProcessPolicyByPID( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    BOOLEAN bRet = TRUE;

    do
    {
        if( InputBuffer == NULLPTR || InputBufferLength < sizeof( DWORD ) )
        {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus->Information = sizeof( DWORD );
            break;
        }

        DWORD ProcessId = *( DWORD* )( InputBuffer );

        if( ProcessId == 0 )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        IoStatus->Status = ProcessFilter_Remove( ProcessId, NULLPTR );
        IoStatus->Information = 0;
        
    } while( false );

    return bRet;
}

BOOLEAN DevIOCntlDelProcessPolicyByMask( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    BOOLEAN bRet = TRUE;

    do
    {
        if( InputBuffer == NULLPTR || InputBufferLength <= 0 )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        WCHAR* ProcessMask = ( WCHAR* )InputBuffer;
        auto BufferSize = ( nsUtils::strlength( ProcessMask ) + 1 ) * sizeof(WCHAR);

        if( BufferSize != InputBufferLength )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        IoStatus->Status = ProcessFilter_Remove( 0, ProcessMask );
        IoStatus->Information = 0;

    } while( false );

    return bRet;
}

BOOLEAN DevIOCntlGetProcessPolicyCount( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    BOOLEAN bRet = TRUE;

    do
    {
        if( OutputBuffer == NULLPTR || OutputBufferLength != sizeof(DWORD) )
        {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus->Information = sizeof( DWORD );
            break;
        }

        DWORD* ProcessPolicyCount = ( DWORD* )OutputBuffer;

        *ProcessPolicyCount = ProcessFilter_Count();

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof( DWORD );

    } while( false );

    return bRet;
}

BOOLEAN DevIOCntlClearProcessPolicy( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    BOOLEAN bRet = TRUE;

    do
    {
        IoStatus->Status = CloseProcessFilter();
        IoStatus->Information = 0;

    } while( false );

    return bRet;
}

BOOLEAN DevIOCntlCollectNotifyEventItems( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    BOOLEAN bRet = TRUE;

    UNREFERENCED_PARAMETER( InputBuffer );
    UNREFERENCED_PARAMETER( InputBufferLength );

    do
    {
        if( OutputBuffer == NULLPTR )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        if( OutputBufferLength <= sizeof( MSG_SEND_PACKET ) + sizeof(ULONG) )
        {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus->Information = sizeof( MSG_SEND_PACKET ) + sizeof( ULONG ) + ( MAX_PATH * sizeof( WCHAR ) );
            break;
        }

        ULONG WrittenBytes = 0;
        ULONG NotifyItemCount = 0;

        IoStatus->Status = CollectNotifyItem( Add2Ptr( OutputBuffer, sizeof( ULONG ) ), OutputBufferLength - sizeof( ULONG ),
                                              &WrittenBytes,
                                              &NotifyItemCount );

        if( NT_SUCCESS( IoStatus->Status ) )
        {
            *( ULONG* )OutputBuffer = NotifyItemCount;
            IoStatus->Information = WrittenBytes;
        }

    } while( false );

    return bRet;
}
