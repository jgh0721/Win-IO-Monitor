#include "fltCreateFile.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"
#include "utilities/bufferMgr.hpp"

#include "fltCmnLibs.hpp"

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

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreCreate( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                  PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;
    CREATE_ARGS                                 CreateArgs;
    CREATE_RESULT                               CreateResult;
    auto&                                       IoStatus = Data->IoStatus;

    __try
    {
        RtlZeroMemory( &CreateArgs, sizeof( CREATE_ARGS ) );
        RtlZeroMemory( &CreateResult, sizeof( CREATE_RESULT ) );

        ///////////////////////////////////////////////////////////////////////
        /// Validate Input

        if( BooleanFlagOn( Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE ) )
            __leave;

        auto SecurityContext = Data->Iopb->Parameters.Create.SecurityContext;

        CreateArgs.FileObject = Data->Iopb->TargetFileObject;
        CreateArgs.CreateOptions = Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;
        CreateArgs.CreateDisposition = ( Data->Iopb->Parameters.Create.Options >> 24 ) & 0x000000ff;
        CreateArgs.CreateDesiredAccess = SecurityContext->DesiredAccess;

        if( BooleanFlagOn( CreateArgs.CreateOptions, FILE_DIRECTORY_FILE ) )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );
        if( IrpContext == NULLPTR )
            __leave;

        if( IrpContext->SrcFileFullPath.Buffer == NULLPTR )
            __leave;
        
        if( nsUtils::EndsWithW( IrpContext->SrcFileFullPath.Buffer, L"IsolationTest.TXT" ) == false )
            __leave;

        IrpContext->IsAudit = true;
        IrpContext->Params = &CreateArgs;
        IrpContext->Result = &CreateResult;

        /*!
            해당 파일에 대한 Lower FileObject 를 획득한다.
            해당 파일이 이미 필터링 중인가?
            CreateDisposition 에 따라 분기 처리를 한다
        */

        KdPrint( ( "[WinIOSol] EVTID=%09d %s ShareAccess=%s Disposition=%s DoC=%d Name=%ws\n", 
                   IrpContext->EvtID, __FUNCTION__
                   , nsW32API::ConvertCreateShareAccess( Data->Iopb->Parameters.Create.ShareAccess )
                   , nsW32API::ConvertCreateDisposition( CreateArgs.CreateDisposition )
                   , BooleanFlagOn( CreateArgs.CreateOptions, FILE_DELETE_ON_CLOSE )
                   , IrpContext->SrcFileFullPath.Buffer ) );

        CreateArgs.CreateFileName = AllocateBuffer<WCHAR>( BUFFER_FILENAME, IrpContext->SrcFileFullPath.BufferSize + _countof( IrpContext->InstanceContext->DeviceNameBuffer ) );

        RtlStringCbCatW( CreateArgs.CreateFileName.Buffer, CreateArgs.CreateFileName.BufferSize, IrpContext->InstanceContext->DeviceNameBuffer );
        RtlStringCbCatW( CreateArgs.CreateFileName.Buffer, CreateArgs.CreateFileName.BufferSize, IrpContext->SrcFileFullPathWOVolume );
        RtlInitUnicodeString( &CreateArgs.CreateFileNameUS, CreateArgs.CreateFileName.Buffer );

        InitializeObjectAttributes( &CreateArgs.CreateObjectAttributes, &CreateArgs.CreateFileNameUS, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );

        CreateResult.IoStatus.Status = 
        nsW32API::FltCreateFileEx( GlobalContext.Filter,
                                   FltObjects->Instance,
                                   &CreateArgs.LowerFileHandle, &CreateArgs.LowerFileObject,
                                   CreateArgs.CreateDesiredAccess, &CreateArgs.CreateObjectAttributes , &CreateResult.IoStatus,
                                   &Data->Iopb->Parameters.Create.AllocationSize,
                                   Data->Iopb->Parameters.Create.FileAttributes,
                                   Data->Iopb->Parameters.Create.ShareAccess,
                                   CreateArgs.CreateDisposition, CreateArgs.CreateOptions,
                                   Data->Iopb->Parameters.Create.EaBuffer, Data->Iopb->Parameters.Create.EaLength,
                                   IO_FORCE_ACCESS_CHECK
                                   );

        FltAcquireResourceExclusive( &IrpContext->InstanceContext->VcbLock );
        CreateArgs.Fcb = Vcb_SearchFCB( IrpContext->InstanceContext, IrpContext->SrcFileFullPathWOVolume );
        SetFlag( CreateResult.CompleteStatus, COMPLETE_FREE_INST_RSRC );

        switch( CreateArgs.CreateDisposition )
        {
            case FILE_SUPERSEDE:
            {
                ProcessPreCreate_SUPERSEDE( IrpContext, &CreateResult );
            } break;
            case FILE_OPEN:
            {
                ProcessPreCreate_OPEN( IrpContext, &CreateResult );
            } break;
            case FILE_CREATE:
            {
                ProcessPreCreate_CREATE( IrpContext, &CreateResult );
            } break;
            case FILE_OPEN_IF:
            {
                ProcessPreCreate_OPEN_IF( IrpContext, &CreateResult );
            } break;
            case FILE_OVERWRITE:
            {
                ProcessPreCreate_OVERWRITE( IrpContext, &CreateResult );
            } break;
            case FILE_OVERWRITE_IF:
            {
                ProcessPreCreate_OVERWRITE_IF( IrpContext, &CreateResult );
            } break;
        } // switch CreateDisposition

        if( BooleanFlagOn( CreateResult.CompleteStatus, COMPLETE_DONT_CONTINUE_PROCESS ) )
            __leave;

        if( CreateArgs.Fcb == NULLPTR )
        {
            CreateArgs.Fcb = AllocateFcb();
            InitializeFCB( CreateArgs.Fcb, IrpContext );

            CreateArgs.FileObject->FsContext = CreateArgs.Fcb;
            CreateArgs.FileObject->FsContext2 = NULLPTR;

            IoSetShareAccess( CreateArgs.CreateDesiredAccess, Data->Iopb->Parameters.Create.ShareAccess,
                              CreateArgs.FileObject, &CreateArgs.Fcb->LowerShareAccess );

            CreateArgs.FileObject->Flags |= FO_CACHE_SUPPORTED;
            CreateArgs.FileObject->PrivateCacheMap = NULLPTR;               // 파일에 접근할 때 캐시를 초기화한다
            CreateArgs.FileObject->Vpb = CreateArgs.LowerFileObject->Vpb;

            CreateArgs.Fcb->LowerFileObject = CreateArgs.LowerFileObject;
            CreateArgs.Fcb->LowerFileHandle = CreateArgs.LowerFileHandle;

            Vcb_InsertFCB( IrpContext->InstanceContext, CreateArgs.Fcb );
        }
        else
        {
            InterlockedIncrement( &CreateArgs.Fcb->OpnCount );
            InterlockedIncrement( &CreateArgs.Fcb->ClnCount );
            InterlockedIncrement( &CreateArgs.Fcb->RefCount );

            // 최초에 LowerFileObject/Handle 을 설정했다면 추가로 설정할 필요 없음
            FltClose( CreateArgs.LowerFileHandle );
            ObDereferenceObject( CreateArgs.LowerFileObject );
        }

        if( BooleanFlagOn( CreateArgs.CreateOptions, FILE_DELETE_ON_CLOSE ) )
            SetFlag( CreateArgs.Fcb->Flags, FILE_DELETE_ON_CLOSE );

        CreateResult.FltStatus = FLT_PREOP_COMPLETE;

        //CreateResult.IoStatus.Status = OpenLowerFileObject( IrpContext );

        //FltAcquireResourceExclusive( &IrpContext->InstanceContext->VcbLock );
        //CreateArgs.Fcb = Vcb_SearchFCB( IrpContext->InstanceContext, IrpContext->SrcFileFullPathWOVolume );

        //// NOTE: 현재 해당 파일이 필터링 중이지 않다면 VCB 락을 해제하고, 그렇지 않으면 VCB 락을 유지한다. 
        //if( CreateArgs.Fcb == NULLPTR )
        //    FltReleaseResource( &IrpContext->InstanceContext->VcbLock );
        //else
        //    SetFlag( CreateResult.CompleteStatus, COMPLETE_FREE_INST_RSRC );


        //if( BooleanFlagOn( CreateResult.CompleteStatus, COMPLETE_ALLOCATE_FCB ) )
        //{
        //    CreateArgs.Fcb = Vcb_SearchFCB( IrpContext->InstanceContext, IrpContext->SrcFileFullPathWOVolume );

        //}

        //CreateArgs.Fcb = AllocateFcb();
        //if( CreateArgs.Fcb == NULLPTR )
        //{
        //    CreateResult.FltStatus = FLT_PREOP_COMPLETE;
        //    CreateResult.IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        //    __leave;
        //}

    }
    __finally
    {
        FltStatus = CreateResult.FltStatus;
        Data->IoStatus = CreateResult.IoStatus;

        if( IrpContext != NULLPTR )
        {
            if( IrpContext->IsAudit == true )
            {
                KdPrint( ( "[WinIOSol] EvtID=%09d %s Status=0x%08x Information=%s Name=%ws\n",
                           IrpContext->EvtID, __FUNCTION__
                           , CreateResult.IoStatus.Status, nsW32API::ConvertCreateResultInformation( CreateResult.IoStatus.Status, CreateResult.IoStatus.Information )
                           , IrpContext->SrcFileFullPath.Buffer ) );


                if( BooleanFlagOn( CreateResult.CompleteStatus, COMPLETE_FREE_INST_RSRC ) )
                    FltReleaseResource( &IrpContext->InstanceContext->VcbLock );
            }
        }

        DeallocateBuffer( &CreateArgs.CreateFileName );
        CloseIrpContext( IrpContext );
    }

    return FltStatus;
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
    CREATE_RESULT* CreateResult = ( CREATE_RESULT* )IrpContext->Result;

    NTSTATUS& Status = CreateResult->IoStatus.Status;


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
        Status = nsW32API::FltCreateFileEx2( GlobalContext.Filter,
                                             IrpContext->FltObjects->Instance,
                                             &CreateArgs->LowerFileHandle, &CreateArgs->LowerFileObject,
                                             DesiredAccess, &CreateArgs->CreateObjectAttributes, &CreateResult->IoStatus,
                                             &AllocationSize, FileAttributes, FILE_SHARE_READ,
                                             CreateDisposition, 0, NULL, 0, IO_FORCE_ACCESS_CHECK, NULLPTR );

        if( NT_SUCCESS( Status ) )
        {
            SetFlag( CreateArgs->FileStatus, FILE_ALREADY_EXISTS );
        }
    }
    __finally
    {
        
    }

    KdPrintEx( ( DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[WinIOSol] %s Status=0x%08x Information=0x%08x Name=%wZ\n",
                 __FUNCTION__, Status, CreateResult->IoStatus.Information, &CreateArgs->CreateFileNameUS ) );
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

