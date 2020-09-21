#include "fltCleanup.hpp"


#include "fltWrite.hpp"
#include "privateFCBMgr.hpp"
#include "irpContext.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif


FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreCleanup( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                  PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;
    FCB*                                        Fcb = NULLPTR;
    FILE_OBJECT*                                FileObject = NULLPTR;

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

        LONG UncleanCount = InterlockedDecrement( &Fcb->ClnCount );
        if( UncleanCount < 0 )
        {
            // TODO: debug it!!
            KdBreakPoint();
            UncleanCount = 0;
        }

        if( UncleanCount == 0 )
        {
            if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, Fcb->AdvFcbHeader.FileSize.QuadPart );

                KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d StartOffset=%I64d EndOffset=%I64d\n"
                           , IrpContext->EvtID, __FUNCTION__, "CcZeroData", __LINE__
                           , Fcb->AdvFcbHeader.ValidDataLength.QuadPart, Fcb->AdvFcbHeader.FileSize.QuadPart ) );

                FltAcquireResourceExclusive( &Fcb->PagingIoResource );
                Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                FltReleaseResource( &Fcb->PagingIoResource );

                if( CcIsFileCached( FileObject ) )
                {
                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )&Fcb->AdvFcbHeader.AllocationSize );
                }
            }
        }

        CcUninitializeCacheMap( FileObject, NULLPTR, NULLPTR );
        IoRemoveShareAccess( FileObject, &Fcb->LowerShareAccess );

        SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

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

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostCleanup( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                    PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    return FltStatus;
}
