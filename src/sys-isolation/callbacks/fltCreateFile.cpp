#include "fltCreateFile.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#include "fltCmnLibs.hpp"
#include "utilities/osInfoMgr.hpp"
#include "utilities/bufferMgr.hpp"
#include "utilities/fltUtilities.hpp"

#include "W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

/*!
    격리필터에 의해 Lower FileObject 를 획득하면, 아직 초기화가 끝나지 않은 전달받은 Upper FileObject 에 값을 채워넣어야한다.

    FsContext
    FsContext2
    SectionObjectPointer
    PrivateCacheMap
    Vpb
    ReadAccess
    WriteAccess
    DeleteAccess
    SharedRead
    SharedWrite
    SharedDelete
    Flags
*/

/*!
    관련 파일 객체(Related FileObject) :
        일반적으로 FileObject 가 위치한 디렉토리를 가르키지만, 해당 파일시스템이 ADS(Alternate File Stream) 를 지원한다면
        관련 파일 객체는 FileObject(ADS) 의 주 파일스트림에 대한 파일객체가 될 수 있다
*/

static LARGE_INTEGER ZERO = { 0, 0 };

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreCreate( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                  PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;
    CREATE_ARGS                                 Args;
    auto&                                       IoStatus = Data->IoStatus;

    __try
    {
        RtlZeroMemory( &Args, sizeof( CREATE_ARGS ) );

        Args.LowerFileHandle = INVALID_HANDLE_VALUE;
        Args.LowerFileObject = NULLPTR;

        ///////////////////////////////////////////////////////////////////////
        /// Validate Input

        if( BooleanFlagOn( Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE ) )
            __leave;

        auto SecurityContext = Data->Iopb->Parameters.Create.SecurityContext;

        Args.FileObject = Data->Iopb->TargetFileObject;
        Args.CreateOptions = Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;
        Args.CreateDisposition = ( Data->Iopb->Parameters.Create.Options >> 24 ) & 0x000000ff;
        Args.CreateDesiredAccess = SecurityContext->DesiredAccess;
        Args.DeleteOnClose = FlagOn( Args.CreateOptions, FILE_DELETE_ON_CLOSE ) > 0;
        Args.RequiringOplock = FlagOn( Args.CreateOptions, FILE_OPEN_REQUIRING_OPLOCK ) > 0;

        if( BooleanFlagOn( Args.CreateOptions, FILE_DIRECTORY_FILE ) )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );
        if( IrpContext == NULLPTR )
            __leave;

        if( IrpContext->SrcFileFullPath.Buffer == NULLPTR )
            __leave;

        if( nsUtils::WildcardMatch_straight( IrpContext->SrcFileFullPathWOVolume, L"*isolationtest.txt*" ) == false )
            __leave;

        IrpContext->IsAudit = true;
        IrpContext->Params = &Args;

        InitializeVolumeProperties( IrpContext );

        Args.CreateFileName = AllocateBuffer<WCHAR>( BUFFER_FILENAME, IrpContext->SrcFileFullPath.BufferSize + _countof( IrpContext->InstanceContext->DeviceNameBuffer ) );

        RtlStringCbCatW( Args.CreateFileName.Buffer, Args.CreateFileName.BufferSize, IrpContext->InstanceContext->DeviceNameBuffer );
        RtlStringCbCatW( Args.CreateFileName.Buffer, Args.CreateFileName.BufferSize, IrpContext->SrcFileFullPathWOVolume );
        RtlInitUnicodeString( &Args.CreateFileNameUS, Args.CreateFileName.Buffer );

        InitializeObjectAttributes( &Args.CreateObjectAttributes, &Args.CreateFileNameUS, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );

        PrintIrpContext( IrpContext );

        ClearFlag( Args.CreateOptions, FILE_OPEN_REQUIRING_OPLOCK );

        AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
        IrpContext->Fcb = Vcb_SearchFCB( IrpContext->InstanceContext, IrpContext->SrcFileFullPathWOVolume );

        if( IsOwnFileObject( Args.FileObject ) == false && IrpContext->Fcb == NULLPTR )
        {
            FltReleaseResource( &IrpContext->InstanceContext->VcbLock );
            ClearFlag( IrpContext->CompleteStatus, COMPLETE_FREE_INST_RSRC );

            CreateFileNonExistFCB( IrpContext );
        }
        else
        {
            CreateFileExistFCB( IrpContext );
        }

        IF_DONT_CONTINUE_PROCESS_LEAVE( IrpContext );

        if( BooleanFlagOn( Args.CreateOptions, FILE_DELETE_ON_CLOSE ) )
            SetFlag( IrpContext->Fcb->Flags, FCB_STATE_DELETE_ON_CLOSE );

        if( FlagOn( Args.CreateOptions, FILE_OPEN_BY_FILE_ID ) )
            SetFlag( ( ( CCB* )FltObjects->FileObject->FsContext2 )->Flags, CCB_STATE_OPEN_BY_FILEID );

        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }
    __finally
    {
        if( IrpContext != NULLPTR )
        {
            if( IrpContext->IsAudit == true )
                PrintIrpContext( IrpContext, true );

            if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_FREE_LOWER_FILEOBJECT ) )
            {
                if( Args.LowerFileHandle != INVALID_HANDLE_VALUE )
                    FltClose( Args.LowerFileHandle );

                if( Args.LowerFileObject != NULLPTR )
                    ObDereferenceObject( Args.LowerFileObject );
            }

            if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
                FltStatus = IrpContext->PreFltStatus;
        }

        DeallocateBuffer( &Args.CreateFileName );
        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

