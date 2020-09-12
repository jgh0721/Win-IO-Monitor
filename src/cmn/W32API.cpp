#include "W32API.hpp"

#include "utilities/osInfoMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

nsW32API::CNtOsKrnlAPI  GlobalNtOsKrnlMgr;
nsW32API::CFltMgrAPI    GlobalFltMgr;

#ifndef IOCTL_DISK_BASE
#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK
#endif

#ifndef IOCTL_DISK_IS_WRITABLE
#define IOCTL_DISK_IS_WRITABLE          CTL_CODE(IOCTL_DISK_BASE, 0x0009, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

///////////////////////////////////////////////////////////////////////////////

NTSTATUS nsW32API::IsVolumeWritable( PVOID FltObject, PBOOLEAN IsWritable )
{
    // from https://greemate.tistory.com/entry/FltIsVolumeWritable

    if( nsUtils::VerifyVersionInfoEx( 6, ">=" ) == true )
        return GlobalFltMgr.FltIsVolumeWritable( FltObject, IsWritable );

    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT VolumeDeviceObject = NULL;

    do
    {
        KEVENT kEvent;
        PIRP pIrp = NULL;
        IO_STATUS_BLOCK IoStatusBlock;

        if( FltObject == NULL || IsWritable == NULL )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        *IsWritable = TRUE;

        Status = FltGetDiskDeviceObject( ( PFLT_VOLUME )FltObject, &VolumeDeviceObject );
        if( !NT_SUCCESS( Status ) )
            break;

        KeInitializeEvent( &kEvent, NotificationEvent, FALSE );

        pIrp = IoBuildDeviceIoControlRequest( IOCTL_DISK_IS_WRITABLE,
                                              VolumeDeviceObject,
                                              NULL, 0, NULL, 0, FALSE,
                                              &kEvent,
                                              &IoStatusBlock );

        if( pIrp == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        Status = IoCallDriver( VolumeDeviceObject, pIrp );
        if( Status == STATUS_PENDING )
        {
            KeWaitForSingleObject( &kEvent, Executive, KernelMode, FALSE, NULL );
            Status = IoStatusBlock.Status;
        }

        if( Status == STATUS_MEDIA_WRITE_PROTECTED )
            *IsWritable = FALSE;

        // IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        Status = STATUS_SUCCESS;

    } while( false );

    if( VolumeDeviceObject != NULL )
        ObDereferenceObject( VolumeDeviceObject );

    return Status;
}
