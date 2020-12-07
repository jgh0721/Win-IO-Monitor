#include "fltQueryInformation.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"
#include "metadata/Metadata.hpp"
#include "pool.hpp"

#include "fltDirectoryControl.hpp"

#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreQueryInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                            PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( IsOwnFileObject( FltObjects->FileObject ) == false )
            __leave;

        if( FLT_IS_FASTIO_OPERATION( Data ) )
        {
            FltStatus = FLT_PREOP_DISALLOW_FASTIO;
            __leave;
        }

        IrpContext = CreateIrpContext( Data, FltObjects );
        PrintIrpContext( IrpContext );

        AcquireCmnResource( IrpContext, FCB_MAIN_SHARED );
        auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;

        switch( FileInformationClass )
        {
            case FileBasicInformation: {
                ProcessFileBasicInformation( IrpContext );
            } break;
            case FileStandardInformation: {
                ProcessFileStandardInformation( IrpContext );
            } break;
            case FileInternalInformation: {
                ProcessFileInternalInformation( IrpContext );
            } break;
            case FileEaInformation: {
                ProcessFileEaInformation( IrpContext );
            } break;
            case FileNameInformation: {
                ProcessFileNameInformation( IrpContext );
            } break;
            case FilePositionInformation: {
                ProcessFilePositionInformation( IrpContext );
            } break;
            case FileAllInformation: {
                ProcessFileAllInformation( IrpContext );
            } break;
            case FileStreamInformation: {
                ProcessFileStreamInformation( IrpContext );
            } break;
            case FileCompressionInformation: {
                ProcessFileCompressionInformation( IrpContext );
            } break;
            // NOTE: FileMoveClusterInformation is reserved for system use
            //case FileMoveClusterInformation: {
            //    ProcessFileMoveClusterInformation( IrpContext );
            //} break;
            case FileNetworkOpenInformation: {
                ProcessFileNetworkOpenInformation( IrpContext );
            } break;
            case FileAttributeTagInformation: {
                ProcessFileAttributeTagInformation( IrpContext );
            } break;
            // NOTE: Windows Vista ~ 
            case FileHardLinkInformation: {
                ProcessFileHardLinkInformation( IrpContext );
            } break;
            // NOTE: Windows 8 ~
            case FileNormalizedNameInformation: {
                ProcessFileNormalizedNameInformation( IrpContext );
            } break;
            // NOTE: Windows 7 ~
            case FileStandardLinkInformation: {
                ProcessFileStandardLinkInformation( IrpContext );
            } break;
            default: {
                FILE_OBJECT* FileObject = FltObjects->FileObject;
                FCB* Fcb = ( FCB* )FileObject->FsContext;
                ULONG ReturnLength = 0;

                auto InputBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
                auto Length = Data->Iopb->Parameters.QueryFileInformation.Length;

                FileObject = IrpContext->Ccb->LowerFileObject != NULLPTR ? IrpContext->Ccb->LowerFileObject : Fcb->LowerFileObject;

                IrpContext->Status = FltQueryInformationFile( FltObjects->Instance, FileObject,
                                                              InputBuffer, Length,
                                                              Data->Iopb->Parameters.QueryFileInformation.FileInformationClass, &ReturnLength );

                AssignCmnResult( IrpContext, IrpContext->Status );
                AssignCmnResultInfo( IrpContext, ReturnLength );

            } break;
        }

        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }
    __finally
    {
        if( IrpContext != NULLPTR )
        {
            if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_IOSTATUS_STATUS ) )
                Data->IoStatus.Status = IrpContext->Status;

            if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_IOSTATUS_INFORMATION ) )
                Data->IoStatus.Information = IrpContext->Information;

            if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
                FltStatus = IrpContext->PreFltStatus;

            PrintIrpContext( IrpContext, true );
        }

        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostQueryInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                              PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS ProcessFileBasicInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    auto InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    auto Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        FILE_BASIC_INFORMATION fbi;
        RtlZeroMemory( &fbi, sizeof( fbi ) );

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance,
                                          IrpContext->Fcb->LowerFileObject,
                                          &fbi,
                                          Length,
                                          FileBasicInformation,
                                          &LengthReturned );

        if( !NT_SUCCESS( Status ) && Status != STATUS_BUFFER_OVERFLOW  )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__ 
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }

        RtlCopyMemory( InputBuffer, &fbi, Length );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