NTSTATUS CreateFileExistFCB( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    auto Data = IrpContext->Data;
    auto Args = ( CREATE_ARGS* )IrpContext->Params;

    __try
    {
        if( IrpContext->Fcb == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            AssignCmnResult( IrpContext, Status );
            __leave;
        }

        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

        if( nsUtils::VerifyVersionInfoEx( 6, 1, ">=") == true )
        {
            auto Ret = GlobalFltMgr.FltCheckOplockEx( &IrpContext->Fcb->FileOplock, IrpContext->Data, OPLOCK_FLAG_OPLOCK_KEY_CHECK_ONLY,
                                                      NULL, NULL, NULL );

            if( Ret == FLT_PREOP_COMPLETE )
            {
                KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltCheckOplockEx FAILED"
                           , Data->IoStatus.Status, ntkernel_error_category::find_ntstatus( Data->IoStatus.Status )->message
                           ) );

                AssignCmnResult( IrpContext, IrpContext->Data->IoStatus.Status );
                AssignCmnFltResult( IrpContext, Ret );
                SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                __leave;
            }
        }

        if( FltCurrentBatchOplock( &IrpContext->Fcb->FileOplock ) )
        {
            AssignCmnResultInfo( IrpContext, FILE_OPBATCH_BREAK_UNDERWAY );

            // 만약 OPLOCK BREAK 이 진행 중이라면 완료될 때 까지 대기한다
            auto Ret = FltCheckOplock( &IrpContext->Fcb->FileOplock, IrpContext->Data, NULL, NULL, NULL );
            if( Ret == FLT_PREOP_COMPLETE )
            {
                AssignCmnResult( IrpContext, STATUS_ACCESS_DENIED );
                AssignCmnFltResult( IrpContext, Ret );
                SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
            }
        }

        // 파일을 정상적으로 열 수 있도록 권한 조정
        switch( Args->CreateDesiredAccess )
        {
            case FILE_CREATE: {
                if( IrpContext->Fcb->OpnCount != 0 )
                {
                    AssignCmnResult( IrpContext, STATUS_OBJECT_NAME_COLLISION );
                    SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                    __leave;
                }
            } break;
            case FILE_OVERWRITE:
            case FILE_OVERWRITE_IF: {
                SetFlag( Args->CreateDesiredAccess, FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_WRITE_DATA );
            } break;
            case FILE_SUPERSEDE: {
                SetFlag( Args->CreateDesiredAccess, DELETE );
            } break;
        }

        Status = IoCheckShareAccess( Args->CreateDesiredAccess,
                                     Data->Iopb->Parameters.Create.ShareAccess, Args->FileObject,
                                     &IrpContext->Fcb->LowerShareAccess, FALSE );
        // from X70FSD, Support Oplock
        if( !NT_SUCCESS( Status ) )
        {
            if( nsUtils::VerifyVersionInfoEx( 6, 1, ">=" ) == true )
            {
                
            }

            AssignCmnResult( IrpContext, Status );
            SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
            __leave;
        }

        if( nsUtils::VerifyVersionInfoEx( 6, 1, ">=" ) == true )
        {
            if( IrpContext->Fcb->OpnCount != 0 )
            {
                auto Ret = FltCheckOplock( &IrpContext->Fcb->FileOplock, Data, IrpContext, NULL, NULL );

                if( Ret == FLT_PREOP_COMPLETE )
                {
                    AssignCmnResult( IrpContext, IrpContext->Data->IoStatus.Status );
                    AssignCmnFltResult( IrpContext, Ret );
                    SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                    __leave;
                }
            }

            if( Args->RequiringOplock == true )
            {
                auto Ret = FltOplockFsctrl( &IrpContext->Fcb->FileOplock, Data, IrpContext->Fcb->OpnCount );

                if( Data->IoStatus.Status != STATUS_SUCCESS &&
                    Data->IoStatus.Status != STATUS_OPLOCK_BREAK_IN_PROGRESS )
                {
                    AssignCmnResult( IrpContext, Data->IoStatus.Status );
                    AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );

                    SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                    SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_LOWER_FILEOBJECT );

                    __leave;
                }
            }
        }
        else
        {
            auto Ret = FltCheckOplock( &IrpContext->Fcb->FileOplock, Data, IrpContext, NULL, NULL );

            if( Ret == FLT_PREOP_COMPLETE )
            {
                AssignCmnResult( IrpContext, IrpContext->Data->IoStatus.Status );
                AssignCmnFltResult( IrpContext, Ret );
                SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                __leave;
            }
        }

        // NOTE: MSDN 에 따르면, 쓰기 권한으로 파일을 열기 전에 IRP_MJ_CREATE 에서 MmFlushImageSection 을 MmFlushForWrite 로 호출해야한다
        if( BooleanFlagOn( Args->CreateDesiredAccess, FILE_WRITE_DATA ) || Args->DeleteOnClose )
        {
            if( MmFlushImageSection( &IrpContext->Fcb->SectionObjects, MmFlushForWrite ) == FALSE )
            {
                Status = Args->DeleteOnClose ? STATUS_CANNOT_DELETE : STATUS_SHARING_VIOLATION;
                AssignCmnResult( IrpContext, Status );
                SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                __leave;
            }
        }

        // 해당 파일을 열 때 버퍼링을 사용하지 않는 옵션이라면 기존 캐시된 내용을 모두 기록하도록 한다
        if( BooleanFlagOn( Args->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ) )
        {
            CcFlushCache( &IrpContext->Fcb->SectionObjects, NULL, 0, NULL );
            ExAcquireResourceExclusiveLite( IrpContext->Fcb->AdvFcbHeader.PagingIoResource, TRUE );
            ExReleaseResourceLite( IrpContext->Fcb->AdvFcbHeader.PagingIoResource );
            CcPurgeCacheSection( &IrpContext->Fcb->SectionObjects, NULL, 0, FALSE );
        }

        if( Args->CreateDisposition == FILE_SUPERSEDE || 
            Args->CreateDisposition == FILE_OVERWRITE || 
            Args->CreateDisposition == FILE_OVERWRITE_IF )
        {
            if( MmCanFileBeTruncated( &IrpContext->Fcb->SectionObjects, &ZERO ) == FALSE )
            {
                AssignCmnResult( IrpContext, STATUS_USER_MAPPED_FILE );
                SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                __leave;
            }
        }

        IO_STATUS_BLOCK IoStatus;
        Status = nsW32API::FltCreateFileEx( GlobalContext.Filter,
                                            IrpContext->FltObjects->Instance,
                                            &Args->LowerFileHandle, &Args->LowerFileObject,
                                            Args->CreateDesiredAccess, &Args->CreateObjectAttributes, &IoStatus,
                                            &Data->Iopb->Parameters.Create.AllocationSize,
                                            Data->Iopb->Parameters.Create.FileAttributes,
                                            Data->Iopb->Parameters.Create.ShareAccess,
                                            Args->CreateDisposition, Args->CreateOptions,
                                            Data->Iopb->Parameters.Create.EaBuffer, Data->Iopb->Parameters.Create.EaLength,
                                            0
        );

        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, IoStatus.Information );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Status=0x%08x,%s\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltCreateFileEx FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       ) );

            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
            __leave;
        }

        IrpContext->Ccb = AllocateCcb();
        if( IrpContext->Ccb != NULLPTR )
        {
            IrpContext->Ccb->ProcessId = IrpContext->ProcessId;
            IrpContext->Ccb->ProcessFileFullPath = CloneBuffer( &IrpContext->ProcessFullPath );
            IrpContext->Ccb->ProcessName = nsUtils::ReverseFindW( IrpContext->Ccb->ProcessFileFullPath.Buffer, L'\\' );

            IrpContext->Ccb->SrcFileFullPath = CloneBuffer( &IrpContext->SrcFileFullPath );

            IrpContext->Ccb->LowerFileHandle = Args->LowerFileHandle;
            IrpContext->Ccb->LowerFileObject = Args->LowerFileObject;
        }

        Args->FileObject->Vpb = Args->LowerFileObject->Vpb;
        Args->FileObject->FsContext = IrpContext->Fcb;
        Args->FileObject->FsContext2 = IrpContext->Ccb;
        Args->FileObject->SectionObjectPointer = &IrpContext->Fcb->SectionObjects;
        Args->FileObject->PrivateCacheMap = NULLPTR;               // 파일에 접근할 때 캐시를 초기화한다
        Args->FileObject->Flags |= FO_CACHE_SUPPORTED;

        IrpContext->Fcb->AdvFcbHeader.IsFastIoPossible = CheckIsFastIOPossible( IrpContext->Fcb );

        InterlockedIncrement( &IrpContext->Fcb->OpnCount );
        InterlockedIncrement( &IrpContext->Fcb->ClnCount );
        InterlockedIncrement( &IrpContext->Fcb->RefCount );

        IoUpdateShareAccess( Args->FileObject, &IrpContext->Fcb->LowerShareAccess );

        if( Args->CreateDisposition == FILE_SUPERSEDE || 
            Args->CreateDisposition == FILE_OVERWRITE || 
            Args->CreateDisposition == FILE_OVERWRITE_IF )
        {
            // 파일 크기를 잘라낼 수 있는지 확인한다
            if( MmCanFileBeTruncated( &IrpContext->Fcb->SectionObjects, &ZERO ) )
            {
                // 캐시 데이터를 비운다. IRP_MJ_CLOSE 호출됨
                CcPurgeCacheSection( &IrpContext->Fcb->SectionObjects, NULL, 0, FALSE ); 

                // 캐시의 파일크기를 재설정, 파일크기를 변경하려면 Paging IO 리소스를 획득해야한다
                AcquireCmnResource( IrpContext, FCB_PGIO_EXCLUSIVE );

                IrpContext->Fcb->AdvFcbHeader.FileSize.QuadPart = 0;
                IrpContext->Fcb->AdvFcbHeader.ValidDataLength.QuadPart = 0;
                IrpContext->Fcb->AdvFcbHeader.AllocationSize.QuadPart = IrpContext->Data->Iopb->Parameters.Create.AllocationSize.QuadPart;

                CcSetFileSizes( Args->FileObject, ( PCC_FILE_SIZES )&IrpContext->Fcb->AdvFcbHeader.AllocationSize );

                FltReleaseResource( &IrpContext->Fcb->PagingIoResource );
                ClearFlag( IrpContext->CompleteStatus, COMPLETE_FREE_PGIO_RSRC );

                Status = STATUS_SUCCESS;
            }
            else
            {
                AssignCmnResult( IrpContext, STATUS_USER_MAPPED_FILE );
                SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                __leave;
            }
        }
    }
    __finally
    {
        
    }

    return Status;
}

