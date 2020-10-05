#include "deviceCntl.hpp"


#include "utilities/contextMgr_Defs.hpp"

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

    // NOTE: 이곳에서 FALSE 를 반환하면 IO 관리자는 IRP 를 생성하여 전달한다 
    return TRUE;
}
