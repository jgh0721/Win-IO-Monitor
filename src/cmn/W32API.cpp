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

        ASSERT( *FileHandle != NULLPTR );

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

    *FileHandle = NULLPTR;

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

            ASSERT( *FileHandle != NULLPTR );

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
                {
                    if( *FileHandle != NULLPTR )
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
                    break;

                }
            }
        }

    } while( false );

    return Status;
}

NTSTATUS nsW32API::FltQueryDirectoryFile( PFLT_INSTANCE Instance, PFILE_OBJECT FileObject, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass, BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan, PULONG LengthReturned )
{
    if( GlobalFltMgr.Is_FltQueryDirectoryFile() == true )
        return GlobalFltMgr.FltQueryDirectoryFile( Instance, FileObject, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan, LengthReturned );

    NTSTATUS            Status = STATUS_SUCCESS;
    PFLT_CALLBACK_DATA  CallbackData = NULLPTR;
    PFLT_PARAMETERS     Params = NULLPTR;

    do
    {
        ASSERT( NULL != Instance );
        ASSERT( NULL != FileInformation );
        ASSERT( Length > 0 );

        Status = FltAllocateCallbackData( Instance, FileObject, &CallbackData );

        if( !NT_SUCCESS( Status ) )
            break;

        CallbackData->Iopb->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
        CallbackData->Iopb->MinorFunction = IRP_MN_QUERY_DIRECTORY;

        if( RestartScan )
            SetFlag( CallbackData->Iopb->OperationFlags, SL_RESTART_SCAN );

        if( ReturnSingleEntry )
            SetFlag( CallbackData->Iopb->OperationFlags, SL_RETURN_SINGLE_ENTRY );

        Params = &CallbackData->Iopb->Parameters;
        Params->DirectoryControl.QueryDirectory.Length = Length;
        Params->DirectoryControl.QueryDirectory.FileName = FileName;
        Params->DirectoryControl.QueryDirectory.FileInformationClass = (::FILE_INFORMATION_CLASS)FileInformationClass;
        Params->DirectoryControl.QueryDirectory.DirectoryBuffer = FileInformation;
        Params->DirectoryControl.QueryDirectory.MdlAddress = NULL;

        FltPerformSynchronousIo( CallbackData );

        Status = CallbackData->IoStatus.Status;

        if( ARGUMENT_PRESENT( LengthReturned ) )
            *LengthReturned = CallbackData->IoStatus.Information;

    } while( false );

    if( CallbackData != NULLPTR )
        FltFreeCallbackData( CallbackData );

    return Status;
}

NTSTATUS nsW32API::FltQueryEaFile( PFLT_INSTANCE Instance, PFILE_OBJECT FileObject, PVOID ReturnedEaData, ULONG Length, BOOLEAN ReturnSingleEntry, PVOID EaList, ULONG EaListLength, PULONG EaIndex, BOOLEAN RestartScan, PULONG LengthReturned )
{
    if( GlobalFltMgr.Is_FltQueryEaFile() == true )
        return GlobalFltMgr.FltQueryEaFile( Instance, FileObject, ReturnedEaData, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan, LengthReturned );

    NTSTATUS            Status = STATUS_EAS_NOT_SUPPORTED;
    PFLT_CALLBACK_DATA  CallbackData = NULLPTR;
    PFLT_PARAMETERS     Params = NULLPTR;

    do
    {
        Status = FltAllocateCallbackData( Instance, FileObject, &CallbackData );

        if( !NT_SUCCESS( Status ) )
            break;

        CallbackData->Iopb->MajorFunction = IRP_MJ_QUERY_EA;
        CallbackData->Iopb->MinorFunction = 0;

        Params = &CallbackData->Iopb->Parameters;

        if( RestartScan != FALSE )
            SetFlag( CallbackData->Iopb->OperationFlags, SL_RESTART_SCAN );

        if( ReturnSingleEntry != FALSE )
            SetFlag( CallbackData->Iopb->OperationFlags, SL_RETURN_SINGLE_ENTRY );

        Params->QueryEa.Length = Length;
        Params->QueryEa.EaList = EaList;
        Params->QueryEa.EaListLength = EaListLength;

        if( ARGUMENT_PRESENT( EaIndex ) )
        {
            SetFlag( CallbackData->Iopb->OperationFlags, SL_INDEX_SPECIFIED );
            Params->QueryEa.EaIndex = *EaIndex;
        }
        else
        {
            Params->QueryEa.EaIndex = 0;
        }

        Params->QueryEa.EaBuffer = ReturnedEaData;

        FltPerformSynchronousIo( CallbackData );

        Status = CallbackData->IoStatus.Status;

        if( ARGUMENT_PRESENT( LengthReturned ) )
            *LengthReturned = CallbackData->IoStatus.Information;

    } while( false );

    if( CallbackData != NULLPTR )
        FltFreeCallbackData( CallbackData );

    return Status;
}

