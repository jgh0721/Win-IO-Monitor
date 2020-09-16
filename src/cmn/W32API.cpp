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

///////////////////////////////////////////////////////////////////////////
/// Debug Helper

const char* nsW32API::ConvertCreateShareAccess( ULONG ShareAccess )
{
    if( ShareAccess & ( FILE_SHARE_READ ) )
    {
        return "FILE_SHARE_READ";
    }
    else if( ShareAccess & ( FILE_SHARE_WRITE ) )
    {
        return "FILE_SHARE_WRITE";
    }
    else if( ShareAccess & ( FILE_SHARE_DELETE ) )
    {
        return "FILE_SHARE_DELETE";
    }
    else if( ShareAccess & ( FILE_SHARE_READ & FILE_SHARE_WRITE ) )
    {
        return "FILE_SHARE_READ|FILE_SHARE_WRITE";
    }
    else if( ShareAccess & ( FILE_SHARE_READ & FILE_SHARE_WRITE & FILE_SHARE_DELETE ) )
    {
        return "FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE";
    }
    else if( ShareAccess & ( FILE_SHARE_WRITE & FILE_SHARE_DELETE ) )
    {
        return "FILE_SHARE_WRITE|FILE_SHARE_DELETE";
    }
    else if( ShareAccess & ( FILE_SHARE_READ & FILE_SHARE_DELETE ) )
    {
        return "FILE_SHARE_READ|FILE_SHARE_DELETE";
    }

    if( ShareAccess == 0 )
        return "None";

    return "Unk";
}

const char* nsW32API::ConvertCreateDisposition( __in ULONG CreateDisposition )
{
    switch( CreateDisposition )
    {
        case FILE_SUPERSEDE:
            return "FILE_SUPERSEDE";
        case FILE_CREATE:
            return "FILE_CREATE";
        case FILE_OPEN:
            return "FILE_OPEN";
        case FILE_OPEN_IF:
            return "FILE_OPEN_IF";
        case FILE_OVERWRITE:
            return "FILE_OVERWRITE";
        case FILE_OVERWRITE_IF:
            return "FILE_OVERWRITE_IF";
    }

    return "Unk";
}

const char* nsW32API::ConvertCreateResultInformation( NTSTATUS Status, ULONG_PTR Information )
{
    if( !NT_SUCCESS( Status ) )
        return "FAILED";

    switch( Information )
    {
        case FILE_CREATED: { return "FILE_CREATED"; } break;
        case FILE_OPENED: { return "FILE_OPENED"; } break;
        case FILE_OVERWRITTEN: { return "FILE_OVERWRITTEN"; } break;
        case FILE_SUPERSEDED: { return "FILE_SUPERSEDED"; } break;
        case FILE_EXISTS: { return "FILE_EXISTS"; } break;
        case FILE_DOES_NOT_EXIST: { return "FILE_DOES_NOT_EXIST"; } break;
    }

    return "Unk";
}

