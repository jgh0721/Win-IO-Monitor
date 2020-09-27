#include "fltQueryInformation.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"
#include "pool.hpp"

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
        if( IrpContext != NULLPTR )
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
                if( NT_SUCCESS( IrpContext->Status ) )
                {
                    AssignCmnResultInfo( IrpContext, ReturnLength );
                }

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
    ULONG_PTR Information = 0;
    ULONG LengthReturned = 0;

    auto InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    auto Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_BASIC_INFORMATION ) )
        {
            AssignCmnResult( IrpContext, STATUS_BUFFER_TOO_SMALL );
            __leave;
        }

        FILE_BASIC_INFORMATION fbi;

        RtlZeroMemory( &fbi, sizeof( fbi ) );
        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance,
                                          IrpContext->Fcb->LowerFileObject,
                                          &fbi,
                                          sizeof( FILE_BASIC_INFORMATION ),
                                          FileBasicInformation,
                                          &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__ 
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }

        RtlCopyMemory( InputBuffer, &fbi, LengthReturned );
        Information = LengthReturned;
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileStandardInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
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

        RtlCopyMemory( InputBuffer, &FileStdInfo, sizeof( FILE_STANDARD_INFORMATION ) );
        Information = LengthReturned;
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileInternalInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
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

        Information = LengthReturned;
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileEaInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
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

        Information = LengthReturned;
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
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
    ULONG LengthReturned = 0;

    PVOID InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;
    PFILE_ALL_INFORMATION FileAllInformationBuffer = NULLPTR;

    __try
    {
        if( Length < sizeof( FILE_ALL_INFORMATION ) )
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            __leave;
        }

        auto Fcb = IrpContext->Fcb;
        SIZE_T poolSize = sizeof( FILE_ALL_INFORMATION ) + ( nsUtils::strlength( Fcb->FileFullPath.Buffer ) * sizeof( WCHAR ) );
        FileAllInformationBuffer = ( PFILE_ALL_INFORMATION )ExAllocatePoolWithTag( PagedPool, poolSize, POOL_MAIN_TAG );

        if( FileAllInformationBuffer == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory( FileAllInformationBuffer, poolSize );
        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, Fcb->LowerFileObject,
                                          FileAllInformationBuffer, poolSize,
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
        
        ULONG RequiredLength = sizeof( FILE_ALL_INFORMATION ) + FileAllInformationBuffer->NameInformation.FileNameLength - sizeof( WCHAR );

        if( Length < RequiredLength )
        {
            RtlCopyMemory( InputBuffer, FileAllInformationBuffer, Length );
            Status = STATUS_BUFFER_OVERFLOW;
            Information = Length;
            __leave;
        }

        Status = STATUS_SUCCESS;
        Information = RequiredLength;
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
    ULONG_PTR Information = 0;
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
                                          FileStreamInformation, &LengthReturned );

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
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileCompressionInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
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

        Information = LengthReturned;
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileNetworkOpenInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
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

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            __leave;
        }

        RtlCopyMemory( InputBuffer, &networkOpenInfo, Length );
        Information = LengthReturned;
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileAttributeTagInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
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

        Information = LengthReturned;
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileHardLinkInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
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

        Information = LengthReturned;
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileNormalizedNameInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
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

        Information = LengthReturned;
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS ProcessFileStandardLinkInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
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

        Information = LengthReturned;
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

