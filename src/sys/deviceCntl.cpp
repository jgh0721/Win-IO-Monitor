#include "deviceCntl.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

BOOLEAN FastIoDeviceControl( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength,
                             PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus,
                             PDEVICE_OBJECT DeviceObject )
{
    KdPrintEx(( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] %s|PID=%d|Thread:%p|DeviceObject:%p|IRQL:%d|IOCTL=0x%08x\n", 
               __FUNCTION__, PsGetCurrentProcessId(), PsGetCurrentThread(), DeviceObject, KeGetCurrentIrql(), IoControlCode ) );

    
    return TRUE;
}