NTSTATUS CreateFileNonExistFCB( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    auto Args = ( CREATE_ARGS* )IrpContext->Params;
    auto Data = IrpContext->Data;

    __try
    {
        IO_STATUS_BLOCK IoStatus;
        Status = nsW32API::FltCreateFileEx( GlobalContext.Filter,
                                            IrpContext->FltObjects->Instance,
                                            &Args->LowerFileHandle, &Args->LowerFileObject,
                                            Args->CreateDesiredAccess, &Args->CreateObjectAttributes, &IoStatus,
                                            &Data->Iopb->Parameters.Create.AllocationSize,
                                            Data->Iopb->Parameters.Create.FileAttributes,
                                            Data->Iopb->Parameters.Create.ShareAccess,
                                            Args->CreateDisposition, Args->CreateOptions,
                                            Data->Iopb->Parameters.Create.EaBuffer, Data->Iopb->Parameters.Create.EaLength,
                                            0
        );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Status=0x%08x,%s\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltCreateFileEx FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       ) );

            AssignCmnResult( IrpContext, Status );
            AssignCmnResultInfo( IrpContext, IoStatus.Information );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
            __leave;
        }

        SetFlag( Args->FileStatus, FILE_ALREADY_EXISTS );
        AssignCmnResultInfo( IrpContext, IoStatus.Information );

        ULONG BytesReturned = 0;
        FILE_STANDARD_INFORMATION fsi;
        RtlZeroMemory( &fsi, sizeof( fsi ) );

        NTSTATUS Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, Args->LowerFileObject,
                                                   &fsi, sizeof( fsi ), FileStandardInformation, &BytesReturned );

        if( NT_SUCCESS( Status ) )
        {
            Args->FileSize = fsi.EndOfFile;
            Args->FileAllocationSize = fsi.AllocationSize;

            // 만약 성공했는데, 해당 객체가 디렉토리라면 무시한다
            if( fsi.Directory != FALSE )
            {
                SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_LOWER_FILEOBJECT );
                AssignCmnFltResult( IrpContext, FLT_PREOP_SUCCESS_NO_CALLBACK );
                __leave;
            }
        }
        else
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Status=0x%08x,%s\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message ) );
        }

        //switch( CreateArgs.CreateDisposition )
        //{
        //    case FILE_SUPERSEDE:
        //    {
        //        ProcessPreCreate_SUPERSEDE( IrpContext );
        //    } break;
        //    case FILE_OPEN:
        //    {
        //        ProcessPreCreate_OPEN( IrpContext );
        //    } break;
        //    case FILE_CREATE:
        //    {
        //        ProcessPreCreate_CREATE( IrpContext );
        //    } break;
        //    case FILE_OPEN_IF:
        //    {
        //        ProcessPreCreate_OPEN_IF( IrpContext );
        //    } break;
        //    case FILE_OVERWRITE:
        //    {
        //        ProcessPreCreate_OVERWRITE( IrpContext );
        //    } break;
        //    case FILE_OVERWRITE_IF:
        //    {
        //        ProcessPreCreate_OVERWRITE_IF( IrpContext );
        //    } break;
        //} // switch CreateDisposition
        //

        AcquireCmnResource( IrpContext, INST_EXCLUSIVE );

        Status = InitializeFcbAndCcb( IrpContext );
        if( !NT_SUCCESS( Status ) )
        {
            AssignCmnResult( IrpContext, Status );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
            __leave;
        }

        {
            // NOTE: 캐시관리자에서 사용할 파일 객체를 생성한다.
            Status = nsW32API::FltCreateFileEx( IrpContext->FltObjects->Filter,
                                                IrpContext->FltObjects->Instance,
                                                &IrpContext->Fcb->LowerFileHandle,
                                                &IrpContext->Fcb->LowerFileObject,
                                                FILE_SPECIAL_ACCESS,
                                                &Args->CreateObjectAttributes,
                                                &IoStatus, NULL,
                                                FILE_ATTRIBUTE_NORMAL, 0,
                                                FILE_OPEN,
                                                FILE_NON_DIRECTORY_FILE,
                                                NULL, 0, IO_IGNORE_SHARE_ACCESS_CHECK );

            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%wZ\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltCreateFileEx FAILED", __LINE__
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , &Args->CreateFileNameUS
                           ) );

                AssignCmnResult( IrpContext, Status );
                AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );

                SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_LOWER_FILEOBJECT);

                __leave;
            }
        }

        if( nsUtils::VerifyVersionInfoEx( 6, 1, ">=" ) == true )
        {
            auto Ret = GlobalFltMgr.FltCheckOplockEx( &IrpContext->Fcb->FileOplock,
                                                      Data, OPLOCK_FLAG_OPLOCK_KEY_CHECK_ONLY,
                                                      NULL, NULL, NULL );

            if( Ret == FLT_PREOP_COMPLETE )
            {
                AssignCmnResult( IrpContext, Data->IoStatus.Status );
                AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );

                SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_LOWER_FILEOBJECT );

                __leave;
            }

            if( Args->RequiringOplock == true )
            {
                Ret = FltOplockFsctrl( &IrpContext->Fcb->FileOplock, Data, IrpContext->Fcb->OpnCount );

                if( Data->IoStatus.Status != STATUS_SUCCESS && 
                    Data->IoStatus.Status != STATUS_OPLOCK_BREAK_IN_PROGRESS )
                {
                    AssignCmnResult( IrpContext, Data->IoStatus.Status );
                    AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );

                    SetFlag( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
                    SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_LOWER_FILEOBJECT );

                    __leave;
                }
            }
        }

        IoSetShareAccess( Args->CreateDesiredAccess, Data->Iopb->Parameters.Create.ShareAccess,
                          Args->FileObject, &IrpContext->Fcb->LowerShareAccess );

        IrpContext->Ccb->LowerFileObject = Args->LowerFileObject;
        IrpContext->Ccb->LowerFileHandle = Args->LowerFileHandle;

        Args->FileObject->Vpb = Args->LowerFileObject->Vpb;
        Args->FileObject->FsContext = IrpContext->Fcb;
        Args->FileObject->FsContext2 = IrpContext->Ccb;
        Args->FileObject->SectionObjectPointer = &IrpContext->Fcb->SectionObjects;
        Args->FileObject->PrivateCacheMap = NULLPTR;               // 파일에 접근할 때 캐시를 초기화한다
        Args->FileObject->Flags |= FO_CACHE_SUPPORTED;

        Vcb_InsertFCB( IrpContext->InstanceContext, IrpContext->Fcb );
    }
    __finally
    {

    }

    return Status;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostCreate( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                    PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    return FltStatus;
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS OpenLowerFileObject( PIRP_CONTEXT IrpContext )
{
    if( IrpContext == NULLPTR )
        return STATUS_INVALID_PARAMETER;

    CREATE_ARGS* CreateArgs = ( CREATE_ARGS* )IrpContext->Params;
    NTSTATUS& Status = IrpContext->Status;

    auto SecurityContext = IrpContext->Data->Iopb->Parameters.Create.SecurityContext;
    auto DesiredAccess = SecurityContext->DesiredAccess;
    auto AllocationSize = IrpContext->Data->Iopb->Parameters.Create.AllocationSize;
    auto FileAttributes = IrpContext->Data->Iopb->Parameters.Create.FileAttributes;
    auto CreateDisposition = ( IrpContext->Data->Iopb->Parameters.Create.Options >> 24 ) & 0x000000ff;

    CreateArgs->CreateFileName = AllocateBuffer<WCHAR>( BUFFER_FILENAME, IrpContext->SrcFileFullPath.BufferSize + _countof( IrpContext->InstanceContext->DeviceNameBuffer ) );

    RtlStringCbCatW( CreateArgs->CreateFileName.Buffer, CreateArgs->CreateFileName.BufferSize, IrpContext->InstanceContext->DeviceNameBuffer );
    RtlStringCbCatW( CreateArgs->CreateFileName.Buffer, CreateArgs->CreateFileName.BufferSize, IrpContext->SrcFileFullPathWOVolume );
    RtlInitUnicodeString( &CreateArgs->CreateFileNameUS, CreateArgs->CreateFileName.Buffer );

    InitializeObjectAttributes( &CreateArgs->CreateObjectAttributes, &CreateArgs->CreateFileNameUS, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );

    __try
    {
        IO_STATUS_BLOCK IoStatus;

        Status = nsW32API::FltCreateFileEx2( GlobalContext.Filter,
                                             IrpContext->FltObjects->Instance,
                                             &CreateArgs->LowerFileHandle, &CreateArgs->LowerFileObject,
                                             DesiredAccess, &CreateArgs->CreateObjectAttributes, &IoStatus,
                                             &AllocationSize, FileAttributes, FILE_SHARE_READ,
                                             CreateDisposition, 0, NULL, 0, IO_FORCE_ACCESS_CHECK, NULLPTR );

        IrpContext->Information = IoStatus.Information;

        if( NT_SUCCESS( Status ) )
        {
            SetFlag( CreateArgs->FileStatus, FILE_ALREADY_EXISTS );
        }
    }
    __finally
    {
        
    }

    KdPrintEx( ( DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[WinIOSol] %s Status=0x%08x Information=0x%08x Name=%wZ\n",
                 __FUNCTION__, Status, IrpContext->Information, &CreateArgs->CreateFileNameUS ) );
    return Status;
}

ACCESS_MASK CreateDesiredAccess( PIRP_CONTEXT IrpContext )
{
    ACCESS_MASK CreatedDesiredAccess = 0;
    CTX_INSTANCE_CONTEXT* InstanceContext = IrpContext->InstanceContext;

    auto CreateDisposition = ( IrpContext->Data->Iopb->Parameters.Create.Options >> 24 ) & 0x000000ff;
    auto SecurityContext = IrpContext->Data->Iopb->Parameters.Create.SecurityContext;
    auto OriginalDesiredAccess = SecurityContext->DesiredAccess;

    CreatedDesiredAccess = OriginalDesiredAccess;

    switch( InstanceContext->VolumeProperties.DeviceType )
    {
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM: {
            switch( CreateDisposition )
            {
                case FILE_SUPERSEDE: {} break;
                case FILE_OPEN: {} break;
                case FILE_CREATE: {} break;
                case FILE_OPEN_IF: {} break;
                case FILE_OVERWRITE: {} break;
                case FILE_OVERWRITE_IF: {} break;
            } // switch CreateDisposition
        } break;      // CD, DVD, Blu-Ray, etc...
        case FILE_DEVICE_DISK_FILE_SYSTEM: {
            switch( CreateDisposition )
            {
                case FILE_SUPERSEDE: {} break;
                case FILE_OPEN: {} break;
                case FILE_CREATE: {} break;
                case FILE_OPEN_IF: {} break;
                case FILE_OVERWRITE: {} break;
                case FILE_OVERWRITE_IF: {} break;
            } // switch CreateDisposition
        } break;
        case FILE_DEVICE_FILE_SYSTEM: {} break;             // Control Device Object
        case FILE_DEVICE_NETWORK_FILE_SYSTEM: {
            switch( CreateDisposition )
            {
                case FILE_SUPERSEDE: {} break;
                case FILE_OPEN: {} break;
                case FILE_CREATE: {} break;
                case FILE_OPEN_IF: {} break;
                case FILE_OVERWRITE: {} break;
                case FILE_OVERWRITE_IF: {} break;
            } // switch CreateDisposition
        } break;
        case FILE_DEVICE_TAPE_FILE_SYSTEM: {} break;
        case FILE_DEVICE_DFS_FILE_SYSTEM: {} break;
    }

    return CreatedDesiredAccess;
}

void InitializeVolumeProperties( PIRP_CONTEXT IrpContext )
{
    auto InstanceContext = ( CTX_INSTANCE_CONTEXT* )IrpContext->InstanceContext;

    /*!
        MSDN 의 권고문에는 InstanceSetupCallback 에서 수행하는 것을 권장하지만,
        몇몇 USB 장치의 볼륨에 대한 정보를 가져올 때 OS 가 응답없음에 빠지는 경우가 존재하여
        이곳에서 값을 가져옴
    */
    if( InstanceContext->IsVolumePropertySet == FALSE && IrpContext->FltObjects->Volume != NULL )
    {
        ULONG                      nReturnedLength = 0;

        FltGetVolumeProperties( IrpContext->FltObjects->Volume,
                                &InstanceContext->VolumeProperties,
                                sizeof( UCHAR ) * _countof( InstanceContext->Data ),
                                &nReturnedLength );

        KeMemoryBarrier();

        InstanceContext->IsVolumePropertySet = TRUE;
    }

    // TODO: 향후 별도 함수로 분리
    if( IrpContext->InstanceContext->IsAllocationPropertySet == FALSE )
    {
        NTSTATUS Status = STATUS_SUCCESS;
        IO_STATUS_BLOCK IoStatus;
        FILE_FS_FULL_SIZE_INFORMATION fsFullSizeInformation;
        RtlZeroMemory( &fsFullSizeInformation, sizeof( FILE_FS_FULL_SIZE_INFORMATION ) );

        switch( IrpContext->InstanceContext->VolumeProperties.DeviceType )
        {
            case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            {
                IrpContext->InstanceContext->BytesPerSector = 2048;
                IrpContext->InstanceContext->SectorsPerAllocationUnit = 1;
            } break;
            case FILE_DEVICE_DISK_FILE_SYSTEM:
            {
                Status = FltQueryVolumeInformation( IrpContext->FltObjects->Instance,
                                                    &IoStatus, &fsFullSizeInformation, sizeof( FILE_FS_FULL_SIZE_INFORMATION )
                                                    , FileFsFullSizeInformation );

                if( !NT_SUCCESS( Status ) )
                {
                    KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n"
                               , IrpContext->EvtID, __FUNCTION__
                               , "FltQueryVolumeInformation FAILED", __LINE__
                               , Status, ntkernel_error_category::find_ntstatus( Status )->message
                               , IrpContext->InstanceContext->DeviceNameBuffer ) );
                    break;
                }

                IrpContext->InstanceContext->BytesPerSector = fsFullSizeInformation.BytesPerSector;
                IrpContext->InstanceContext->SectorsPerAllocationUnit = fsFullSizeInformation.SectorsPerAllocationUnit;
            } break;
            case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            {
                ULONG LengthReturned = 0;
                Status = FltQueryVolumeInformationFile( IrpContext->FltObjects->Instance
                                                        , ( ( CREATE_ARGS* )IrpContext->Params )->LowerFileObject
                                                        , &fsFullSizeInformation, sizeof( FILE_FS_FULL_SIZE_INFORMATION )
                                                        , FileFsFullSizeInformation
                                                        , &LengthReturned );
                if( !NT_SUCCESS( Status ) )
                {
                    KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Dev=%ws Src=%ws\n"
                               , IrpContext->EvtID, __FUNCTION__
                               , "FltQueryVolumeInformationFile FAILED", __LINE__
                               , Status, ntkernel_error_category::find_ntstatus( Status )->message
                               , IrpContext->InstanceContext->DeviceNameBuffer
                               , IrpContext->SrcFileFullPath.Buffer ) );
                    break;
                }

                IrpContext->InstanceContext->BytesPerSector = fsFullSizeInformation.BytesPerSector;
                IrpContext->InstanceContext->SectorsPerAllocationUnit = fsFullSizeInformation.SectorsPerAllocationUnit;
            } break;
        }

        if( NT_SUCCESS( Status ) )
        {
            IrpContext->InstanceContext->ClusterSize = IrpContext->InstanceContext->BytesPerSector * IrpContext->InstanceContext->SectorsPerAllocationUnit;

            KeMemoryBarrier();
            IrpContext->InstanceContext->IsAllocationPropertySet = TRUE;
        }
    }

}

