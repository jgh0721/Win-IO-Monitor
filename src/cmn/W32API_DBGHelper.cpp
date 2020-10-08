#include "W32API_DBGHelper.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsW32API
{
    void FormatIrpFlags( char* PrintBuffer, ULONG BufferSize, ULONG IrpFlags )
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

    const char* ConvertIrpMajorFuncTo( UCHAR MajorFunction )
    {
        return FltGetIrpName( MajorFunction );
    }

    void FormatIrpMajorFunc( char* PrintBuffer, ULONG BufferSize, UCHAR MajorFunction )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        RtlStringCbCatA( PrintBuffer, BufferSize, ConvertIrpMajorFuncTo( MajorFunction ) );
    }

    const char* ConvertIrpMinorFuncTo( UCHAR MajorFunction, UCHAR MinorFunction )
    {
        switch( MajorFunction )
        {
            case IRP_MJ_READ:
            case IRP_MJ_WRITE:
            {
                switch( MinorFunction )
                {
                    case IRP_MN_NORMAL: return "NORMAL";
                    case IRP_MN_DPC: return "DPC";
                    case IRP_MN_MDL: return "MDL";
                    case IRP_MN_COMPLETE: return "COMPLETE";
                    case IRP_MN_COMPRESSED: return "COMPRESSED";

                    case IRP_MN_MDL_DPC: return "MDL_DPC";
                    case IRP_MN_COMPLETE_MDL: return "COMPLETE_MDL";
                    case IRP_MN_COMPLETE_MDL_DPC: return "COMPLETE_MDL_DPC";

                    case IRP_MN_QUERY_LEGACY_BUS_INFORMATION: return "QUERY_LEGACY_BUS";
                }
            } break;

            case IRP_MJ_DIRECTORY_CONTROL:
            {
                switch( MinorFunction )
                {
                    case IRP_MN_QUERY_DIRECTORY: return "QUERY_DIRECTORY";
                    case IRP_MN_NOTIFY_CHANGE_DIRECTORY: return "NOTIFY_CHANGE_DIRECTORY";
                }
            } break;

            case IRP_MJ_FILE_SYSTEM_CONTROL:
            {
                switch( MinorFunction )
                {
                    case IRP_MN_USER_FS_REQUEST: { return "USER_FS_REQUEST"; } break;
                    case IRP_MN_MOUNT_VOLUME: { return "MOUNT_VOLUME"; } break;
                    case IRP_MN_VERIFY_VOLUME: { return "VERIFY_VOLUME"; } break;
                    case IRP_MN_LOAD_FILE_SYSTEM: { return "LOAD_FILE_SYSTEM"; } break;
                        // case IRP_MN_TRACK_LINK: {} break;   
                    case IRP_MN_KERNEL_CALL: { return "KERNEL_CALL"; } break;
                }
            } break;
            case IRP_MJ_LOCK_CONTROL:
            {
                switch( MinorFunction )
                {
                    case IRP_MN_LOCK: { return "LOCK"; } break;
                    case IRP_MN_UNLOCK_SINGLE: { return "UNLOCK_SINGLE"; } break;
                    case IRP_MN_UNLOCK_ALL: { return "UNLOCK_ALL"; } break;
                    case IRP_MN_UNLOCK_ALL_BY_KEY: { return "UNLOCK_ALL_BY_KEY"; } break;
                }
            } break;

            case IRP_MJ_SYSTEM_CONTROL:
            {
                switch( MinorFunction )
                {
                    case IRP_MN_QUERY_ALL_DATA: { return "QUERY_ALL_DATA"; } break;
                    case IRP_MN_QUERY_SINGLE_INSTANCE: { return "QUERY_SINGLE_INSTANCE"; } break;
                    case IRP_MN_CHANGE_SINGLE_INSTANCE: { return "CHANGE_SINGLE_INSTANCE"; } break;
                    case IRP_MN_CHANGE_SINGLE_ITEM: { return "CHANGE_SINGLE_ITEM"; } break;
                    case IRP_MN_ENABLE_EVENTS: { return "ENABLE_EVENTS"; } break;
                    case IRP_MN_DISABLE_EVENTS: { return "DISABLE_EVENTS"; } break;
                    case IRP_MN_ENABLE_COLLECTION: { return "ENABLE_COLLECTION"; } break;
                    case IRP_MN_DISABLE_COLLECTION: { return "DISABLE_COLLECTION"; } break;
                    case IRP_MN_REGINFO: { return "REGINFO"; } break;
                    case IRP_MN_EXECUTE_METHOD: { return "EXECUTE_METHOD"; } break;
                        // Minor code 0x0a is reserved
                    case 0x0a: { return "RESERVED(0x0a)"; } break;
                    case IRP_MN_REGINFO_EX: { return "REGINFO_EX"; } break;
                        // Minor code 0x0c is reserved
                    case 0x0c: { return "RESERVED(0x0c)"; } break;
                }
            } break;
            case IRP_MJ_PNP:
            {
                switch( MinorFunction )
                {
                    case IRP_MN_START_DEVICE: { return "START_DEVICE"; } break;
                    case IRP_MN_QUERY_REMOVE_DEVICE: { return "QUERY_REMOVE_DEVICE"; } break;
                    case IRP_MN_REMOVE_DEVICE: { return "REMOVE_DEVICE"; } break;
                    case IRP_MN_CANCEL_REMOVE_DEVICE: { return "CANCEL_REMOVE_DEVICE"; } break;
                    case IRP_MN_STOP_DEVICE: { return "STOP_DEVICE"; } break;
                    case IRP_MN_QUERY_STOP_DEVICE: { return "QUERY_STOP_DEVICE"; } break;
                    case IRP_MN_CANCEL_STOP_DEVICE: { return "CANCEL_STOP_DEVICE"; } break;

                    case IRP_MN_QUERY_DEVICE_RELATIONS: { return "QUERY_DEVICE_RELATIONS"; } break;
                    case IRP_MN_QUERY_INTERFACE: { return "QUERY_INTERFACE"; } break;
                    case IRP_MN_QUERY_CAPABILITIES: { return "QUERY_CAPABILITIES"; } break;
                    case IRP_MN_QUERY_RESOURCES: { return "QUERY_RESOURCES"; } break;
                    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: { return "QUERY_RESOURCE_REQUIREMENTS"; } break;
                    case IRP_MN_QUERY_DEVICE_TEXT: { return "QUERY_DEVICE_TEXT"; } break;
                    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: { return "FILTER_RESOURCE_REQUIREMENTS"; } break;

                    case IRP_MN_READ_CONFIG: { return "READ_CONFIG"; } break;
                    case IRP_MN_WRITE_CONFIG: { return "WRITE_CONFIG"; } break;
                    case IRP_MN_EJECT: { return "EJECT"; } break;
                    case IRP_MN_SET_LOCK: { return "SET_LOCK"; } break;
                    case IRP_MN_QUERY_ID: { return "QUERY_ID"; } break;
                    case IRP_MN_QUERY_PNP_DEVICE_STATE: { return "QUERY_PNP_DEVICE_STATE"; } break;
                    case IRP_MN_QUERY_BUS_INFORMATION: { return "QUERY_BUS_INFORMATION"; } break;
                    case IRP_MN_DEVICE_USAGE_NOTIFICATION: { return "DEVICE_USAGE_NOTIFICATION"; } break;
                    case IRP_MN_SURPRISE_REMOVAL: { return "SURPRISE_REMOVAL"; } break;
                    case IRP_MN_DEVICE_ENUMERATED: { return "DEVICE_ENUMERATED"; } break;
                }
            } break;
            case IRP_MJ_POWER:
            {
                switch( MinorFunction )
                {
                    case IRP_MN_WAIT_WAKE: { return "WAIT_WAKE"; } break;
                    case IRP_MN_POWER_SEQUENCE: { return "POWER_SEQUENCE"; } break;
                    case IRP_MN_SET_POWER: { return "SET_POWER"; } break;
                    case IRP_MN_QUERY_POWER: { return "QUERY_POWER"; } break;
                }
            } break;
        }

        return "None";
    }

    void FormatIrpMinorFunc( char* PrintBuffer, ULONG BufferSize, UCHAR MajorFunction, UCHAR MinorFunction )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        RtlStringCbCatA( PrintBuffer, BufferSize, ConvertIrpMinorFuncTo( MajorFunction, MinorFunction ) );
    }

    void FormatTopLevelIrp( char* PrintBuffer, ULONG BufferSize )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        PIRP Top = IoGetTopLevelIrp();

        if( (LONG_PTR)Top == FSRTL_FSP_TOP_LEVEL_IRP )
        {
            RtlStringCbCatA( PrintBuffer, BufferSize, "FSP" );
        }
        else if( ( LONG_PTR )Top == FSRTL_CACHE_TOP_LEVEL_IRP )
        {
            RtlStringCbCatA( PrintBuffer, BufferSize, "CACHE" );
        }
        else if( ( LONG_PTR )Top == FSRTL_MOD_WRITE_TOP_LEVEL_IRP )
        {
            RtlStringCbCatA( PrintBuffer, BufferSize, "MOD_WRITE" );
        }
        else if( ( LONG_PTR )Top == FSRTL_FAST_IO_TOP_LEVEL_IRP )
        {
            RtlStringCbCatA( PrintBuffer, BufferSize, "FAST_IO" );
        }
        else
        {
            auto len = strlen( PrintBuffer );
            RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "%p", Top );
        }
    }

    void FormatOperationFlags( char* PrintBuffer, ULONG BufferSize, ULONG OperationFlags )
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

    const char* ConvertFileInformationClassTo( const FILE_INFORMATION_CLASS FileInformationClass )
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
            case FileIdExtdDirectoryInformation: { return "FileIdExtdDirectoryInformation"; } break;
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

        return "Unknown";
    }

    void FormatFileInformationClass( char* PrintBuffer, ULONG BufferSize, FILE_INFORMATION_CLASS FileInformationClass )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        RtlStringCbCatA( PrintBuffer, BufferSize, ConvertFileInformationClassTo( FileInformationClass ) );
    }

    const char* ConvertFsInformationClassTo( const FS_INFORMATION_CLASS FsInformationClass )
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

        return "Unknown";
    }

    void FormatFsInformationClass( char* PrintBuffer, ULONG BufferSize, FS_INFORMATION_CLASS FsInformationClass )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        RtlStringCbCatA( PrintBuffer, BufferSize, ConvertFsInformationClassTo( FsInformationClass ) );
    }

    const char* ConvertBuiltInFsControlCodeTo( ULONG FsControlCode )
    {
        switch( FsControlCode )
        {
            case FSCTL_REQUEST_OPLOCK_LEVEL_1: { return "FSCTL_REQUEST_OPLOCK_LEVEL_1"; } break;
            case FSCTL_REQUEST_OPLOCK_LEVEL_2: { return "FSCTL_REQUEST_OPLOCK_LEVEL_2"; } break;
            case FSCTL_REQUEST_BATCH_OPLOCK: { return "FSCTL_REQUEST_BATCH_OPLOCK"; } break;
            case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE: { return "FSCTL_OPLOCK_BREAK_ACKNOWLEDGE"; } break;
            case FSCTL_OPBATCH_ACK_CLOSE_PENDING: { return "FSCTL_OPBATCH_ACK_CLOSE_PENDING"; } break;
            case FSCTL_OPLOCK_BREAK_NOTIFY: { return "FSCTL_OPLOCK_BREAK_NOTIFY"; } break;
            case FSCTL_LOCK_VOLUME: { return "FSCTL_LOCK_VOLUME"; } break;
            case FSCTL_UNLOCK_VOLUME: { return "FSCTL_UNLOCK_VOLUME"; } break;
            case FSCTL_DISMOUNT_VOLUME: { return "FSCTL_DISMOUNT_VOLUME"; } break;

            case FSCTL_IS_VOLUME_MOUNTED: { return "FSCTL_IS_VOLUME_MOUNTED"; } break;
            case FSCTL_IS_PATHNAME_VALID: { return "FSCTL_IS_PATHNAME_VALID"; } break;
            case FSCTL_MARK_VOLUME_DIRTY: { return "FSCTL_MARK_VOLUME_DIRTY"; } break;

            case FSCTL_QUERY_RETRIEVAL_POINTERS: { return "FSCTL_QUERY_RETRIEVAL_POINTERS"; } break;
            case FSCTL_GET_COMPRESSION: { return "FSCTL_GET_COMPRESSION"; } break;
            case FSCTL_SET_COMPRESSION: { return "FSCTL_SET_COMPRESSION"; } break;


            case FSCTL_SET_BOOTLOADER_ACCESSED: { return "FSCTL_SET_BOOTLOADER_ACCESSED"; } break;
            case FSCTL_OPLOCK_BREAK_ACK_NO_2: { return "FSCTL_OPLOCK_BREAK_ACK_NO_2"; } break;
            case FSCTL_INVALIDATE_VOLUMES: { return "FSCTL_INVALIDATE_VOLUMES"; } break;
            case FSCTL_QUERY_FAT_BPB: { return "FSCTL_QUERY_FAT_BPB"; } break;
            case FSCTL_REQUEST_FILTER_OPLOCK: { return "FSCTL_REQUEST_FILTER_OPLOCK"; } break;
            case FSCTL_FILESYSTEM_GET_STATISTICS: { return "FSCTL_FILESYSTEM_GET_STATISTICS"; } break;

            case FSCTL_GET_NTFS_VOLUME_DATA: { return "FSCTL_GET_NTFS_VOLUME_DATA"; } break;
            case FSCTL_GET_NTFS_FILE_RECORD: { return "FSCTL_GET_NTFS_FILE_RECORD"; } break;
            case FSCTL_GET_VOLUME_BITMAP: { return "FSCTL_GET_VOLUME_BITMAP"; } break;
            case FSCTL_GET_RETRIEVAL_POINTERS: { return "FSCTL_GET_RETRIEVAL_POINTERS"; } break;
            case FSCTL_MOVE_FILE: { return "FSCTL_MOVE_FILE"; } break;
            case FSCTL_IS_VOLUME_DIRTY: { return "FSCTL_IS_VOLUME_DIRTY"; } break;

            case FSCTL_ALLOW_EXTENDED_DASD_IO: { return "FSCTL_ALLOW_EXTENDED_DASD_IO"; } break;



            case FSCTL_FIND_FILES_BY_SID: { return "FSCTL_FIND_FILES_BY_SID"; } break;


            case FSCTL_SET_OBJECT_ID: { return "FSCTL_SET_OBJECT_ID"; } break;
            case FSCTL_GET_OBJECT_ID: { return "FSCTL_GET_OBJECT_ID"; } break;
            case FSCTL_DELETE_OBJECT_ID: { return "FSCTL_DELETE_OBJECT_ID"; } break;
            case FSCTL_SET_REPARSE_POINT: { return "FSCTL_SET_REPARSE_POINT"; } break;
            case FSCTL_GET_REPARSE_POINT: { return "FSCTL_GET_REPARSE_POINT"; } break;
            case FSCTL_DELETE_REPARSE_POINT: { return "FSCTL_DELETE_REPARSE_POINT"; } break;
            case FSCTL_ENUM_USN_DATA: { return "FSCTL_ENUM_USN_DATA"; } break;
            case FSCTL_SECURITY_ID_CHECK: { return "FSCTL_SECURITY_ID_CHECK"; } break;
            case FSCTL_READ_USN_JOURNAL: { return "FSCTL_READ_USN_JOURNAL"; } break;
            case FSCTL_SET_OBJECT_ID_EXTENDED: { return "FSCTL_SET_OBJECT_ID_EXTENDED"; } break;
            case FSCTL_CREATE_OR_GET_OBJECT_ID: { return "FSCTL_CREATE_OR_GET_OBJECT_ID"; } break;
            case FSCTL_SET_SPARSE: { return "FSCTL_SET_SPARSE"; } break;
            case FSCTL_SET_ZERO_DATA: { return "FSCTL_SET_ZERO_DATA"; } break;
            case FSCTL_QUERY_ALLOCATED_RANGES: { return "FSCTL_QUERY_ALLOCATED_RANGES"; } break;
            case FSCTL_ENABLE_UPGRADE: { return "FSCTL_ENABLE_UPGRADE"; } break;

            case FSCTL_SET_ENCRYPTION: { return "FSCTL_SET_ENCRYPTION"; } break;
            case FSCTL_ENCRYPTION_FSCTL_IO: { return "FSCTL_ENCRYPTION_FSCTL_IO"; } break;
            case FSCTL_WRITE_RAW_ENCRYPTED: { return "FSCTL_WRITE_RAW_ENCRYPTED"; } break;
            case FSCTL_READ_RAW_ENCRYPTED: { return "FSCTL_READ_RAW_ENCRYPTED"; } break;
            case FSCTL_CREATE_USN_JOURNAL: { return "FSCTL_CREATE_USN_JOURNAL"; } break;
            case FSCTL_READ_FILE_USN_DATA: { return "FSCTL_READ_FILE_USN_DATA"; } break;
            case FSCTL_WRITE_USN_CLOSE_RECORD: { return "FSCTL_WRITE_USN_CLOSE_RECORD"; } break;
            case FSCTL_EXTEND_VOLUME: { return "FSCTL_EXTEND_VOLUME"; } break;
            case FSCTL_QUERY_USN_JOURNAL: { return "FSCTL_QUERY_USN_JOURNAL"; } break;
            case FSCTL_DELETE_USN_JOURNAL: { return "FSCTL_DELETE_USN_JOURNAL"; } break;
            case FSCTL_MARK_HANDLE: { return "FSCTL_MARK_HANDLE"; } break;
            case FSCTL_SIS_COPYFILE: { return "FSCTL_SIS_COPYFILE"; } break;
            case FSCTL_SIS_LINK_FILES: { return "FSCTL_SIS_LINK_FILES"; } break;



            case FSCTL_RECALL_FILE: { return "FSCTL_RECALL_FILE"; } break;

            case FSCTL_READ_FROM_PLEX: { return "FSCTL_READ_FROM_PLEX"; } break;
            case FSCTL_FILE_PREFETCH: { return "FSCTL_FILE_PREFETCH"; } break;

            case FSCTL_MAKE_MEDIA_COMPATIBLE: { return "FSCTL_MAKE_MEDIA_COMPATIBLE"; } break;
            case FSCTL_SET_DEFECT_MANAGEMENT: { return "FSCTL_SET_DEFECT_MANAGEMENT"; } break;
            case FSCTL_QUERY_SPARING_INFO: { return "FSCTL_QUERY_SPARING_INFO"; } break;
            case FSCTL_QUERY_ON_DISK_VOLUME_INFO: { return "FSCTL_QUERY_ON_DISK_VOLUME_INFO"; } break;
            case FSCTL_SET_VOLUME_COMPRESSION_STATE: { return "FSCTL_SET_VOLUME_COMPRESSION_STATE"; } break;

            case FSCTL_TXFS_MODIFY_RM: { return "FSCTL_TXFS_MODIFY_RM"; } break;
            case FSCTL_TXFS_QUERY_RM_INFORMATION: { return "FSCTL_TXFS_QUERY_RM_INFORMATION"; } break;

            case FSCTL_TXFS_ROLLFORWARD_REDO: { return "FSCTL_TXFS_ROLLFORWARD_REDO"; } break;
            case FSCTL_TXFS_ROLLFORWARD_UNDO: { return "FSCTL_TXFS_ROLLFORWARD_UNDO"; } break;
            case FSCTL_TXFS_START_RM: { return "FSCTL_TXFS_START_RM"; } break;
            case FSCTL_TXFS_SHUTDOWN_RM: { return "FSCTL_TXFS_SHUTDOWN_RM"; } break;
            case FSCTL_TXFS_READ_BACKUP_INFORMATION: { return "FSCTL_TXFS_READ_BACKUP_INFORMATION"; } break;
            case FSCTL_TXFS_WRITE_BACKUP_INFORMATION: { return "FSCTL_TXFS_WRITE_BACKUP_INFORMATION"; } break;
            case FSCTL_TXFS_CREATE_SECONDARY_RM: { return "FSCTL_TXFS_CREATE_SECONDARY_RM"; } break;
            case FSCTL_TXFS_GET_METADATA_INFO: { return "FSCTL_TXFS_GET_METADATA_INFO"; } break;
            case FSCTL_TXFS_GET_TRANSACTED_VERSION: { return "FSCTL_TXFS_GET_TRANSACTED_VERSION"; } break;

            case FSCTL_TXFS_SAVEPOINT_INFORMATION: { return "FSCTL_TXFS_SAVEPOINT_INFORMATION"; } break;
            case FSCTL_TXFS_CREATE_MINIVERSION: { return "FSCTL_TXFS_CREATE_MINIVERSION"; } break;



            case FSCTL_TXFS_TRANSACTION_ACTIVE: { return "FSCTL_TXFS_TRANSACTION_ACTIVE"; } break;
            case FSCTL_SET_ZERO_ON_DEALLOCATION: { return "FSCTL_SET_ZERO_ON_DEALLOCATION"; } break;
            case FSCTL_SET_REPAIR: { return "FSCTL_SET_REPAIR"; } break;
            case FSCTL_GET_REPAIR: { return "FSCTL_GET_REPAIR"; } break;
            case FSCTL_WAIT_FOR_REPAIR: { return "FSCTL_WAIT_FOR_REPAIR"; } break;

            case FSCTL_INITIATE_REPAIR: { return "FSCTL_INITIATE_REPAIR"; } break;
            case FSCTL_CSC_INTERNAL: { return "FSCTL_CSC_INTERNAL"; } break;
            case FSCTL_SHRINK_VOLUME: { return "FSCTL_SHRINK_VOLUME"; } break;
            case FSCTL_SET_SHORT_NAME_BEHAVIOR: { return "FSCTL_SET_SHORT_NAME_BEHAVIOR"; } break;
            case FSCTL_DFSR_SET_GHOST_HANDLE_STATE: { return "FSCTL_DFSR_SET_GHOST_HANDLE_STATE"; } break;

            case FSCTL_TXFS_LIST_TRANSACTION_LOCKED_FILES: { return "FSCTL_TXFS_LIST_TRANSACTION_LOCKED_FILES"; } break;

            case FSCTL_TXFS_LIST_TRANSACTIONS: { return "FSCTL_TXFS_LIST_TRANSACTIONS"; } break;
            case FSCTL_QUERY_PAGEFILE_ENCRYPTION: { return "FSCTL_QUERY_PAGEFILE_ENCRYPTION"; } break;

            case FSCTL_RESET_VOLUME_ALLOCATION_HINTS: { return "FSCTL_RESET_VOLUME_ALLOCATION_HINTS"; } break;

            case FSCTL_QUERY_DEPENDENT_VOLUME: { return "FSCTL_QUERY_DEPENDENT_VOLUME"; } break;
            case FSCTL_SD_GLOBAL_CHANGE: { return "FSCTL_SD_GLOBAL_CHANGE"; } break;

            case FSCTL_TXFS_READ_BACKUP_INFORMATION2: { return "FSCTL_TXFS_READ_BACKUP_INFORMATION2"; } break;

            case FSCTL_LOOKUP_STREAM_FROM_CLUSTER: { return "FSCTL_LOOKUP_STREAM_FROM_CLUSTER"; } break;
            case FSCTL_TXFS_WRITE_BACKUP_INFORMATION2: { return "FSCTL_TXFS_WRITE_BACKUP_INFORMATION2"; } break;
            case FSCTL_FILE_TYPE_NOTIFICATION: { return "FSCTL_FILE_TYPE_NOTIFICATION"; } break;


            case FSCTL_GET_BOOT_AREA_INFO: { return "FSCTL_GET_BOOT_AREA_INFO"; } break;
            case FSCTL_GET_RETRIEVAL_POINTER_BASE: { return "FSCTL_GET_RETRIEVAL_POINTER_BASE"; } break;
            case FSCTL_SET_PERSISTENT_VOLUME_STATE: { return "FSCTL_SET_PERSISTENT_VOLUME_STATE"; } break;
            case FSCTL_QUERY_PERSISTENT_VOLUME_STATE: { return "FSCTL_QUERY_PERSISTENT_VOLUME_STATE"; } break;

            case FSCTL_REQUEST_OPLOCK: { return "FSCTL_REQUEST_OPLOCK"; } break;

            case FSCTL_CSV_TUNNEL_REQUEST: { return "FSCTL_CSV_TUNNEL_REQUEST"; } break;
            case FSCTL_IS_CSV_FILE: { return "FSCTL_IS_CSV_FILE"; } break;

            case FSCTL_QUERY_FILE_SYSTEM_RECOGNITION: { return "FSCTL_QUERY_FILE_SYSTEM_RECOGNITION"; } break;
            case FSCTL_CSV_GET_VOLUME_PATH_NAME: { return "FSCTL_CSV_GET_VOLUME_PATH_NAME"; } break;
            case FSCTL_CSV_GET_VOLUME_NAME_FOR_VOLUME_MOUNT_POINT: { return "FSCTL_CSV_GET_VOLUME_NAME_FOR_VOLUME_MOUNT_POINT"; } break;
            case FSCTL_CSV_GET_VOLUME_PATH_NAMES_FOR_VOLUME_NAME: { return "FSCTL_CSV_GET_VOLUME_PATH_NAMES_FOR_VOLUME_NAME"; } break;
            case FSCTL_IS_FILE_ON_CSV_VOLUME: { return "FSCTL_IS_FILE_ON_CSV_VOLUME"; } break;
        }

        return "";
    }

    void FormatBuiltInFsControlCode( char* PrintBuffer, ULONG BufferSize, ULONG FsControlCode )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        RtlStringCbCatA( PrintBuffer, BufferSize, ConvertBuiltInFsControlCodeTo( FsControlCode ) );
    }

    const char* ConvertCreateDispositionTo( __in ULONG CreateDisposition )
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

        return "";
    }

    void FormatCreateDisposition( char* PrintBuffer, ULONG BufferSize, ULONG CreateDisposition )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        RtlStringCbCatA( PrintBuffer, BufferSize, ConvertCreateDispositionTo( CreateDisposition ) );
    }

    void FormatCreateDesiredAccess( char* PrintBuffer, ULONG BufferSize, ULONG DesiredAccess )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        if( BooleanFlagOn( DesiredAccess, GENERIC_READ ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "GENERIC_READ|" );
        if( BooleanFlagOn( DesiredAccess, GENERIC_WRITE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "GENERIC_WRITE|" );
        if( BooleanFlagOn( DesiredAccess, GENERIC_EXECUTE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "GENERIC_EXECUTE|" );
        if( BooleanFlagOn( DesiredAccess, GENERIC_ALL ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "GENERIC_ALL|" );

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

    const char* ConvertCreateShareAccessTo( USHORT ShareAccess )
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
        else if( ShareAccess & ( FILE_SHARE_READ | FILE_SHARE_WRITE ) )
        {
            return "FILE_SHARE_READ|FILE_SHARE_WRITE";
        }
        else if( ShareAccess & ( FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE ) )
        {
            return "FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE";
        }
        else if( ShareAccess & ( FILE_SHARE_WRITE | FILE_SHARE_DELETE ) )
        {
            return "FILE_SHARE_WRITE|FILE_SHARE_DELETE";
        }
        else if( ShareAccess & ( FILE_SHARE_READ | FILE_SHARE_DELETE ) )
        {
            return "FILE_SHARE_READ|FILE_SHARE_DELETE";
        }

        if( ShareAccess == 0 )
            return "None";

        return "";
    }

    void FormatCreateShareAccess( char* PrintBuffer, ULONG BufferSize, USHORT ShareAccess )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        RtlStringCbCatA( PrintBuffer, BufferSize, ConvertCreateShareAccessTo( ShareAccess ) );
    }

    void FormatCreateOptions( char* PrintBuffer, ULONG BufferSize, ULONG CreateOptions )
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

    const char* ConvertCreateResultInformationTo( NTSTATUS Status, ULONG_PTR Information )
    {
        if( !NT_SUCCESS( Status ) && Information != FILE_DOES_NOT_EXIST && Information != FILE_EXISTS )
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

        return "";
    }

    const char* ConvertSecurityInformationTo( SECURITY_INFORMATION SecurityInformation )
    {
        switch( SecurityInformation )
        {
            case OWNER_SECURITY_INFORMATION: { return "OWNER_SECURITY_INFORMATION"; } break;
            case GROUP_SECURITY_INFORMATION: { return "GROUP_SECURITY_INFORMATION"; } break;
            case DACL_SECURITY_INFORMATION: { return "DACL_SECURITY_INFORMATION"; } break;
            case SACL_SECURITY_INFORMATION: { return "SACL_SECURITY_INFORMATION"; } break;
            case LABEL_SECURITY_INFORMATION: { return "LABEL_SECURITY_INFORMATION"; } break;

            case PROTECTED_DACL_SECURITY_INFORMATION: { return "PROTECTED_DACL_SECURITY_INFORMATION"; } break;
            case PROTECTED_SACL_SECURITY_INFORMATION: { return "PROTECTED_SACL_SECURITY_INFORMATION"; } break;
            case UNPROTECTED_DACL_SECURITY_INFORMATION: { return "UNPROTECTED_DACL_SECURITY_INFORMATION"; } break;
            case UNPROTECTED_SACL_SECURITY_INFORMATION: { return "UNPROTECTED_SACL_SECURITY_INFORMATION"; } break;
        }
        return "";
    }

    void FormatFileBasicInformation( char* PrintBuffer, ULONG BufferSize, FILE_BASIC_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "CreationTime=%I64d LastAccessTime=%I64d LastWriteTime=%I64d ChangeTime=%I64d FileAttributes=0x%08x"
                            , Info->CreationTime.QuadPart
                            , Info->LastAccessTime.QuadPart
                            , Info->LastWriteTime.QuadPart
                            , Info->ChangeTime.QuadPart
                            , Info->FileAttributes
        );
    }

    void FormatFileStandardInformation( char* PrintBuffer, ULONG BufferSize, FILE_STANDARD_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "AllocationSize=%I64d EndOfFile=%I64d NumberOfLinks=%d DeletePending=%d Directory=%d"
                            , Info->AllocationSize.QuadPart
                            , Info->EndOfFile.QuadPart
                            , Info->NumberOfLinks
                            , Info->DeletePending
                            , Info->Directory
        );
    }

    void FormatFileAccessInformation( char* PrintBuffer, ULONG BufferSize, FILE_ACCESS_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        FormatCreateDesiredAccess( PrintBuffer, BufferSize, Info->AccessFlags );
    }

    void FormatFileRenameInformation( char* PrintBuffer, ULONG BufferSize, __in_z_opt WCHAR* Destination, FILE_RENAME_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "ReplaceIfExists=%d RootDirectory=%p Dst=%ws"
                            , Info->ReplaceIfExists
                            , Info->RootDirectory
                            , Destination == NULLPTR ? Info->FileName : Destination
        );
    }

    void FormatFileDispositionInformation( char* PrintBuffer, ULONG BufferSize, FILE_DISPOSITION_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "DeleteFile=%d"
                            , Info->DeleteFile
        );
    }

    void FormatFilePositionInformation( char* PrintBuffer, ULONG BufferSize, FILE_POSITION_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "CurrentByteOffset=%I64d"
                            , Info->CurrentByteOffset.QuadPart );
    }

    void FormatFileAlignmentInformation( char* PrintBuffer, ULONG BufferSize, FILE_ALIGNMENT_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "%d", Info->AlignmentRequirement );
    }

    void FormatFileAllocationInformation( char* PrintBuffer, ULONG BufferSize, FILE_ALLOCATION_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "AllocationFile=%I64d"
                            , Info->AllocationSize.QuadPart );
    }

    void FormatFileEndOfFileInformation( char* PrintBuffer, ULONG BufferSize, FILE_END_OF_FILE_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "EndOfFile=%I64d"
                            , Info->EndOfFile.QuadPart
        );
    }

    void FormatFileNetworkOpenInformation( char* PrintBuffer, ULONG BufferSize, FILE_NETWORK_OPEN_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "CreationTime=%I64d LastAccessTime=%I64d LastWriteTime=%I64d ChangeTime=%I64d AllocationSize=%I64d EndOfFile=%I64d FileAttributes=0x%08x"
                            , Info->CreationTime.QuadPart
                            , Info->LastAccessTime.QuadPart
                            , Info->LastWriteTime.QuadPart
                            , Info->ChangeTime.QuadPart
                            , Info->AllocationSize.QuadPart
                            , Info->EndOfFile.QuadPart
                            , Info->FileAttributes
        );
    }

    void FormatFileValidDataLengthInformation( char* PrintBuffer, ULONG BufferSize, FILE_VALID_DATA_LENGTH_INFORMATION* Info )
    {
        if( Info == NULLPTR )
            return;

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, "ValidDataLength=%I64d"
                            , Info->ValidDataLength.QuadPart );
    }

    void FormatFileRenameInformationEx( char* PrintBuffer, ULONG BufferSize, __in_z_opt WCHAR* Destination, nsW32API::FILE_RENAME_INFORMATION_EX* Info )
    {
        if( Info == NULLPTR )
            return;

        RtlStringCbPrintfA( PrintBuffer, BufferSize, "Flags=0x%08x,", Info->Flags );
        nsW32API::FormatFileRenameInformationEx( PrintBuffer, BufferSize, Info->Flags );

        auto len = strlen( PrintBuffer );
        RtlStringCbPrintfA( &PrintBuffer[ len ], BufferSize - len, " RootDirectory=%p Dst=%ws\n"
                            , Info->RootDirectory
                            , Destination == NULLPTR ? Info->FileName : Destination
        );
    }

    void FormatFileRenameInformationEx( char* PrintBuffer, ULONG BufferSize, ULONG Flags )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        if( BooleanFlagOn( Flags, FILE_RENAME_REPLACE_IF_EXISTS ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_REPLACE_IF_EXISTS|" );
        if( BooleanFlagOn( Flags, FILE_RENAME_POSIX_SEMANTICS ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_POSIX_SEMANTICS|" );
        if( BooleanFlagOn( Flags, FILE_RENAME_SUPPRESS_PIN_STATE_INHERITANCE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_SUPPRESS_PIN_STATE_INHERITANCE|" );
        if( BooleanFlagOn( Flags, FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE|" );
        if( BooleanFlagOn( Flags, FILE_RENAME_NO_INCREASE_AVAILABLE_SPACE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_NO_INCREASE_AVAILABLE_SPACE|" );
        if( BooleanFlagOn( Flags, FILE_RENAME_NO_DECREASE_AVAILABLE_SPACE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_NO_DECREASE_AVAILABLE_SPACE|" );
        if( BooleanFlagOn( Flags, FILE_RENAME_PRESERVE_AVAILABLE_SPACE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_PRESERVE_AVAILABLE_SPACE|" );
        if( BooleanFlagOn( Flags, FILE_RENAME_IGNORE_READONLY_ATTRIBUTE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_IGNORE_READONLY_ATTRIBUTE|" );
        if( BooleanFlagOn( Flags, FILE_RENAME_FORCE_RESIZE_TARGET_SR ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_FORCE_RESIZE_TARGET_SR|" );
        if( BooleanFlagOn( Flags, FILE_RENAME_FORCE_RESIZE_SOURCE_SR ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_FORCE_RESIZE_SOURCE_SR|" );
        if( BooleanFlagOn( Flags, FILE_RENAME_FORCE_RESIZE_SR ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_RENAME_FORCE_RESIZE_SR|" );
    }

    void FormatFileDispositionInformationEx( char* PrintBuffer, ULONG BufferSize, nsW32API::FILE_DISPOSITION_INFORMATION_EX* Info )
    {
        if( Info == NULLPTR )
            return;

        RtlStringCbPrintfA( PrintBuffer, BufferSize, "Flags=0x%08x,", Info->Flags );
        nsW32API::FormatFileDispositionInformationEx( PrintBuffer, BufferSize, Info->Flags );
    }

    void FormatFileDispositionInformationEx( char* PrintBuffer, ULONG BufferSize, ULONG Flags )
    {
        if( PrintBuffer == NULLPTR || BufferSize == 0 ) return;

        if( Flags == FILE_DISPOSITION_DO_NOT_DELETE )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_DISPOSITION_DO_NOT_DELETE|" );
        if( BooleanFlagOn( Flags, FILE_DISPOSITION_DELETE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_DISPOSITION_DELETE|" );
        if( BooleanFlagOn( Flags, FILE_DISPOSITION_POSIX_SEMANTICS ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_DISPOSITION_POSIX_SEMANTICS|" );
        if( BooleanFlagOn( Flags, FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK|" );
        if( BooleanFlagOn( Flags, FILE_DISPOSITION_ON_CLOSE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_DISPOSITION_ON_CLOSE|" );
        if( BooleanFlagOn( Flags, FILE_DISPOSITION_IGNORE_READONLY_ATTRIBUTE ) )
            RtlStringCbCatA( PrintBuffer, BufferSize, "FILE_DISPOSITION_IGNORE_READONLY_ATTRIBUTE|" );
    }

} // nsW32API
