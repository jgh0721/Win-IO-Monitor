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

const char* nsW32API::ConvertFsInformationClassTo( const FS_INFORMATION_CLASS FsInformationClass )
{
    switch( FsInformationClass )
    {
        case FileFsVolumeInformation: { return "FileFsVolumeInformation"; } break;
        case FileFsLabelInformation: { return "FileFsLabelInformation"; } break;
        case FileFsSizeInformation: { return "FileFsSizeInformation"; } break;
        case FileFsDeviceInformation: { return "FileFsDeviceInformation"; } break;
        case FileFsAttributeInformation: { return "FileFsAttributeInformation"; } break;
        case FileFsControlInformation: { return "FileFsControlInformation"; } break;
        case FileFsFullSizeInformation: { return "FileFsFullSizeInformation"; } break;
        case FileFsObjectIdInformation: { return "FileFsObjectIdInformation"; } break;
        case FileFsDriverPathInformation: { return "FileFsDriverPathInformation"; } break;
        case FileFsVolumeFlagsInformation: { return "FileFsVolumeFlagsInformation"; } break;
        case FileFsSectorSizeInformation: { return "FileFsSectorSizeInformation"; } break;
        case FileFsDataCopyInformation: { return "FileFsDataCopyInformation"; } break;
        case FileFsMetadataSizeInformation: { return "FileFsMetadataSizeInformation"; } break;
        case FileFsFullSizeInformationEx: { return "FileFsFullSizeInformationEx"; } break;
    }

    return "Unknown FsInformationClass";
}

