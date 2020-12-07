#include "fltSetVolumeInformation.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreSetVolumeInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
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

        AcquireCmnResource( IrpContext, INST_SHARED );

        auto Fcb = ( FCB* )FltObjects->FileObject->FsContext;
        auto Ccb = ( CCB* )FltObjects->FileObject->FsContext2;

        if( Ccb->LowerFileObject == NULLPTR )
        {
            AssignCmnResult( IrpContext, STATUS_FILE_DELETED );
            __leave;
        }

        PFLT_CALLBACK_DATA RetNewCallbackData = NULLPTR;

        do
        {
            NTSTATUS Status = FltAllocateCallbackData( FltObjects->Instance, Ccb->LowerFileObject, &RetNewCallbackData );

            if( NT_SUCCESS( Status ) )
            {
                RtlCopyMemory( RetNewCallbackData->Iopb, Data->Iopb, sizeof( FLT_IO_PARAMETER_BLOCK ) );

                RetNewCallbackData->Iopb->TargetFileObject = Ccb->LowerFileObject;

                FltPerformSynchronousIo( RetNewCallbackData );

                Status = RetNewCallbackData->IoStatus.Status;

                AssignCmnResult( IrpContext, Status );
            }
        } while( false );

        if( RetNewCallbackData != NULLPTR )
            FltFreeCallbackData( RetNewCallbackData );
        
        FltStatus = FLT_PREOP_COMPLETE;
    }
    __finally
    {
        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostSetVolumeInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                                    PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}
