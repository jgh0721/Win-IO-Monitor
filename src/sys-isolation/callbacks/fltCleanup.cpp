﻿#include "fltCleanup.hpp"


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

        // File Oplock 점검, 완료될 때 까지 대기한다
        FltCheckOplock( &IrpContext->Fcb->FileOplock, Data, NULL, NULL, NULL );

        // 해당 파일객체에 잡혀있던 Byte-Range Lock 을 모두 해제한다
        FsRtlFastUnlockAll( &Fcb->FileLock, FileObject, FltGetRequestorProcess( Data ), NULL );

        if( IrpContext->Fcb->SectionObjects.DataSectionObject != NULLPTR )
        {
            CcFlushCache( &Fcb->SectionObjects, NULL, 0, NULL );

            ExAcquireResourceExclusiveLite( IrpContext->Fcb->AdvFcbHeader.PagingIoResource, TRUE );
            ExReleaseResourceLite( IrpContext->Fcb->AdvFcbHeader.PagingIoResource );

            CcPurgeCacheSection( &IrpContext->Fcb->SectionObjects, NULL, 0, FALSE );
        }

        CcUninitializeCacheMap( FileObject, NULLPTR, NULLPTR );
        IoRemoveShareAccess( FileObject, &Fcb->LowerShareAccess );

        MmForceSectionClosed( &Fcb->SectionObjects, TRUE );

        if( IrpContext->Ccb->LowerFileHandle != INVALID_HANDLE_VALUE )
        {
            FltClose( IrpContext->Ccb->LowerFileHandle );
            IrpContext->Ccb->LowerFileHandle = INVALID_HANDLE_VALUE;
        }

        if( IrpContext->Ccb->LowerFileObject != NULLPTR )
        {
            ObDereferenceObject( IrpContext->Ccb->LowerFileObject );
            IrpContext->Ccb->LowerFileObject = NULLPTR;
        }

        SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

        // NOTE: 파일이 삭제예약이 걸려있고, 모든 핸들이 닫혔다면 SFO 도 닫아서 파일이 삭제될 수 있도록 한다
        if( BooleanFlagOn( Fcb->Flags, FILE_DELETE_ON_CLOSE ) && 
            UncleanCount == 0 )
        {
            if( Fcb->LowerFileHandle != INVALID_HANDLE_VALUE )
                FltClose( Fcb->LowerFileHandle );
            Fcb->LowerFileHandle = INVALID_HANDLE_VALUE;
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

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostCleanup( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                    PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    return FltStatus;
}
