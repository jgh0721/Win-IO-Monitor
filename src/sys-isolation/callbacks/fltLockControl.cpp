#include "fltLockControl.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreLockControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                       PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;
    IRP_CONTEXT*                                IrpContext = NULLPTR;

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        if( FLT_IS_FASTIO_OPERATION( Data ) != FALSE )
            return FLT_PREOP_DISALLOW_FASTIO;

        IrpContext = CreateIrpContext( Data, FltObjects );
        PrintIrpContext( IrpContext );

        AcquireCmnResource( IrpContext, FCB_MAIN_SHARED );

        auto Ret = FltCheckOplock( &IrpContext->Fcb->FileOplock, Data, NULL, NULL, NULL );
        // FltCheckOplock 은 수행에 실패하면 FLT_PREOP_COMPLETE 를 반환한다
        if( Ret == FLT_PREOP_COMPLETE )
        {
            AssignCmnResult( IrpContext, Data->IoStatus.Status );
            AssignCmnResultInfo( IrpContext, Data->IoStatus.Information );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            __leave;
        }

        Ret = FltProcessFileLock( &IrpContext->Fcb->FileLock, Data, NULL );

        AssignCmnResult( IrpContext, Data->IoStatus.Status );
        AssignCmnResultInfo( IrpContext, Data->IoStatus.Information );
        AssignCmnFltResult( IrpContext, Ret != FLT_PREOP_PENDING ? FLT_PREOP_COMPLETE : Ret );
    }
    __finally
    {
        if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
            FltStatus = IrpContext->PreFltStatus;

        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostLockControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                         PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}