NTSTATUS ProcessPreCreate_SUPERSEDE( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_SUPERSEDE_NEW( Args, Result );

    return ProcessPreCreate_SUPERSEDE_EXIST( Args, Result );
}

NTSTATUS ProcessPreCreate_SUPERSEDE_NEW( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {
        if( BooleanFlagOn( CreateArgs->FileStatus, FILE_ALREADY_EXISTS ) )
        {
            CreateArgs->FileSize.QuadPart = 0;

            Result->IoStatus.Status = STATUS_SUCCESS;
            Result->IoStatus.Information = FILE_SUPERSEDED;
            Result->FltStatus = FLT_PREOP_COMPLETE;
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

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_SUPERSEDE_EXIST( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_OPEN( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_OPEN_NEW( Args, Result );

    return ProcessPreCreate_OPEN_EXIST( Args, Result );
}

NTSTATUS ProcessPreCreate_OPEN_NEW( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {
        // 파일이 존재하지 않으면 오류 코드를 반환하고 끝낸다
        if( !BooleanFlagOn( CreateArgs->FileStatus, FILE_ALREADY_EXISTS ) )
        {
            Result->IoStatus.Status = STATUS_OBJECT_NAME_NOT_FOUND;
            Result->IoStatus.Information = 0;
            Result->FltStatus = FLT_PREOP_COMPLETE;

            SetFlag( Result->CompleteStatus, COMPLETE_DONT_CONTINUE_PROCESS );
            __leave;
        }


    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_OPEN_EXIST( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_CREATE( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_CREATE_NEW( Args, Result );

    return ProcessPreCreate_CREATE_EXIST( Args, Result );
}

NTSTATUS ProcessPreCreate_CREATE_NEW( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_CREATE_EXIST( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_OPEN_IF( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_OPEN_IF_NEW( Args, Result );

    return ProcessPreCreate_OPEN_IF_EXIST( Args, Result );
}

NTSTATUS ProcessPreCreate_OPEN_IF_NEW( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_OPEN_IF_EXIST( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_OVERWRITE( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_OVERWRITE_NEW( Args, Result );

    return ProcessPreCreate_OVERWRITE_EXIST( Args, Result );
}

NTSTATUS ProcessPreCreate_OVERWRITE_NEW( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_OVERWRITE_EXIST( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_OVERWRITE_IF( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    if( ( ( CREATE_ARGS* )Args->Params )->Fcb == NULLPTR )
        return ProcessPreCreate_OVERWRITE_IF_NEW( Args, Result );

    return ProcessPreCreate_OVERWRITE_IF_EXIST( Args, Result );
}

NTSTATUS ProcessPreCreate_OVERWRITE_IF_NEW( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}

NTSTATUS ProcessPreCreate_OVERWRITE_IF_EXIST( IRP_CONTEXT* Args, CREATE_RESULT* Result )
{
    auto CreateArgs = ( ( CREATE_ARGS* )Args->Params );

    __try
    {

    }
    __finally
    {

    }

    return Result->IoStatus.Status;
}
