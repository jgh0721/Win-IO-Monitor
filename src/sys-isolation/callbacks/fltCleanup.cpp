#include "fltCleanup.hpp"

#include "fltWrite.hpp"
#include "privateFCBMgr.hpp"
#include "irpContext.hpp"
#include "metadata/Metadata.hpp"

#include "utilities/osInfoMgr.hpp"
#include "utilities/fltUtilities.hpp"
#include "utilities/contextMgr.hpp"
#include "utilities/detectDelete.hpp"

#include "communication/Communication.hpp"

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

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( FLT_IS_FASTIO_OPERATION( Data ) )
        {
            FltStatus = FLT_PREOP_DISALLOW_FASTIO;
            __leave;
        }

        if( FLT_IS_IRP_OPERATION( Data ) == false )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );

        if( IrpContext->IsOwnObject == false )
        {
            if( IrpContext->StreamContext != NULLPTR )
            {
                *CompletionContext = IrpContext;
                AssignCmnFltResult( IrpContext, FLT_PREOP_SYNCHRONIZE );
                __leave;
            }

            __leave;
        }

        FileObject = FltObjects->FileObject;
        Fcb = ( FCB* )FileObject->FsContext;

        if( BooleanFlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) )
        {
            // NOTE: CcFlushCache 는 동기적으로 호출되어 IRP_MJ_WRITE 를 생성하고 대기한다.
            // IRP_MJ_WRITE 는 Fcb 의 MainResource 를 배타적으로 획득한다
            if( BooleanFlagOn( IrpContext->Fcb->Flags, FCB_STATE_FILE_MODIFIED ) )
            {
                if( IrpContext->Fcb->SectionObjects.DataSectionObject != NULLPTR )
                {
                    CcFlushCache( &Fcb->SectionObjects, NULL, 0, NULL );

                    FsRtlEnterFileSystem();
                    ExAcquireResourceExclusiveLite( IrpContext->Fcb->AdvFcbHeader.PagingIoResource, TRUE );
                    ExReleaseResourceLite( IrpContext->Fcb->AdvFcbHeader.PagingIoResource );
                    FsRtlExitFileSystem();
                }
            }
        }

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

                FsRtlEnterFileSystem();
                FltAcquireResourceExclusive( &Fcb->PagingIoResource );
                Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                FltReleaseResource( &Fcb->PagingIoResource );
                FsRtlExitFileSystem();

                if( CcIsFileCached( FileObject ) )
                {
                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )&Fcb->AdvFcbHeader.AllocationSize );
                }
            }

            CheckEventFileCleanup( IrpContext );
        }

        // File Oplock 점검, 완료될 때 까지 대기한다
        FltCheckOplock( &IrpContext->Fcb->FileOplock, Data, NULL, NULL, NULL );

        // 해당 파일객체에 잡혀있던 Byte-Range Lock 을 모두 해제한다
        FsRtlFastUnlockAll( &Fcb->FileLock, FileObject, FltGetRequestorProcess( Data ), NULL );

        Fcb->AdvFcbHeader.IsFastIoPossible = (UCHAR)CheckIsFastIOPossible( IrpContext->Fcb );

        if( nsUtils::VerifyVersionInfoEx( 6, ">=" ) == true )
        {
            ULONG WritableRefs = GlobalNtOsKrnlMgr.MmDoesFileHaveUserWritableReferences( &Fcb->SectionObjects );

            if( FileObject->WriteAccess != FALSE )
            {
                if( Fcb->SectionObjects.DataSectionObject != NULLPTR )
                    GlobalNtOsKrnlMgr.FsRtlChangeBackingFileObject( NULLPTR, FileObject, nsW32API::ChangeDataControlArea, 0 );
                if( Fcb->SectionObjects.ImageSectionObject != NULLPTR )
                    GlobalNtOsKrnlMgr.FsRtlChangeBackingFileObject( NULLPTR, FileObject, nsW32API::ChangeImageControlArea, 0 );
                if( Fcb->SectionObjects.SharedCacheMap != NULLPTR )
                    GlobalNtOsKrnlMgr.FsRtlChangeBackingFileObject( NULLPTR, FileObject, nsW32API::ChangeSharedCacheMap, 0 );
            }
        }

        if( IrpContext->Fcb->SectionObjects.DataSectionObject != NULLPTR )
        {
            CcFlushCache( &Fcb->SectionObjects, NULL, 0, NULL );

            FsRtlEnterFileSystem();
            ExAcquireResourceExclusiveLite( IrpContext->Fcb->AdvFcbHeader.PagingIoResource, TRUE );
            ExReleaseResourceLite( IrpContext->Fcb->AdvFcbHeader.PagingIoResource );
            FsRtlExitFileSystem();

            CcPurgeCacheSection( &IrpContext->Fcb->SectionObjects, NULL, 0, FALSE );
        }

        auto TruncateSize = Fcb->AdvFcbHeader.FileSize;
        CcUninitializeCacheMap( FileObject, &TruncateSize, NULLPTR );
        IoRemoveShareAccess( FileObject, &Fcb->LowerShareAccess );

        // TODO: CCB 의 Flags 를 추가로 점검하도록 한다
        if( BooleanFlagOn( Fcb->Flags, FCB_STATE_METADATA_ASSOC ) &&
            BooleanFlagOn( IrpContext->Ccb->Flags, CCB_STATE_SIZE_CHAGNED ) )
        {
            WriteMetaData( IrpContext, IrpContext->Fcb->LowerFileObject, IrpContext->Fcb->MetaDataInfo );

            SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
        }

        if( !BooleanFlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) )
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
        if( BooleanFlagOn( Fcb->Flags, FCB_STATE_DELETE_ON_CLOSE ) &&
            UncleanCount == 0 )
        {
            if( Fcb->LowerFileHandle != INVALID_HANDLE_VALUE )
                FltClose( Fcb->LowerFileHandle );
            Fcb->LowerFileHandle = INVALID_HANDLE_VALUE;

            if( Fcb->LowerFileObject != NULLPTR )
                ObDereferenceObject( Fcb->LowerFileObject );
            Fcb->LowerFileObject = NULLPTR;
        }

        Data->IoStatus.Status = STATUS_SUCCESS;
        Data->IoStatus.Information = 0;

        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }
    __finally
    {
        if( IrpContext != NULLPTR )
        {
            if( FlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
                FltStatus = IrpContext->PreFltStatus;

            if( IrpContext->IsOwnObject == true )
            {
                PrintIrpContext( IrpContext, true );
            }

            if( ( IrpContext->IsOwnObject == true ) || 
                ( IrpContext->IsOwnObject == false && IrpContext->StreamContext == NULLPTR ) )
                CloseIrpContext( IrpContext );
        }
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostCleanup( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                    PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    IRP_CONTEXT*                                IrpContext = (IRP_CONTEXT*)CompletionContext;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    /*!
        해당 핸들러는 격리필터의 관리를 받지 않는 일반 파일을 위해 호출된다.
        격리필터의 관리를 받는 모든 파일은 Pre 핸들러에서 처리가 완료된다 
    */

    do
    {
        if( !NT_SUCCESS( Data->IoStatus.Status ) )
            break;

        //  From MS Delete Sample
        //
        //  Determine whether or not we should check for deletion. What
        //  flags a file as a deletion candidate is one or more of the following:
        //
        //  1. NumOps > 0. This means there are or were racing changes to
        //  the file delete disposition state, and, in that case,
        //  we don't know what that state is. So, let's err to the side of
        //  caution and check if it was deleted.
        //
        //  2. SetDisp. If this is TRUE and we haven't raced in setting delete
        //  disposition, this reflects the true delete disposition state of the
        //  file, meaning we must check for deletes if it is set to TRUE.
        //
        //  3. DeleteOnClose. If the file was ever opened with
        //  FILE_DELETE_ON_CLOSE, we must check to see if it was deleted.
        //  FileDispositionInformationEx allows the this flag to be unset.
        //
        //  Also, if a deletion of this stream was already notified, there is no
        //  point notifying it again.
        //

        if( ( ( IrpContext->StreamContext->NumOps > 0 ) ||
              ( IrpContext->StreamContext->SetDisp ) ||
              ( IrpContext->StreamContext->DeleteOnClose ) ) &&
            ( 0 == IrpContext->StreamContext->IsNotified ) )
        {
            NTSTATUS Status = STATUS_SUCCESS;
            FILE_STANDARD_INFORMATION fileInfo;

            //
            //  The check for deletion is done via a query to
            //  FileStandardInformation. If that returns STATUS_FILE_DELETED
            //  it means the stream was deleted.
            //

            Status = FltQueryInformationFile( Data->Iopb->TargetInstance,
                                              Data->Iopb->TargetFileObject,
                                              &fileInfo,
                                              sizeof( fileInfo ),
                                              FileStandardInformation,
                                              NULL );

            if( Status == STATUS_FILE_DELETED )
            {

                Status = SolProcessDelete( IrpContext );
            }
        }

    } while( false );

    CloseIrpContext( IrpContext );
    return FltStatus;
}
