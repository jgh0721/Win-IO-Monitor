﻿#include "W32API.hpp"

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

NTSTATUS nsW32API::FltCreateFileEx( PFLT_FILTER Filter, PFLT_INSTANCE Instance, PHANDLE FileHandle, PFILE_OBJECT* FileObject, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength, ULONG Flags )
{
    if( GlobalFltMgr.Is_FltCreateFileEx() == true )
        return GlobalFltMgr.FltCreateFileEx( Filter, Instance, FileHandle, FileObject, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength, Flags );

    NTSTATUS Status = FltCreateFile( Filter, Instance, 
                                     FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, 
                                     AllocationSize, FileAttributes, ShareAccess, 
                                     CreateDisposition, CreateOptions, 
                                     EaBuffer, EaLength, Flags );

    do
    {
        if( !NT_SUCCESS( Status ) )
            break;

        ASSERT( *FileHandle != INVALID_HANDLE_VALUE && *FileHandle != NULL );

        if( ARGUMENT_PRESENT( FileObject ) )
        {
            //
            //  If the user provided an output FileObject parameter,
            //  then we need to get a reference to the fileobject and return it.
            //

            Status = ObReferenceObjectByHandle( *FileHandle,
                                                DesiredAccess,
                                                *IoFileObjectType,
                                                KernelMode,
                                                ( PVOID* )FileObject,
                                                NULL );

            if( !NT_SUCCESS( Status ) )
                break;
        }

    } while( false );

    return Status;
}

NTSTATUS nsW32API::FltCreateFileEx2( PFLT_FILTER Filter, PFLT_INSTANCE Instance, PHANDLE FileHandle, PFILE_OBJECT* FileObject, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength, ULONG Flags, PIO_DRIVER_CREATE_CONTEXT DriverContext )
{
    if( GlobalFltMgr.Is_FltCreateFileEx2() == true )
        return GlobalFltMgr.FltCreateFileEx2( Filter, Instance, FileHandle, FileObject, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength, Flags, DriverContext );

    NTSTATUS Status;

    //  If we are here, FltCreateFileEx2 does not exist.  We
    //  cannot open files within the context of a transaction
    //  from here.  If Txf exists, so should FltCreateFileEx2.

    UNREFERENCED_PARAMETER( DriverContext );
    ASSERT( DriverContext == NULL );

    //  Zero out output parameters.

    *FileHandle = INVALID_HANDLE_VALUE;

    if( ARGUMENT_PRESENT( FileObject ) )
    {
        *FileObject = NULL;
    }

    do
    {
        if( GlobalFltMgr.Is_FltCreateFileEx() == true )
        {
            //  If the system has FltCreateFileEx, we call that.

            Status = GlobalFltMgr.FltCreateFileEx( Filter,
                                                   Instance,
                                                   FileHandle,
                                                   FileObject,
                                                   DesiredAccess,
                                                   ObjectAttributes,
                                                   IoStatusBlock,
                                                   AllocationSize,
                                                   FileAttributes,
                                                   ShareAccess,
                                                   CreateDisposition,
                                                   CreateOptions,
                                                   EaBuffer,
                                                   EaLength,
                                                   Flags );
        }
        else
        {

            //
            //  Attempt the open.
            //

            Status = FltCreateFile( Filter,
                                    Instance,
                                    FileHandle,
                                    DesiredAccess,
                                    ObjectAttributes,
                                    IoStatusBlock,
                                    AllocationSize,
                                    FileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    EaBuffer,
                                    EaLength,
                                    Flags );

            if( !NT_SUCCESS( Status ) )
                break;

            ASSERT( *FileHandle != INVALID_HANDLE_VALUE && *FileHandle != NULL );

            if( ARGUMENT_PRESENT( FileObject ) )
            {
                //
                //  If the user provided an output FileObject parameter,
                //  then we need to get a reference to the fileobject and return it.
                //

                Status = ObReferenceObjectByHandle( *FileHandle,
                                                    DesiredAccess,
                                                    *IoFileObjectType,
                                                    KernelMode,
                                                    (PVOID*)FileObject,
                                                    NULL );

                if( !NT_SUCCESS( Status ) )
                    break;
            }
        }

    } while( false );

    if( !NT_SUCCESS( Status ) )
    {
        if( *FileHandle != INVALID_HANDLE_VALUE )
        {
            FltClose( *FileHandle );
        }

        if( ARGUMENT_PRESENT( FileObject ) )
        {
            if( *FileObject != NULLPTR )
            {
                ObDereferenceObject( *FileObject );
            }
        }
    }

    return Status;
}
