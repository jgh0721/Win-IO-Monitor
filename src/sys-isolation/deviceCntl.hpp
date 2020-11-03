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

BOOLEAN DevIOCntlSetDriverConfig( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlGetDriverConfig( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlSetDriverStatus( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlGetDriverStatus( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );

BOOLEAN DevIOCntlSetStubCode( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlGetStubCode( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );

BOOLEAN DevIOCntlAddGlobalPolicy( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );;
BOOLEAN DevIOCntlDelGlobalPolicy( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlGetGlobalPolicyCnt( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlRstGlobalPolicy( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlGetGlobalPolicy( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );

BOOLEAN DevIOCntlAddProcessPolicy( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlDelProcessPolicyID( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlDelProcessPolicyPID( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlDelProcessPolicyMask( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlAddProcessPolicyItem( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlDelProcessPolicyItem( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlRstProcessPolicy( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );

BOOLEAN DevIOCntlFileGetType( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlFileSetSolutionData( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlFileGetSolutionData( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlFileEncrypt( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );
BOOLEAN DevIOCntlFileDecrypt( IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PIO_STATUS_BLOCK IoStatus );

#endif // HDR_WINIOMONITOR_DEVICE_CNTL