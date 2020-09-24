#include "fltFileSystemControl.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreFileSystemControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                             PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;
    IRP_CONTEXT*                                IrpContext = NULLPTR;
    PFLT_CALLBACK_DATA                          NewCallbackData = NULLPTR;

    UNREFERENCED_PARAMETER( CompletionContext );

    FsRtlEnterFileSystem();

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );
        PrintIrpContext( IrpContext );

        auto FsControlCode = Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode;

        switch( Data->Iopb->MinorFunction )
        {
            default: {
                NTSTATUS Status = STATUS_SUCCESS;

                Status = FltAllocateCallbackData( FltObjects->Instance, IrpContext->Ccb->LowerFileObject, &NewCallbackData );

                if( !NT_SUCCESS( Status ) )
                {
                    KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Status=0x%08x,%s\n",
                               IrpContext->EvtID, __FUNCTION__
                               , "FltAllocateCallbackData FAILED"
                               , Status, ntkernel_error_category::find_ntstatus( Status )->message 
                               ) );

                    AssignCmnResult( IrpContext, Status );
                    AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
                    __leave;
                }

                RtlCopyMemory( NewCallbackData->Iopb, Data->Iopb, sizeof( FLT_IO_PARAMETER_BLOCK ) );
                NewCallbackData->Iopb->TargetFileObject = IrpContext->Ccb->LowerFileObject;
                FltPerformSynchronousIo( NewCallbackData );

                AssignCmnResult( IrpContext, NewCallbackData->IoStatus.Status );
                AssignCmnResultInfo( IrpContext, NewCallbackData->IoStatus.Information );
                AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            } break;
        }
    }
    __finally
    {
        if( NewCallbackData != NULLPTR )
            FltFreeCallbackData( NewCallbackData );

        if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
            FltStatus = IrpContext->PreFltStatus;

        CloseIrpContext( IrpContext );
    }

    FsRtlExitFileSystem();

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostFileSystemControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                               PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}
