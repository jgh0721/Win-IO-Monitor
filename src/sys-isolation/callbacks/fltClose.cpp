#include "fltClose.hpp"

#include "privateFCBMgr.hpp"
#include "irpContext.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreClose( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                   PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;
    FCB*                                        Fcb = NULLPTR;
    FILE_OBJECT*                                FileObject = NULLPTR;
    bool                                        IsUninitializeFcb = false;

    __try
    {
        if( IsOwnFileObject( FltObjects->FileObject ) == false )
            __leave;

        if( FLT_IS_FASTIO_OPERATION( Data ) )
        {
            FltStatus = FLT_PREOP_DISALLOW_FASTIO;
            __leave;
        }

        if( FLT_IS_IRP_OPERATION( Data ) == false )
            __leave;

        FileObject = FltObjects->FileObject;
        Fcb = ( FCB* )FileObject->FsContext;
        IrpContext = CreateIrpContext( Data, FltObjects );

        KdPrint( ( "[WinIOSol] EvtID=%09d %s Open=%d Clean=%d Ref=%d Name=%ws\n"
                   , IrpContext->EvtID, __FUNCTION__
                   , Fcb->OpnCount, Fcb->ClnCount, Fcb->RefCount
                   , IrpContext->SrcFileFullPath.Buffer
                   ) );

        AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

        InterlockedDecrement( &Fcb->OpnCount );
        LONG RefCount = InterlockedDecrement( &Fcb->RefCount );

        if( RefCount < 0 )
        {
            // TODO: debug It!
            KdBreakPoint();
            RefCount = 0;
        }

        if( RefCount == 0 )
        {
            Vcb_DeleteFCB( IrpContext->InstanceContext, Fcb );
            IsUninitializeFcb = true;
        }

        if( BooleanFlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) )
        {
            DeallocateCcb( IrpContext->Ccb );
        }

        if( IsUninitializeFcb == true )
        {
            if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_FREE_MAIN_RSRC ) )
            {
                FltReleaseResource( &Fcb->MainResource );
                ClearFlag( IrpContext->CompleteStatus, COMPLETE_FREE_MAIN_RSRC );
            }

            UninitializeFCB( IrpContext );
            DeallocateFcb( IrpContext->Fcb );
        }

        Data->IoStatus.Status = STATUS_SUCCESS;
        Data->IoStatus.Information = 0;

        FltStatus = FLT_PREOP_COMPLETE;
    }
    __finally
    {
        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostClose( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                     PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    return FltStatus;
}