void nsW32API::PrintOutIrpFlags( char* PrintBuffer, ULONG BufferSize, ULONG IrpFlags )
{
    if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

    if( BooleanFlagOn( IrpFlags, IRP_BUFFERED_IO ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_BUFFERED_IO|" );

    if( BooleanFlagOn( IrpFlags, IRP_CLOSE_OPERATION ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_CLOSE_OPERATION|" );

    if( BooleanFlagOn( IrpFlags, IRP_DEALLOCATE_BUFFER ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_DEALLOCATE_BUFFER|" );

    if( BooleanFlagOn( IrpFlags, IRP_INPUT_OPERATION ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_INPUT_OPERATION|" );

    if( BooleanFlagOn( IrpFlags, IRP_NOCACHE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_NOCACHE|" );

    if( BooleanFlagOn( IrpFlags, IRP_PAGING_IO ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_PAGING_IO|" );

    if( BooleanFlagOn( IrpFlags, IRP_SYNCHRONOUS_API ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_SYNCHRONOUS_API|" );

    if( BooleanFlagOn( IrpFlags, IRP_SYNCHRONOUS_PAGING_IO ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_SYNCHRONOUS_PAGING_IO|" );

    //if( BooleanFlagOn( IrpFlags, IRP_MOUNT_COMPLETION ) )
    //    RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_MOUNT_COMPLETION|" );

    if( BooleanFlagOn( IrpFlags, IRP_CREATE_OPERATION ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_CREATE_OPERATION|" );

    if( BooleanFlagOn( IrpFlags, IRP_READ_OPERATION ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_READ_OPERATION|" );

    if( BooleanFlagOn( IrpFlags, IRP_WRITE_OPERATION ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_WRITE_OPERATION|" );

    if( BooleanFlagOn( IrpFlags, IRP_DEFER_IO_COMPLETION ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_DEFER_IO_COMPLETION|" );

    if( BooleanFlagOn( IrpFlags, IRP_ASSOCIATED_IRP ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_ASSOCIATED_IRP|" );

    if( BooleanFlagOn( IrpFlags, IRP_OB_QUERY_NAME ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_OB_QUERY_NAME|" );

    if( BooleanFlagOn( IrpFlags, IRP_HOLD_DEVICE_QUEUE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_HOLD_DEVICE_QUEUE|" );

    if( BooleanFlagOn( IrpFlags, IRP_UM_DRIVER_INITIATED_IO ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "IRP_UM_DRIVER_INITIATED_IO|" );
}

void nsW32API::PrintOutOperationFlags( char* PrintBuffer, ULONG BufferSize, ULONG OperationFlags )
{
    if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;
    
    if( BooleanFlagOn( OperationFlags, SL_CASE_SENSITIVE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_CASE_SENSITIVE|" );
    if( BooleanFlagOn( OperationFlags, SL_EXCLUSIVE_LOCK ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_EXCLUSIVE_LOCK|" );
    if( BooleanFlagOn( OperationFlags, SL_FAIL_IMMEDIATELY ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_FAIL_IMMEDIATELY|" );
    if( BooleanFlagOn( OperationFlags, SL_FORCE_ACCESS_CHECK ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_FORCE_ACCESS_CHECK|" );
    if( BooleanFlagOn( OperationFlags, SL_FORCE_DIRECT_WRITE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_FORCE_DIRECT_WRITE|" );
    if( BooleanFlagOn( OperationFlags, SL_INDEX_SPECIFIED ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_INDEX_SPECIFIED|" );
    if( BooleanFlagOn( OperationFlags, SL_OPEN_PAGING_FILE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_OPEN_PAGING_FILE|" );
    if( BooleanFlagOn( OperationFlags, SL_OPEN_TARGET_DIRECTORY ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_OPEN_TARGET_DIRECTORY|" );
    if( BooleanFlagOn( OperationFlags, SL_OVERRIDE_VERIFY_VOLUME ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_OVERRIDE_VERIFY_VOLUME|" );
    if( BooleanFlagOn( OperationFlags, SL_RESTART_SCAN ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_RESTART_SCAN|" );
    if( BooleanFlagOn( OperationFlags, SL_RETURN_SINGLE_ENTRY ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_RETURN_SINGLE_ENTRY|" );
    if( BooleanFlagOn( OperationFlags, SL_WATCH_TREE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_WATCH_TREE|" );
    if( BooleanFlagOn( OperationFlags, SL_WRITE_THROUGH ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SL_WRITE_THROUGH|" );
}

void nsW32API::PrintOutCreateOptions( char* PrintBuffer, ULONG BufferSize, ULONG CreateOptions )
{
    if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

    if( BooleanFlagOn( CreateOptions, FILE_DIRECTORY_FILE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_DIRECTORY_FILE|" );
    if( BooleanFlagOn( CreateOptions, FILE_WRITE_THROUGH ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_WRITE_THROUGH|" );
    if( BooleanFlagOn( CreateOptions, FILE_SEQUENTIAL_ONLY ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_SEQUENTIAL_ONLY|" );
    if( BooleanFlagOn( CreateOptions, FILE_NO_INTERMEDIATE_BUFFERING ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_NO_INTERMEDIATE_BUFFERING|" );

    if( BooleanFlagOn( CreateOptions, FILE_SYNCHRONOUS_IO_ALERT ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_SYNCHRONOUS_IO_ALERT|" );
    if( BooleanFlagOn( CreateOptions, FILE_SYNCHRONOUS_IO_NONALERT ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_SYNCHRONOUS_IO_NONALERT|" );
    if( BooleanFlagOn( CreateOptions, FILE_NON_DIRECTORY_FILE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_NON_DIRECTORY_FILE|" );
    if( BooleanFlagOn( CreateOptions, FILE_CREATE_TREE_CONNECTION ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_CREATE_TREE_CONNECTION|" );

    if( BooleanFlagOn( CreateOptions, FILE_COMPLETE_IF_OPLOCKED ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_COMPLETE_IF_OPLOCKED|" );
    if( BooleanFlagOn( CreateOptions, FILE_NO_EA_KNOWLEDGE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_NO_EA_KNOWLEDGE|" );
    if( BooleanFlagOn( CreateOptions, FILE_OPEN_REMOTE_INSTANCE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_OPEN_REMOTE_INSTANCE|" );
    if( BooleanFlagOn( CreateOptions, FILE_RANDOM_ACCESS ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RANDOM_ACCESS|" );

    if( BooleanFlagOn( CreateOptions, FILE_DELETE_ON_CLOSE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_DELETE_ON_CLOSE|" );
    if( BooleanFlagOn( CreateOptions, FILE_OPEN_BY_FILE_ID ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_OPEN_BY_FILE_ID|" );
    if( BooleanFlagOn( CreateOptions, FILE_OPEN_FOR_BACKUP_INTENT ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_OPEN_FOR_BACKUP_INTENT|" );
    if( BooleanFlagOn( CreateOptions, FILE_NO_COMPRESSION ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_NO_COMPRESSION|" );

    if( BooleanFlagOn( CreateOptions, FILE_OPEN_REQUIRING_OPLOCK ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_OPEN_REQUIRING_OPLOCK|" );
    if( BooleanFlagOn( CreateOptions, FILE_DISALLOW_EXCLUSIVE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_DISALLOW_EXCLUSIVE|" );

    if( BooleanFlagOn( CreateOptions, FILE_RESERVE_OPFILTER ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RESERVE_OPFILTER|" );
    if( BooleanFlagOn( CreateOptions, FILE_OPEN_REPARSE_POINT ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_OPEN_REPARSE_POINT|" );
    if( BooleanFlagOn( CreateOptions, FILE_OPEN_NO_RECALL ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_OPEN_NO_RECALL|" );
    if( BooleanFlagOn( CreateOptions, FILE_OPEN_FOR_FREE_SPACE_QUERY ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_OPEN_FOR_FREE_SPACE_QUERY|" );
}

void nsW32API::PrintOutCreateDesiredAccess( char* PrintBuffer, ULONG BufferSize, ULONG DesiredAccess )
{
    if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

    if( BooleanFlagOn( DesiredAccess, FILE_READ_DATA ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_READ_DATA|" );
    if( BooleanFlagOn( DesiredAccess, FILE_READ_ATTRIBUTES ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_READ_ATTRIBUTES|" );
    if( BooleanFlagOn( DesiredAccess, FILE_READ_EA ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_READ_EA|" );

    if( BooleanFlagOn( DesiredAccess, FILE_WRITE_DATA ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_WRITE_DATA|" );
    if( BooleanFlagOn( DesiredAccess, FILE_WRITE_ATTRIBUTES ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_WRITE_ATTRIBUTES|" );
    if( BooleanFlagOn( DesiredAccess, FILE_WRITE_EA ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_WRITE_EA|" );

    if( BooleanFlagOn( DesiredAccess, DELETE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "DELETE|" );
    if( BooleanFlagOn( DesiredAccess, READ_CONTROL ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "READ_CONTROL|" );
    if( BooleanFlagOn( DesiredAccess, WRITE_DAC ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "WRITE_DAC|" );
    if( BooleanFlagOn( DesiredAccess, WRITE_OWNER ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "WRITE_OWNER|" );
    if( BooleanFlagOn( DesiredAccess, SYNCHRONIZE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "SYNCHRONIZE|" );
    if( BooleanFlagOn( DesiredAccess, FILE_EXECUTE ) )
        RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_EXECUTE|" );
}