NTSTATUS ProcessPreCreate_SUPERSEDE( IRP_CONTEXT* Args )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_SUPERSEDE_NEW( Args );

    return ProcessPreCreate_SUPERSEDE_EXIST( Args );
}

NTSTATUS ProcessPreCreate_SUPERSEDE_NEW( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {
        if( BooleanFlagOn( CreateArgs->FileStatus, FILE_ALREADY_EXISTS ) )
        {
            CreateArgs->FileSize.QuadPart = 0;

            AssignCmnResult( Args, STATUS_SUCCESS );
            AssignCmnResultInfo( Args, FILE_SUPERSEDED );
            AssignCmnFltResult( Args, FLT_PREOP_COMPLETE );
        }
        else
        {

            //nsW32API::FltCreateFileEx2( GlobalContext.Filter,
            //                            Args->FltObjects->Instance,
            //                            &CreateArgs->LowerFileHandle, &CreateArgs->LowerFileObject,
            //                            0, );
        }
    }
    __finally
    {
        
    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_SUPERSEDE_EXIST( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_OPEN( IRP_CONTEXT* Args )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_OPEN_NEW( Args );

    return ProcessPreCreate_OPEN_EXIST( Args );
}

NTSTATUS ProcessPreCreate_OPEN_NEW( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {
        // 파일이 존재하지 않으면 오류 코드를 반환하고 끝낸다
        if( !BooleanFlagOn( CreateArgs->FileStatus, FILE_ALREADY_EXISTS ) )
        {

            AssignCmnResult( Args, STATUS_OBJECT_NAME_NOT_FOUND );
            AssignCmnResultInfo( Args, 0 );
            AssignCmnFltResult( Args, FLT_PREOP_COMPLETE );

            SetFlag( Args->CompleteStatus, COMPLETE_DONT_CONT_PROCESS );
            __leave;
        }


    }
    __finally
    {

    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_OPEN_EXIST( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_CREATE( IRP_CONTEXT* Args )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_CREATE_NEW( Args );

    return ProcessPreCreate_CREATE_EXIST( Args );
}

NTSTATUS ProcessPreCreate_CREATE_NEW( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_CREATE_EXIST( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_OPEN_IF( IRP_CONTEXT* Args )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_OPEN_IF_NEW( Args );

    return ProcessPreCreate_OPEN_IF_EXIST( Args );
}

NTSTATUS ProcessPreCreate_OPEN_IF_NEW( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_OPEN_IF_EXIST( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_OVERWRITE( IRP_CONTEXT* Args )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_OVERWRITE_NEW( Args );

    return ProcessPreCreate_OVERWRITE_EXIST( Args );
}

NTSTATUS ProcessPreCreate_OVERWRITE_NEW( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_OVERWRITE_EXIST( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_OVERWRITE_IF( IRP_CONTEXT* Args )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_OVERWRITE_IF_NEW( Args );

    return ProcessPreCreate_OVERWRITE_IF_EXIST( Args );
}

NTSTATUS ProcessPreCreate_OVERWRITE_IF_NEW( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Args->Status;
}

NTSTATUS ProcessPreCreate_OVERWRITE_IF_EXIST( IRP_CONTEXT* Args )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Args->Status;
}
