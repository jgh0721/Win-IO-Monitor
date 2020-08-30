#include "WinIOMonitor_W32API.hpp"

#include "utilities/osInfoMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

nsW32API::NtOsKrnlAPI nsW32API::NtOsKrnlAPIMgr;
nsW32API::FltMgrAPI nsW32API::FltMgrAPIMgr;

///////////////////////////////////////////////////////////////////////////////

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

const char* nsW32API::ConvertIRPMajorFunction( UCHAR MajorFunction )
{
    switch( MajorFunction )
    {
        case IRP_MJ_CREATE: { return "CREATE"; } break;
        case IRP_MJ_CREATE_NAMED_PIPE: { return "CREATE_PIPE"; } break;
        case IRP_MJ_CLOSE: { return "CLOSE"; } break;
        case IRP_MJ_READ: { return "READ"; } break;
        case IRP_MJ_WRITE: { return "WRITE"; } break;
        case IRP_MJ_QUERY_INFORMATION: { return "QUERY_INFO"; } break;
        case IRP_MJ_SET_INFORMATION: { return "SET_INFO"; } break;
        case IRP_MJ_QUERY_EA: { return "QUERY_EA"; } break;
        case IRP_MJ_SET_EA: { return "SET_EA"; } break;
        case IRP_MJ_FLUSH_BUFFERS: { return "FLUSH"; } break;
        case IRP_MJ_QUERY_VOLUME_INFORMATION: { return "QUERY_VOL"; } break;
        case IRP_MJ_SET_VOLUME_INFORMATION: { return "SET_VOL"; } break;
        case IRP_MJ_DIRECTORY_CONTROL: { return "DIR_CONTROL"; } break;
        case IRP_MJ_FILE_SYSTEM_CONTROL: { return "FS_CONTROL"; } break;
        case IRP_MJ_DEVICE_CONTROL: { return "DEV_CONTROL"; } break;
        case IRP_MJ_INTERNAL_DEVICE_CONTROL: { return "INTL_DEV_CONTROL"; } break;
        case IRP_MJ_SHUTDOWN: { return "SHUTDOWN"; } break;
        case IRP_MJ_LOCK_CONTROL: { return "LCK_CNTL"; } break;
        case IRP_MJ_CLEANUP: { return "CLEANUP"; } break;
        case IRP_MJ_CREATE_MAILSLOT: { return "CREATE_MAIL"; } break;
        case IRP_MJ_QUERY_SECURITY: { return "QUERY_SEC"; } break;
        case IRP_MJ_SET_SECURITY: { return "SET_SEC"; } break;
        case IRP_MJ_POWER: { return "POWER"; } break;
        case IRP_MJ_SYSTEM_CONTROL: { return "SYS_CONTROL"; } break;
        case IRP_MJ_DEVICE_CHANGE: { return "DEV_CHANGE"; } break;
        case IRP_MJ_QUERY_QUOTA: { return "QUERY_QUOTA"; } break;
        case IRP_MJ_SET_QUOTA: { return "SET_QUOTA"; } break;
        case IRP_MJ_PNP: { return "PNP"; } break;
    }

    return "Unk";
}

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

///////////////////////////////////////////////////////////////////////////////

void nsW32API::InitializeNtOsKrnlAPI( NtOsKrnlAPI* ntOsKrnlAPI )
{
    ASSERT( ntOsKrnlAPI != NULLPTR );

    UNICODE_STRING routineName;

    RtlInitUnicodeString( &routineName, L"PsSetLoadImageNotifyRoutine" );
    ntOsKrnlAPI->pfnPsSetLoadImageNotifyRoutine = ( NtOsKrnlAPI::PsSetLoadImageNotifyRoutine )MmGetSystemRoutineAddress( &routineName );

    RtlInitUnicodeString( &routineName, L"PsSetCreateProcessNotifyRoutine" );
    ntOsKrnlAPI->pfnPsSetCreateProcessNotifyRoutine = ( NtOsKrnlAPI::PsSetCreateProcessNotifyRoutine )MmGetSystemRoutineAddress( &routineName );

    RtlInitUnicodeString( &routineName, L"PsSetCreateProcessNotifyRoutineEx" );
    ntOsKrnlAPI->pfnPsSetCreateProcessNotifyRoutineEx = ( NtOsKrnlAPI::PsSetCreateProcessNotifyRoutineEx )MmGetSystemRoutineAddress( &routineName );

    RtlInitUnicodeString( &routineName, L"IoGetTransactionParameterBlock" );
    ntOsKrnlAPI->pfnIoGetTransactionParameterBlock = ( NtOsKrnlAPI::IoGetTransactionParameterBlock )MmGetSystemRoutineAddress( &routineName );

    RtlInitUnicodeString( &routineName, L"SeLocateProcessImageName" );
    ntOsKrnlAPI->pfnSeLocateProcessImageName = ( NtOsKrnlAPI::SeLocateProcessImageName )MmGetSystemRoutineAddress( &routineName );

    RtlInitUnicodeString( &routineName, L"ZwQueryInformationProcess" );
    ntOsKrnlAPI->pfnZwQueryInformationProcess = ( NtOsKrnlAPI::ZwQueryInformationProcess )MmGetSystemRoutineAddress( &routineName );
}

void nsW32API::InitializeFltMgrAPI( FltMgrAPI* fltMgrAPI )
{
    ASSERT( fltMgrAPI != NULLPTR );

    fltMgrAPI->pfnFltCreateFileEx           = ( FltMgrAPI::FltCreateFileEx ) FltGetRoutineAddress( "FltCreateFileEx" );
    fltMgrAPI->pfnFltCreateFileEx2          = ( FltMgrAPI::FltCreateFileEx2 ) FltGetRoutineAddress( "FltCreateFileEx2" );

    fltMgrAPI->pfnFltGetFileContext         = ( FltMgrAPI::FltGetFileContext ) FltGetRoutineAddress( "FltGetFileContext" );
    fltMgrAPI->pfnFltSetFileContext         = ( FltMgrAPI::FltSetFileContext ) FltGetRoutineAddress( "FltSetFileContext" );
    fltMgrAPI->pfnFltGetTransactionContext  = ( FltMgrAPI::FltGetTransactionContext ) FltGetRoutineAddress( "FltGetTransactionContext" );
    fltMgrAPI->pfnFltSetTransactionContext  = ( FltMgrAPI::FltSetTransactionContext ) FltGetRoutineAddress( "FltSetTransactionContext" );
    fltMgrAPI->pfnFltEnlistInTransaction    = ( FltMgrAPI::FltEnlistInTransaction ) FltGetRoutineAddress( "FltEnlistInTransaction" );

    fltMgrAPI->pfnFltIsVolumeWritable       = ( FltMgrAPI::FltIsVolumeWritable ) FltGetRoutineAddress( "FltIsVolumeWritable" );
    fltMgrAPI->pfnFltGetFileSystemType      = ( FltMgrAPI::FltGetFileSystemType ) FltGetRoutineAddress( "FltGetFileSystemType" );
}

NTSTATUS nsW32API::IsVolumeWritable( PVOID FltObject, PBOOLEAN IsWritable )
{
    // from https://greemate.tistory.com/entry/FltIsVolumeWritable

    if( nsUtils::VerifyVersionInfoEx( 6, ">=" ) == true )
        return FltMgrAPIMgr.pfnFltIsVolumeWritable( FltObject, IsWritable );

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