const char* nsW32API::ConvertFileInformationClassTo( const FILE_INFORMATION_CLASS FileInformationClass )
{
    switch( FileInformationClass )
    {
        case FileDirectoryInformation: { return "FileDirectoryInformation"; } break;
        case FileFullDirectoryInformation: { return "FileFullDirectoryInformation"; } break;
        case FileBothDirectoryInformation: { return "FileBothDirectoryInformation"; } break;
        case FileBasicInformation: { return "FileBasicInformation"; } break;
        case FileStandardInformation: { return "FileStandardInformation"; } break;
        case FileInternalInformation: { return "FileInternalInformation"; } break;
        case FileEaInformation: { return "FileEaInformation"; } break;
        case FileAccessInformation: { return "FileAccessInformation"; } break;
        case FileNameInformation: { return "FileNameInformation"; } break;
        case FileRenameInformation: { return "FileRenameInformation"; } break;
        case FileLinkInformation: { return "FileLinkInformation"; } break;
        case FileNamesInformation: { return "FileNamesInformation"; } break;
        case FileDispositionInformation: { return "FileDispositionInformation"; } break;
        case FilePositionInformation: { return "FilePositionInformation"; } break;
        case FileFullEaInformation: { return "FileFullEaInformation"; } break;
        case FileModeInformation: { return "FileModeInformation"; } break;
        case FileAlignmentInformation: { return "FileAlignmentInformation"; } break;
        case FileAllInformation: { return "FileAllInformation"; } break;
        case FileAllocationInformation: { return "FileAllocationInformation"; } break;
        case FileEndOfFileInformation: { return "FileEndOfFileInformation"; } break;
        case FileAlternateNameInformation: { return "FileAlternateNameInformation"; } break;
        case FileStreamInformation: { return "FileStreamInformation"; } break;
        case FilePipeInformation: { return "FilePipeInformation"; } break;
        case FilePipeLocalInformation: { return "FilePipeLocalInformation"; } break;
        case FilePipeRemoteInformation: { return "FilePipeRemoteInformation"; } break;
        case FileMailslotQueryInformation: { return "FileMailslotQueryInformation"; } break;
        case FileMailslotSetInformation: { return "FileMailslotSetInformation"; } break;
        case FileCompressionInformation: { return "FileCompressionInformation"; } break;
        case FileObjectIdInformation: { return "FileObjectIdInformation"; } break;
        case FileCompletionInformation: { return "FileCompletionInformation"; } break;
        case FileMoveClusterInformation: { return "FileMoveClusterInformation"; } break;
        case FileQuotaInformation: { return "FileQuotaInformation"; } break;
        case FileReparsePointInformation: { return "FileReparsePointInformation"; } break;
        case FileNetworkOpenInformation: { return "FileNetworkOpenInformation"; } break;
        case FileAttributeTagInformation: { return "FileAttributeTagInformation"; } break;
        case FileTrackingInformation: { return "FileTrackingInformation"; } break;
        case FileIdBothDirectoryInformation: { return "FileIdBothDirectoryInformation"; } break;
        case FileIdFullDirectoryInformation: { return "FileIdFullDirectoryInformation"; } break;
        case FileValidDataLengthInformation: { return "FileValidDataLengthInformation"; } break;
        case FileShortNameInformation: { return "FileShortNameInformation"; } break;
        case FileIoCompletionNotificationInformation: { return "FileIoCompletionNotificationInformation"; } break;
        case FileIoStatusBlockRangeInformation: { return "FileIoStatusBlockRangeInformation"; } break;
        case FileIoPriorityHintInformation: { return "FileIoPriorityHintInformation"; } break;
        case FileSfioReserveInformation: { return "FileSfioReserveInformation"; } break;
        case FileSfioVolumeInformation: { return "FileSfioVolumeInformation"; } break;
        case FileHardLinkInformation: { return "FileHardLinkInformation"; } break;
        case FileProcessIdsUsingFileInformation: { return "FileProcessIdsUsingFileInformation"; } break;
        case FileNormalizedNameInformation: { return "FileNormalizedNameInformation"; } break;
        case FileNetworkPhysicalNameInformation: { return "FileNetworkPhysicalNameInformation"; } break;
        case FileIdGlobalTxDirectoryInformation: { return "FileIdGlobalTxDirectoryInformation"; } break;
        case FileIsRemoteDeviceInformation: { return "FileIsRemoteDeviceInformation"; } break;
        case FileAttributeCacheInformation: { return "FileAttributeCacheInformation"; } break;
        case FileNumaNodeInformation: { return "FileNumaNodeInformation"; } break;
        case FileStandardLinkInformation: { return "FileStandardLinkInformation"; } break;
        case FileRemoteProtocolInformation: { return "FileRemoteProtocolInformation"; } break;
        
        case FileRenameInformationBypassAccessCheck: { return "FileRenameInformationBypassAccessCheck"; } break;
        case FileLinkInformationBypassAccessCheck: { return "FileLinkInformationBypassAccessCheck"; } break;
        case FileVolumeNameInformation: { return "FileVolumeNameInformation"; } break;
        case FileIdInformation: { return "FileIdInformation"; } break;
        case FileidExtdDirectoryInformation: { return "FileidExtdDirectoryInformation"; } break;
        case FileReplaceCompletionInformation: { return "FileReplaceCompletionInformation"; } break;
        case FileHardLinkFullIdInformation: { return "FileHardLinkFullIdInformation"; } break;
        case FileIdExtdBothDirectoryInformation: { return "FileIdExtdBothDirectoryInformation"; } break;
        case FileDispositionInformationEx: { return "FileDispositionInformationEx"; } break;
        case FileRenameInformationEx: { return "FileRenameInformationEx"; } break;
        case FileRenameInformationExBypassAccessCheck: { return "FileRenameInformationExBypassAccessCheck"; } break;
        case FileDesiredStorageClassInformation: { return "FileDesiredStorageClassInformation"; } break;
        case FileStatInformation: { return "FileStatInformation"; } break;
        case FileMemoryPartitionInformation: { return "FileMemoryPartitionInformation"; } break;
        case FileStatLxInformation: { return "FileStatLxInformation"; } break;
        case FileCaseSensitiveInformation: { return "FileCaseSensitiveInformation"; } break;
        case FileLinkInformationEx: { return "FileLinkInformationEx"; } break;
        case FileLinkInformationExBypassAccessCheck: { return "FileLinkInformationExBypassAccessCheck"; } break;
        case FileStorageReserveIdInformation: { return "FileStorageReserveIdInformation"; } break;
        case FileCaseSensitiveInformationForceAccessCheck: { return "FileCaseSensitiveInformationForceAccessCheck"; } break;
    }

    return "Unknown FileInformationClass";
}