NTSTATUS ProcessFileStandardInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_STANDARD_INFORMATION ) )
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            __leave;
        }

        FILE_STANDARD_INFORMATION FileStdInfo;
        RtlZeroMemory( &FileStdInfo, sizeof( FileStdInfo ) );

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, 
                                          IrpContext->Fcb->LowerFileObject,
                                          &FileStdInfo, sizeof( FileStdInfo ),
                                          FileStandardInformation, &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }

        if( ( IrpContext->ProcessFilter != NULLPTR ) && 
            BooleanFlagOn( IrpContext->Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
        {
            FileStdInfo.EndOfFile = IrpContext->Fcb->AdvFcbHeader.FileSize;
            FileStdInfo.AllocationSize = IrpContext->Fcb->AdvFcbHeader.AllocationSize;
        }

        RtlCopyMemory( InputBuffer, &FileStdInfo, Length );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

NTSTATUS ProcessFileInternalInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_INTERNAL_INFORMATION ) )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            __leave;
        }

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, IrpContext->Fcb->LowerFileObject,
                                          InputBuffer, Length,
                                          FileInternalInformation, &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

NTSTATUS ProcessFileEaInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_EA_INFORMATION ) )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            __leave;
        }

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, IrpContext->Fcb->LowerFileObject,
                                          InputBuffer, Length,
                                          FileEaInformation, &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

