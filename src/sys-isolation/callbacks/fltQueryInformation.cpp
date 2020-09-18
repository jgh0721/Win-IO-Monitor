#include "fltQueryInformation.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

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
        auto InputBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
        auto Length = Data->Iopb->Parameters.QueryFileInformation.Length;

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
            case FileAttributeTagInformation: {
                ProcessFileAttributeTagInformation( IrpContext );
            } break;
            case FileCompressionInformation: {
                ProcessFileCompressionInformation( IrpContext );
            } break;
            case FileMoveClusterInformation: {
                ProcessFileMoveClusterInformation( IrpContext );
            } break;
            case FileNetworkOpenInformation: {
                ProcessFileNetworkOpenInformation( IrpContext );
            } break;
            case FileStreamInformation: {
                ProcessFileStreamInformation( IrpContext );
            } break;
            case FileHardLinkInformation: {
                ProcessFileHardLinkInformation( IrpContext );
            } break;
            default: {
                FILE_OBJECT* FileObject = FltObjects->FileObject;
                FCB* Fcb = ( FCB* )FileObject->FsContext;
                ULONG ReturnLength = 0;

                Data->IoStatus.Status = FltQueryInformationFile( FltObjects->Instance, Fcb->LowerFileObject,
                                                                 InputBuffer, Length,
                                                                 Data->Iopb->Parameters.QueryFileInformation.FileInformationClass, &ReturnLength );

                Data->IoStatus.Information = ReturnLength;

            } break;
        }

        FltStatus = FLT_PREOP_COMPLETE;
    }
    __finally
    {
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
    auto& IoStatus = IrpContext->Data->IoStatus;

    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;
    auto InputBuffer = IrpContext->Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    auto Length = IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length;

    __try
    {
        if( Length < sizeof( FILE_BASIC_INFORMATION ) )
        {
            IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            __leave;
        }

        FILE_BASIC_INFORMATION fbi;
        ULONG LengthReturned = 0;
        
        RtlZeroMemory( &fbi, sizeof( fbi ) );
        IoStatus.Status = FltQueryInformationFile( IrpContext->FltObjects->Instance,
                                                   IrpContext->Fcb->LowerFileObject,
                                                   &fbi,
                                                   sizeof( FILE_BASIC_INFORMATION ),
                                                   FileBasicInformation,
                                                   &LengthReturned );

        if( !NT_SUCCESS( IoStatus.Status ) )
        {
            KdPrint( ( "[iMonFSD] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n",
                       IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED", __LINE__ 
                       , IoStatus.Status, ntkernel_error_category::find_ntstatus( IoStatus.Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );

            IoStatus.Information = 0;
            __leave;
        }

        RtlCopyMemory( InputBuffer, &fbi, LengthReturned );
        IoStatus.Information = LengthReturned;
    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileAllInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileAttributeTagInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileCompressionInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileEaInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileInternalInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileMoveClusterInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileNameInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileNetworkOpenInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFilePositionInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileStandardInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileStreamInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileHardLinkInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;

    __try
    {

    }
    __finally
    {

    }

    return IoStatus.Status;
}

