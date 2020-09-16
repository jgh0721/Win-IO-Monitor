#include "fltSetInformation.hpp"

#include "privateFCBMgr.hpp"
#include "irpContext.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                          PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;

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
        FCB* Fcb = (FCB*)FileObject->FsContext;
        IrpContext = CreateIrpContext( Data, FltObjects );
        const auto& FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;

        KdPrint( ( "[WinIOSol] EvtID=%09d %s CLASS=%-45s Name=%ws\n",
                   IrpContext->EvtID, __FUNCTION__
                   , nsW32API::ConvertFileInformationClassTo( FileInformationClass )
                   , IrpContext->SrcFileFullPath.Buffer
                   ) );

        Data->IoStatus.Status = FltSetInformationFile( FltObjects->Instance, Fcb->LowerFileObject, Data->Iopb->Parameters.SetFileInformation.InfoBuffer,
                                                       Data->Iopb->Parameters.SetFileInformation.Length,
                                                       Data->Iopb->Parameters.SetFileInformation.FileInformationClass );

        FltStatus = FLT_PREOP_COMPLETE;
    }
    __finally
    {
        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                            PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    return FltStatus;
}