NTSTATUS ProcessFileNameInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_NAME_INFORMATION ) )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            __leave;
        }

        auto nameInfo = ( PFILE_NAME_INFORMATION )InputBuffer;
        ULONG nameLength = ( ULONG )( nsUtils::strlength( IrpContext->Fcb->FileFullPathWOVolume ) * sizeof( WCHAR ) );
        ULONG RequiredLength = sizeof( FILE_NAME_INFORMATION ) + nameLength - sizeof( WCHAR );

        if( Length < RequiredLength )
        {
            nameInfo->FileNameLength = RequiredLength;
            RtlCopyMemory( nameInfo->FileName, IrpContext->Fcb->FileFullPathWOVolume, Length - sizeof( ULONG ) );
            Status = STATUS_BUFFER_OVERFLOW;
            Information = Length;
            __leave;
        }

        nameInfo->FileNameLength = nameLength;
        RtlCopyMemory( nameInfo->FileName, IrpContext->Fcb->FileFullPathWOVolume, nameLength );

        if( ( IrpContext->ProcessFilter != NULLPTR ) &&
            BooleanFlagOn( IrpContext->Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
        {
            CorrectFileName( IrpContext, IrpContext->Fcb->MetaDataInfo, nameInfo->FileName, nameInfo->FileNameLength );
        }

        Status = STATUS_SUCCESS;
        Information = nameLength + sizeof( ULONG );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFilePositionInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_POSITION_INFORMATION ) )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            __leave;
        }

        auto FilePositionInfo = ( PFILE_POSITION_INFORMATION )InputBuffer;
        FilePositionInfo->CurrentByteOffset = IrpContext->FltObjects->FileObject->CurrentByteOffset;

        Status = STATUS_SUCCESS;
        // from http://fsfilters.blogspot.com/2012/04/setting-iostatusinformation.html
        Information = sizeof( FilePositionInfo->CurrentByteOffset );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileAllInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;

    auto InputBuffer = (PFILE_ALL_INFORMATION)IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    auto Fcb = IrpContext->Fcb;
    auto Ccb = IrpContext->Ccb;

    ULONG LengthReturned = 0;
    auto RequiredSize = sizeof( FILE_ALL_INFORMATION );
    PFILE_ALL_INFORMATION FileAllInformationBuffer = NULLPTR;
    // NOTE: FILE_ALL_INFORMATION 에는 WCHAR 1개가 이미 할당되어있다. 또한, 커널 드라이버는 NULL 문자를 사용하지 않는다 
    if( Fcb->FileFullPathWOVolume != NULLPTR && nsUtils::strlength( Fcb->FileFullPathWOVolume ) > 1 )
        RequiredSize += ( nsUtils::strlength( Fcb->FileFullPathWOVolume ) * sizeof( WCHAR ) );

    __try
    {
        if( Length == 0 )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            Information = RequiredSize;
            __leave;
        }

        FileAllInformationBuffer = ( PFILE_ALL_INFORMATION )ExAllocatePoolWithTag( PagedPool, RequiredSize, POOL_MAIN_TAG );
        if( FileAllInformationBuffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        // NOTE: IO 관리자가 직접 채운 버퍼를 손상시키지 않기 위해 별도의 버퍼를 할당하여 정보를 획득한다. 
        RtlZeroMemory( FileAllInformationBuffer, RequiredSize );
        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, Ccb->LowerFileObject,
                                          FileAllInformationBuffer, RequiredSize,
                                          FileAllInformation, &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x Device=%ws Name=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__
                       , "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->InstanceContext->DeviceNameBuffer
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );
            __leave;
        }

        // pre-post processing
        // based on http://fsfilters.blogspot.com/2011/11/filters-and-irpmjqueryinformation.html
        // based on FastFAT
        /*!
            typedef struct _FILE_ALL_INFORMATION {
                FILE_BASIC_INFORMATION BasicInformation;
                FILE_STANDARD_INFORMATION StandardInformation;
                FILE_INTERNAL_INFORMATION InternalInformation;
                FILE_EA_INFORMATION EaInformation;
                FILE_ACCESS_INFORMATION AccessInformation;
                FILE_POSITION_INFORMATION PositionInformation;
                FILE_MODE_INFORMATION ModeInformation;
                FILE_ALIGNMENT_INFORMATION AlignmentInformation;
                FILE_NAME_INFORMATION NameInformation;
            } FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;
        */
        ULONG CopiedBytes = 0;

        CopiedBytes += sizeof( FILE_BASIC_INFORMATION );
        if( Length >= CopiedBytes && LengthReturned >= CopiedBytes )
            RtlCopyMemory( &InputBuffer->BasicInformation, &FileAllInformationBuffer->BasicInformation, sizeof( FILE_BASIC_INFORMATION ) );

        CopiedBytes += sizeof( FILE_STANDARD_INFORMATION );
        if( Length >= CopiedBytes && LengthReturned >= CopiedBytes )
        {
            RtlCopyMemory( &InputBuffer->StandardInformation, &FileAllInformationBuffer->StandardInformation, sizeof( FILE_STANDARD_INFORMATION ) );

            if( ( IrpContext->ProcessFilter != NULLPTR ) &&
                BooleanFlagOn( IrpContext->Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
            {
                InputBuffer->StandardInformation.EndOfFile      = IrpContext->Fcb->AdvFcbHeader.FileSize;
                InputBuffer->StandardInformation.AllocationSize = IrpContext->Fcb->AdvFcbHeader.AllocationSize;
            }
        }

        CopiedBytes += sizeof( FILE_INTERNAL_INFORMATION );
        if( Length >= CopiedBytes && LengthReturned >= CopiedBytes )
            RtlCopyMemory( &InputBuffer->InternalInformation, &FileAllInformationBuffer->InternalInformation, sizeof( FILE_INTERNAL_INFORMATION ) );

        CopiedBytes += sizeof( FILE_EA_INFORMATION );
        if( Length >= CopiedBytes && LengthReturned >= CopiedBytes )
            RtlCopyMemory( &InputBuffer->EaInformation, &FileAllInformationBuffer->EaInformation, sizeof( FILE_EA_INFORMATION ) );

        // this information was processed by IO Manager before call my callback
        CopiedBytes += sizeof( FILE_ACCESS_INFORMATION );

        CopiedBytes += sizeof( FILE_POSITION_INFORMATION );
        if( Length >= CopiedBytes && LengthReturned >= CopiedBytes )
            InputBuffer->PositionInformation.CurrentByteOffset = IrpContext->FltObjects->FileObject->CurrentByteOffset;

        // this information was processed by IO Manager before call my callback
        CopiedBytes += sizeof( FILE_MODE_INFORMATION );

        // this information was processed by IO Manager before call my callback
        CopiedBytes += sizeof( FILE_ALIGNMENT_INFORMATION );

        if( Length >= ( CopiedBytes + sizeof( FILE_NAME_INFORMATION) ) )
        {
            CopiedBytes += sizeof( FILE_NAME_INFORMATION ) - sizeof( WCHAR );
            // NOTE: 항상 파일이름을 모두 가져왔을 때의 길이를 넣는다
            InputBuffer->NameInformation.FileNameLength = nsUtils::strlength( Fcb->FileFullPathWOVolume ) * sizeof( WCHAR );

            auto RemainSize = Length - CopiedBytes;
            RtlCopyMemory( &InputBuffer->NameInformation.FileName, &FileAllInformationBuffer->NameInformation.FileName, RemainSize );

            // NOTE: 파일이름이 제대로 모두 복사되었을 때에만 이름을 조정한다
            if( ( IrpContext->ProcessFilter != NULLPTR ) &&
                BooleanFlagOn( IrpContext->Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
            {
                if( RemainSize >= InputBuffer->NameInformation.FileNameLength )
                {
                    auto src = InputBuffer->NameInformation.FileNameLength;

                    CorrectFileName( IrpContext, IrpContext->Fcb->MetaDataInfo,
                                     InputBuffer->NameInformation.FileName, InputBuffer->NameInformation.FileNameLength );

                    LengthReturned -= src - InputBuffer->NameInformation.FileNameLength;
                }
            }
        }

        if( Length >= RequiredSize )
            Status = STATUS_SUCCESS;
        else
            Status = STATUS_BUFFER_OVERFLOW;

        // NOTE: 간혹가다 정신없는 녀석들이 초기 할당 버퍼를 터무니없이 크게 요청하는 경우가 있다( 예), 버퍼크기 65536 등 )
        // NOTE: 이곳에서는 실제 반환되는 크기를 정확히 전달해야한다
        if( Length > LengthReturned )
            Information = LengthReturned;
        else
            Information = Length;
    }
    __finally
    {
        if( FileAllInformationBuffer != NULLPTR )
            ExFreePoolWithTag( FileAllInformationBuffer, POOL_MAIN_TAG );

        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileStreamInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_STREAM_INFORMATION ) )
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            __leave;
        }

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance,
                                          IrpContext->Fcb->LowerFileObject,
                                          InputBuffer, Length,
                                          FileStreamInformation, 
                                          &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }

        ULONG Offset = 0;
        auto InfoBuffer = ( PFILE_STREAM_INFORMATION )InputBuffer;
        static WCHAR NTFS_DEFAULT_STREAM[] = L"::$DATA";
        static WCHAR NTFS_ALTERNATE_STREAM[] = L":$DATA";

        /*!
            From MSDN

            If a file system supports stream enumeration, but the file has no streams other than the default data stream, which is unnamed, the file system should return a single FILE_STREAM_INFORMATION structure containing either "::$DATA" or a zero-length Unicode string as the StreamName.

            NTFS returns "::$DATA" as the StreamName for the default data stream.

            For a named data stream, NTFS appends ":$DATA" to the stream name. For example, for a user data stream with the name "Authors," NTFS returns ":Authors:$DATA" as the StreamName.
        */
        do
        {
            Offset = InfoBuffer->NextEntryOffset;

            if( (InfoBuffer->StreamNameLength == 0) || 
                ( (InfoBuffer->StreamNameLength == (_countof( NTFS_DEFAULT_STREAM ) * sizeof(WCHAR) - sizeof(WCHAR) )) && 
                  (RtlCompareMemory( InfoBuffer->StreamName, NTFS_DEFAULT_STREAM, InfoBuffer->StreamNameLength ) == InfoBuffer->StreamNameLength ) ) )
            {
                if( ( IrpContext->ProcessFilter != NULLPTR ) &&
                    BooleanFlagOn( IrpContext->Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
                {
                    InfoBuffer->StreamSize.QuadPart = IrpContext->Fcb->AdvFcbHeader.FileSize.QuadPart;
                    InfoBuffer->StreamAllocationSize.QuadPart = IrpContext->Fcb->AdvFcbHeader.AllocationSize.QuadPart;
                }
            }
            else
            {
                // 추가 스트림 목록
                if( InfoBuffer->StreamNameLength >= (_countof( NTFS_ALTERNATE_STREAM ) * sizeof(WCHAR) - sizeof(WCHAR) ) )
                {
                    if( nsUtils::EndsWithW( InfoBuffer->StreamName, NTFS_ALTERNATE_STREAM ) != NULLPTR )
                    {
                        // TODO: 파일 스트림 이름을 이용하여 실제 파일을 열고, 헤더 정보 및 크기를 획득한다
                        // TODO: 만약 현재 열려진 파일이라면 FCB 를 검색한다
                        // TODO: 그 후 위와 같이 StreamSize 및 StreamAllocationSize 를 수정한다
                    }
                }
            }

            InfoBuffer = ( PFILE_STREAM_INFORMATION )Add2Ptr( InfoBuffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

NTSTATUS ProcessFileCompressionInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_COMPRESSION_INFORMATION ) )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            __leave;
        }

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, IrpContext->Fcb->LowerFileObject,
                                          InputBuffer, Length,
                                          FileCompressionInformation, &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

NTSTATUS ProcessFileNetworkOpenInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_NETWORK_OPEN_INFORMATION ) )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            __leave;
        }

        FILE_NETWORK_OPEN_INFORMATION networkOpenInfo;

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, IrpContext->Fcb->LowerFileObject,
                                          &networkOpenInfo, sizeof( FILE_NETWORK_OPEN_INFORMATION ),
                                          FileNetworkOpenInformation, &LengthReturned );

        if( !NT_SUCCESS( Status ) && Status != STATUS_BUFFER_OVERFLOW )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }

        if( ( IrpContext->ProcessFilter != NULLPTR ) &&
            BooleanFlagOn( IrpContext->Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
        {
            networkOpenInfo.EndOfFile = IrpContext->Fcb->AdvFcbHeader.FileSize;
            networkOpenInfo.AllocationSize = IrpContext->Fcb->AdvFcbHeader.AllocationSize;
        }

        RtlCopyMemory( InputBuffer, &networkOpenInfo, Length );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

NTSTATUS ProcessFileAttributeTagInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_ATTRIBUTE_TAG_INFORMATION ) )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            __leave;
        }

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, IrpContext->Fcb->LowerFileObject,
                                          InputBuffer, Length,
                                          FileAttributeTagInformation, &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