NTSTATUS nsW32API::FltSetEaFile( PFLT_INSTANCE Instance, PFILE_OBJECT FileObject, PVOID EaBuffer, ULONG Length )
{
    if( GlobalFltMgr.Is_FltSetEaFile() == true )
        return GlobalFltMgr.FltSetEaFile( Instance, FileObject, EaBuffer, Length );

    NTSTATUS            Status = STATUS_EAS_NOT_SUPPORTED;
    PFLT_CALLBACK_DATA  CallbackData = NULLPTR;
    PFLT_PARAMETERS     Params = NULLPTR;

    do
    {
        Status = FltAllocateCallbackData( Instance, FileObject, &CallbackData );

        if( !NT_SUCCESS( Status ) )
            break;

        CallbackData->Iopb->MajorFunction = IRP_MJ_SET_EA;
        CallbackData->Iopb->MinorFunction = 0;

        Params = &CallbackData->Iopb->Parameters;

        Params->SetEa.EaBuffer = EaBuffer;
        Params->SetEa.Length = Length;

        FltPerformSynchronousIo( CallbackData );

        Status = CallbackData->IoStatus.Status;

    } while( false );

    if( CallbackData != NULLPTR )
        FltFreeCallbackData( CallbackData );

    return Status;
}

FLT_PREOP_CALLBACK_STATUS nsW32API::FltProcessFileLock( PFILE_LOCK FileLock, PFLT_CALLBACK_DATA CallbackData, PVOID Context )
{
    FLT_PREOP_CALLBACK_STATUS FltStatus;
    PFLT_IO_PARAMETER_BLOCK Iopb = CallbackData->Iopb;

    IO_STATUS_BLOCK Iosb;
    NTSTATUS        Status;

    BOOLEAN	ExclusiveLock;
    BOOLEAN FailImmediately;

    Iosb.Information = 0;

    ASSERT( Iopb->MajorFunction == IRP_MJ_LOCK_CONTROL );

    ExclusiveLock = Iopb->Parameters.LockControl.ExclusiveLock;
    FailImmediately = Iopb->Parameters.LockControl.FailImmediately;

    switch( Iopb->MinorFunction )
    {
        case IRP_MN_LOCK:

            ( VOID )FsRtlFastLock( FileLock,
                                   Iopb->TargetFileObject,
                                   &Iopb->Parameters.LockControl.ByteOffset,
                                   Iopb->Parameters.LockControl.Length,
                                   Iopb->Parameters.LockControl.ProcessId,
                                   Iopb->Parameters.LockControl.Key,
                                   FailImmediately,
                                   ExclusiveLock,
                                   &Iosb,
                                   Context,
                                   FALSE );

            break;

        case IRP_MN_UNLOCK_SINGLE:

            Iosb.Status = FsRtlFastUnlockSingle( FileLock,
                                                 Iopb->TargetFileObject,
                                                 &Iopb->Parameters.LockControl.ByteOffset,
                                                 Iopb->Parameters.LockControl.Length,
                                                 Iopb->Parameters.LockControl.ProcessId,
                                                 Iopb->Parameters.LockControl.Key,
                                                 Context,
                                                 FALSE );


            break;

        case IRP_MN_UNLOCK_ALL:

            Iosb.Status = FsRtlFastUnlockAll( FileLock,
                                              Iopb->TargetFileObject,
                                              Iopb->Parameters.LockControl.ProcessId,
                                              Context );


            break;

        case IRP_MN_UNLOCK_ALL_BY_KEY:

            Iosb.Status = FsRtlFastUnlockAllByKey( FileLock,
                                                   Iopb->TargetFileObject,
                                                   Iopb->Parameters.LockControl.ProcessId,
                                                   Iopb->Parameters.LockControl.Key,
                                                   Context );

            break;

        default:

            Iosb.Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    CallbackData->IoStatus = Iosb;

    Status = Iosb.Status;

    if( Status == STATUS_PENDING )
    {
        FltStatus = FLT_PREOP_PENDING;
    }
    else
    {
        FltStatus = FLT_PREOP_COMPLETE;
    }

    return FltStatus;
}


NTSTATUS nsW32API::IoReplaceFileObjectName( PFILE_OBJECT FileObject, PWSTR NewFileName, USHORT FileNameLength )
{
    if( nsUtils::VerifyVersionInfoEx( 6, 1, ">=" ) == true )
        return GlobalNtOsKrnlMgr.IoReplaceFileObjectName( FileObject, NewFileName, FileNameLength );

    PWSTR buffer = NULLPTR;
    PUNICODE_STRING fileName = NULLPTR;
    USHORT newMaxLength;

    do
    {
        fileName = &FileObject->FileName;

        //
        // If the new name fits inside the current buffer we simply copy it over
        // instead of allocating a new buffer (and keep the MaximumLength value
        // the same).
        //
        if( FileNameLength <= fileName->MaximumLength )
            break;

        //
        // Use an optimal buffer size
        //
        newMaxLength = FileNameLength;

        buffer = ( PWSTR )ExAllocatePoolWithTag( PagedPool,
                                                 newMaxLength,
                                                 'main' );

        if( !buffer )
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if( fileName->Buffer != NULL )
        {
            ExFreePool( fileName->Buffer );
        }

        fileName->Buffer = buffer;
        fileName->MaximumLength = newMaxLength;

    } while( false );

    fileName->Length = FileNameLength;
    RtlZeroMemory( fileName->Buffer, fileName->MaximumLength );
    RtlCopyMemory( fileName->Buffer, NewFileName, FileNameLength );

    return STATUS_SUCCESS;
}
