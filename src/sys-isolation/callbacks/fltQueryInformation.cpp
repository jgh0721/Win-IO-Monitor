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

        PVOID InputBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
        ULONG Length = Data->Iopb->Parameters.QueryFileInformation.Length;
        ULONG ReturnLength = 0;

        Data->IoStatus.Status = FltQueryInformationFile( FltObjects->Instance, Fcb->LowerFileObject,
                                                         InputBuffer, Length,
                                                         Data->Iopb->Parameters.QueryFileInformation.FileInformationClass, &ReturnLength );

        Data->IoStatus.Information = ReturnLength;
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

    return FltStatus;
}