NTSTATUS ProcessFileHardLinkInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_LINKS_INFORMATION ) )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            __leave;
        }

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, IrpContext->Fcb->LowerFileObject,
                                          InputBuffer, Length,
                                          FileHardLinkInformation, &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }

        // TODO: 향후 반환된 구조체를 조사하여 파일이름을 변경해서 전달해야한다( 하드링크 지원 )
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

NTSTATUS ProcessFileNormalizedNameInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_NAME_INFORMATION ) )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            __leave;
        }

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, IrpContext->Fcb->LowerFileObject,
                                          InputBuffer, Length,
                                          FileNormalizedNameInformation, &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }

        if( ( IrpContext->ProcessFilter != NULLPTR ) &&
            BooleanFlagOn( IrpContext->Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
        {
            auto nameInfo = ( PFILE_NAME_INFORMATION )InputBuffer;

            CorrectFileName( IrpContext, IrpContext->Fcb->MetaDataInfo, nameInfo->FileName, nameInfo->FileNameLength );
        }
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

NTSTATUS ProcessFileStandardLinkInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_STANDARD_LINK_INFORMATION ) )
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            __leave;
        }

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, IrpContext->Fcb->LowerFileObject,
                                          InputBuffer, Length,
                                          FileStandardLinkInformation, &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthReturned );
    }

    return Status;
}

