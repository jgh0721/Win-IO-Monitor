#include "fltQueryVolumeInformation.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreQueryVolumeInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                                  PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );

        if( IrpContext != NULLPTR )
            PrintIrpContext( IrpContext );

        AcquireCmnResource( IrpContext, FCB_MAIN_SHARED );
        auto FsInformationClass = (nsW32API::FS_INFORMATION_CLASS)Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass;

        switch( FsInformationClass )
        {
            case FileFsVolumeInformation: {
                ProcessFileFsVolumeInformation( IrpContext );
            } break;
            case FileFsLabelInformation: {
                ProcessFileFsLabelInformation( IrpContext );
            } break;
            case FileFsSizeInformation: {
                ProcessFileFsSizeInformation( IrpContext );
            } break;
            case FileFsDeviceInformation: {
                ProcessFileFsDeviceInformation( IrpContext );
            } break;
            case FileFsAttributeInformation: {
                ProcessFileFsAttributeInformation( IrpContext );
            } break;
            case FileFsControlInformation: {
                ProcessFileFsControlInformation( IrpContext );
            } break;
            case FileFsFullSizeInformation: {
                ProcessFileFsFullSizeInformation( IrpContext );
            } break;
            case FileFsObjectIdInformation: {
                ProcessFileFsObjectIdInformation( IrpContext );
            } break;
            case FileFsDriverPathInformation: {
                ProcessFileFsDriverPathInformation( IrpContext );
            } break;
            case FileFsVolumeFlagsInformation: {
                ProcessFileFsVolumeFlagsInformation( IrpContext );
            } break;
            case nsW32API::FileFsSectorSizeInformation: {
                ProcessFileFsSectorSizeInformation( IrpContext );
            } break;
            case nsW32API::FileFsDataCopyInformation: {
                ProcessFileFsDataCopyInformation( IrpContext );
            } break;
            case nsW32API::FileFsMetadataSizeInformation: {
                ProcessFileFsMetadataSizeInformation( IrpContext );
            } break;
            case nsW32API::FileFsFullSizeInformationEx: {
                ProcessFileFsFullSizeInformationEx( IrpContext );
            } break;
            default:
                break;
        }
        
        FltStatus = FLT_PREOP_COMPLETE;
    }
    __finally
    {
        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostQueryVolumeInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
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

NTSTATUS ProcessFileFsVolumeInformation( IRP_CONTEXT* IrpContext )
{
    auto& IoStatus = IrpContext->Data->IoStatus;
    auto InputBuffer = IrpContext->Data->Iopb->Parameters.QueryVolumeInformation.VolumeBuffer;
    auto Length = IrpContext->Data->Iopb->Parameters.QueryVolumeInformation.Length;
    auto FsInformationClass = IrpContext->Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass;

    __try
    {
        /// 입력값 체크
        if( Length < sizeof( FILE_FS_VOLUME_INFORMATION ) )
        {
            IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus.Information = sizeof( FILE_FS_VOLUME_INFORMATION );
            __leave;
        }

        ULONG LengthReturned = 0;
        auto LowerFileObject = ( ( FCB* )IrpContext->FltObjects->FileObject->FsContext )->LowerFileObject;

        IoStatus.Status = FltQueryVolumeInformationFile( IrpContext->FltObjects->Instance,
                                                         LowerFileObject,
                                                         InputBuffer, Length, FsInformationClass, &LengthReturned );

        IoStatus.Information = LengthReturned;
    }
    __finally
    {
        
    }

    return IoStatus.Status;
}

NTSTATUS ProcessFileFsLabelInformation( IRP_CONTEXT* IrpContext )
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

NTSTATUS ProcessFileFsSizeInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsDeviceInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsAttributeInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsControlInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsFullSizeInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsObjectIdInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsDriverPathInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsVolumeFlagsInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsSectorSizeInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsDataCopyInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsMetadataSizeInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessFileFsFullSizeInformationEx( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    __try
    {

    }
    __finally
    {

    }

    return Status;
}
