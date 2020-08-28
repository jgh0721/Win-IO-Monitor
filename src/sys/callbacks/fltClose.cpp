#include "fltClose.hpp"

#include "irpContext.hpp"
#include "utilities/contextMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

FLT_PREOP_CALLBACK_STATUS FLTAPI WinIOPreClose( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                IrpContext = CreateIrpContext( Data, FltObjects );

    do
    {
        if( IrpContext != NULLPTR )
        {
            KeEnterCriticalRegion();
            ExAcquireResourceExclusiveLite( IrpContext->StreamContext->Resource, TRUE );

            IrpContext->StreamContext->CloseCount++;

            ExReleaseResourceLite( IrpContext->StreamContext->Resource );
            KeLeaveCriticalRegion();

            PrintIrpContext( IrpContext );

            if( IrpContext->StreamContext )
                CtxReleaseContext( IrpContext->StreamContext );
            IrpContext->StreamContext = NULLPTR;

            FltStatus = FLT_PREOP_SYNCHRONIZE;
            *CompletionContext = IrpContext;
        }
        
    } while( false );

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI WinIOPostClose( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    auto                        IrpContext = ( IRP_CONTEXT* )CompletionContext;

    do
    {
        
    } while( false );

    CloseIrpContext( IrpContext );

    return FltStatus;
}
