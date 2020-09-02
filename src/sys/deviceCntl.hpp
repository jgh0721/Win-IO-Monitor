#ifndef HDR_WINIOMONITOR_DEVICE_CNTL
#define HDR_WINIOMONITOR_DEVICE_CNTL

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

BOOLEAN FLTAPI FastIoDeviceControl( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait,
                                    IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength,
                                    OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength,
                                    IN ULONG IoControlCode, OUT PIO_STATUS_BLOCK IoStatus,
                                    IN PDEVICE_OBJECT DeviceObject );

EXTERN_C_END

///////////////////////////////////////////////////////////////////////////////
// IOCTL Handler

BOOLEAN DevIOCntlSetTimeOutMs( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, IN ULONG IoControlCode, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN DevIOCntlGetTimeOutMs( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, IN ULONG IoControlCode, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );

BOOLEAN DevIOCntlAddProcessPolicy( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlDelProcessPolicyByPID( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlDelProcessPolicyByMask( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlGetProcessPolicyCount( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlClearProcessPolicy( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );

BOOLEAN DevIOCntlCollectNotifyEventItems( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );

#endif // HDR_WINIOMONITOR_DEVICE_CNTL