#include "deviceCntl.hpp"

#include "driverMgmt.hpp"
#include "utilities/contextMgr_Defs.hpp"

#include "WinIOIsolation_IOCTL.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

BOOLEAN FLTAPI FastIoDeviceControl( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength,
                                    PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus,
                                    PDEVICE_OBJECT DeviceObject )
{
    UNREFERENCED_PARAMETER( FileObject );
    UNREFERENCED_PARAMETER( Wait );

    KdPrintEx(( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOSol] %s|PID=%d|Thread:%p|DeviceObject:%p|IRQL:%d|IOCTL=0x%08x\n", 
               __FUNCTION__, PsGetCurrentProcessId(), PsGetCurrentThread(), DeviceObject, KeGetCurrentIrql(), IoControlCode ) );

    switch( (int)IoControlCode )
    {
        case IOCTL_SET_DRIVER_STATUS: {
            DevIOCntlSetDriverStatus( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;
        case IOCTL_GET_DRIVER_STATUS: {
            DevIOCntlGetDriverStatus( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;
    }

    // NOTE: 이곳에서 FALSE 를 반환하면 IO 관리자는 IRP 를 생성하여 전달한다 
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL Handler


BOOLEAN DevIOCntlSetDriverStatus( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != (ULONG)PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBuffer == NULLPTR || InputBufferLength < sizeof( BOOLEAN ) )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if( *(BOOLEAN*)InputBuffer != FALSE )
            InterlockedIncrement( &FeatureContext.IsRunning );
        else
            InterlockedDecrement( &FeatureContext.IsRunning );

        IoStatus->Status = STATUS_SUCCESS;

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlGetDriverStatus( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( OutputBuffer == NULLPTR || OutputBufferLength < sizeof(BOOLEAN) )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        *( BOOLEAN* )OutputBuffer = FeatureContext.IsRunning > 0;
        IoStatus->Status = STATUS_SUCCESS;

    } while( false );

    return TRUE;
}
