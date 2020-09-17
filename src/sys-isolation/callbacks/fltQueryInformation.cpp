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

        FILE_OBJECT* FileObject = FltObjects->FileObject;
        FCB* Fcb = ( FCB* )FileObject->FsContext;
        IrpContext = CreateIrpContext( Data, FltObjects );
        if( IrpContext != NULLPTR )
            PrintIrpContext( IrpContext );

        auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;
        auto InputBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
        auto Length = Data->Iopb->Parameters.QueryFileInformation.Length;

        switch( FileInformationClass )
        {
            case FileAllInformation: {
                ProcessFileAllInformation( IrpContext );
            } break;
            case FileAttributeTagInformation: {
                ProcessFileAttributeTagInformation( IrpContext );
            } break;
            case FileBasicInformation: {
                ProcessFileBasicInformation( IrpContext );
            } break;
            case FileCompressionInformation: {
                ProcessFileCompressionInformation( IrpContext );
            } break;
            case FileEaInformation: {
                ProcessFileEaInformation( IrpContext );
            } break;
            case FileInternalInformation: {
                ProcessFileInternalInformation( IrpContext );
            } break;
            case FileMoveClusterInformation: {
                ProcessFileMoveClusterInformation( IrpContext );
            } break;
            case FileNameInformation: {
                ProcessFileNameInformation( IrpContext );
            } break;
            case FileNetworkOpenInformation: {
                ProcessFileNetworkOpenInformation( IrpContext );
            } break;
            case FilePositionInformation: {
                ProcessFilePositionInformation( IrpContext );
            } break;
            case FileStandardInformation: {
                ProcessFileStandardInformation( IrpContext );
            } break;
            case FileStreamInformation: {
                ProcessFileStreamInformation( IrpContext );
            } break;
            case FileHardLinkInformation: {
                ProcessFileHardLinkInformation( IrpContext );
            } break;
            default: {
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

NTSTATUS ProcessFileBasicInformation( IRP_CONTEXT* IrpContext )
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

