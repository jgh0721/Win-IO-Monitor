﻿#include "fltFlushBuffers.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreFlushBuffers( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
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

        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

        CcFlushCache( &IrpContext->Fcb->SectionObjects, NULL, 0, &Data->IoStatus );

        if( !NT_SUCCESS( Data->IoStatus.Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Status=0x%08x,%s\n"
                       , IrpContext->EvtID, __FUNCTION__, "CcFlushCache FAILED"
                       , Data->IoStatus.Status, ntkernel_error_category::find_ntstatus( Data->IoStatus.Status )->message
                       ) );
        }

        ExAcquireResourceExclusiveLite( IrpContext->Fcb->AdvFcbHeader.PagingIoResource, TRUE );
        ExReleaseResourceLite( IrpContext->Fcb->AdvFcbHeader.PagingIoResource );

        if( IrpContext->Ccb != NULLPTR && IrpContext->Ccb->LowerFileObject != NULLPTR )
            FltFlushBuffers( FltObjects->Instance, IrpContext->Ccb->LowerFileObject );

        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }
    __finally
    {
        if( IrpContext != NULLPTR )
        {
            if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
                FltStatus = IrpContext->PreFltStatus;
        }

        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostFlushBuffers( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                          PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}
