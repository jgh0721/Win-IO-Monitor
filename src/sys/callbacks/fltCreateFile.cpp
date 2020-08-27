#include "fltCreateFile.hpp"

#include "irpContext.hpp"
#include "WinIOMonitor_W32API.hpp"
#include "utilities/contextMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS WinIOPreCreate( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                          PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS       FltStatus       = FLT_PREOP_SUCCESS_NO_CALLBACK;
    auto                            IrpContext      = CreateIrpContext( Data, FltObjects );
    CTX_INSTANCE_CONTEXT*           InstanceContext = NULLPTR;

    __try
    {
        CtxGetContext( FltObjects, NULLPTR, FLT_INSTANCE_CONTEXT, ( PFLT_CONTEXT* )&InstanceContext );

        if( IrpContext != NULLPTR )
        {
            PrintIrpContext( IrpContext );
            *CompletionContext = IrpContext;
            FltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
        }
    }
    __finally
    {
        CtxReleaseContext( InstanceContext );
    }
    
    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS WinIOPostCreate( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                            PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    auto IrpContext = ( PIRP_CONTEXT )CompletionContext;

    //if( NT_SUCCESS( Data->IoStatus.Status ) )
    //{
    //    
    //}

    CtxReleaseContext( IrpContext->StreamContext );

    CloseIrpContext( IrpContext );

    return FLT_POSTOP_FINISHED_PROCESSING;
}
