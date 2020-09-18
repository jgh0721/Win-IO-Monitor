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
    auto&                                       IoStatus = Data->IoStatus;

    __try
    {
        RtlZeroMemory( &CreateArgs, sizeof( CREATE_ARGS ) );

        CreateArgs.LowerFileHandle = INVALID_HANDLE_VALUE;
        CreateArgs.LowerFileObject = NULLPTR;

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

        InitializeVolumeProperties( IrpContext );

        /*!
            해당 파일에 대한 Lower FileObject 를 획득한다.
            해당 파일이 이미 필터링 중인가?
            CreateDisposition 에 따라 분기 처리를 한다
        */

        PrintIrpContext( IrpContext );

        CreateArgs.CreateFileName = AllocateBuffer<WCHAR>( BUFFER_FILENAME, IrpContext->SrcFileFullPath.BufferSize + _countof( IrpContext->InstanceContext->DeviceNameBuffer ) );

        RtlStringCbCatW( CreateArgs.CreateFileName.Buffer, CreateArgs.CreateFileName.BufferSize, IrpContext->InstanceContext->DeviceNameBuffer );
        RtlStringCbCatW( CreateArgs.CreateFileName.Buffer, CreateArgs.CreateFileName.BufferSize, IrpContext->SrcFileFullPathWOVolume );
        RtlInitUnicodeString( &CreateArgs.CreateFileNameUS, CreateArgs.CreateFileName.Buffer );

        InitializeObjectAttributes( &CreateArgs.CreateObjectAttributes, &CreateArgs.CreateFileNameUS, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );

        IO_STATUS_BLOCK IoStatus;
        IrpContext->Status = nsW32API::FltCreateFileEx( GlobalContext.Filter,
                                                        FltObjects->Instance,
                                                        &CreateArgs.LowerFileHandle, &CreateArgs.LowerFileObject,
                                                        CreateArgs.CreateDesiredAccess, &CreateArgs.CreateObjectAttributes, &IoStatus,
                                                        &Data->Iopb->Parameters.Create.AllocationSize,
                                                        Data->Iopb->Parameters.Create.FileAttributes,
                                                        Data->Iopb->Parameters.Create.ShareAccess,
                                                        CreateArgs.CreateDisposition, CreateArgs.CreateOptions,
                                                        Data->Iopb->Parameters.Create.EaBuffer, Data->Iopb->Parameters.Create.EaLength,
                                                        IO_FORCE_ACCESS_CHECK
        );

        if( NT_SUCCESS( IrpContext->Status ) )
        {
            SetFlag( CreateArgs.FileStatus, FILE_ALREADY_EXISTS );

            ULONG BytesReturned = 0;
            FILE_STANDARD_INFORMATION fsi;
            RtlZeroMemory( &fsi, sizeof( fsi ) );

            NTSTATUS Status = FltQueryInformationFile( FltObjects->Instance, CreateArgs.LowerFileObject,
                                                       &fsi, sizeof( fsi ), FileStandardInformation, &BytesReturned );

            if( NT_SUCCESS( Status ) )
            {
                CreateArgs.FileSize = fsi.EndOfFile;
                CreateArgs.FileAllocationSize = fsi.AllocationSize;

                // 만약 성공했는데, 해당 객체가 디렉토리라면 무시한다
                if( fsi.Directory != FALSE )
                {
                    SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_LOWER_FILEOBJECT );
                    SetFlag( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS );

                    IrpContext->PreFltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
                    __leave;
                }
            }
            else
            {
                KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Status=0x%08x,%s\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message ) );
            }

        }

        AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
        CreateArgs.Fcb = Vcb_SearchFCB( IrpContext->InstanceContext, IrpContext->SrcFileFullPathWOVolume );

        switch( CreateArgs.CreateDisposition )
        {
            case FILE_SUPERSEDE:
            {
                ProcessPreCreate_SUPERSEDE( IrpContext );
            } break;
            case FILE_OPEN:
            {
                ProcessPreCreate_OPEN( IrpContext );
            } break;
            case FILE_CREATE:
            {
                ProcessPreCreate_CREATE( IrpContext );
            } break;
            case FILE_OPEN_IF:
            {
                ProcessPreCreate_OPEN_IF( IrpContext );
            } break;
            case FILE_OVERWRITE:
            {
                ProcessPreCreate_OVERWRITE( IrpContext );
            } break;
            case FILE_OVERWRITE_IF:
            {
                ProcessPreCreate_OVERWRITE_IF( IrpContext );
            } break;
        } // switch CreateDisposition

        IF_DONT_CONTINUE_PROCESS_LEAVE( IrpContext );

        if( CreateArgs.Fcb == NULLPTR )
        {
            CreateArgs.Fcb = AllocateFcb();
            InitializeFCB( CreateArgs.Fcb, IrpContext );

            IoSetShareAccess( CreateArgs.CreateDesiredAccess, Data->Iopb->Parameters.Create.ShareAccess,
                              CreateArgs.FileObject, &CreateArgs.Fcb->LowerShareAccess );

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
            SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_LOWER_FILEOBJECT );
        }

        CreateArgs.FileObject->Vpb = CreateArgs.LowerFileObject->Vpb;
        CreateArgs.FileObject->FsContext = CreateArgs.Fcb;
        CreateArgs.FileObject->FsContext2 = NULLPTR;
        CreateArgs.FileObject->SectionObjectPointer = &CreateArgs.Fcb->SectionObjects;
        CreateArgs.FileObject->PrivateCacheMap = NULLPTR;               // 파일에 접근할 때 캐시를 초기화한다
        CreateArgs.FileObject->Flags |= FO_CACHE_SUPPORTED;

        if( BooleanFlagOn( CreateArgs.CreateOptions, FILE_DELETE_ON_CLOSE ) )
            SetFlag( CreateArgs.Fcb->Flags, FILE_DELETE_ON_CLOSE );

        IrpContext->PreFltStatus = FLT_PREOP_COMPLETE;
        SetFlag( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS );

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
        if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
            FltStatus = IrpContext->PreFltStatus;

        if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_FREE_LOWER_FILEOBJECT ) )
        {
            if( CreateArgs.LowerFileHandle != INVALID_HANDLE_VALUE )
                FltClose( CreateArgs.LowerFileHandle );

            if( CreateArgs.LowerFileObject != NULLPTR )
                ObDereferenceObject( CreateArgs.LowerFileObject );
        }

        if( IrpContext != NULLPTR )
        {
            if( IrpContext->IsAudit == true )
            {
                KdPrint( ( "[WinIOSol] EvtID=%09d %s Status=0x%08x Information=%s Name=%ws\n",
                           IrpContext->EvtID, __FUNCTION__
                           , IrpContext->Status, nsW32API::ConvertCreateResultInformation( IrpContext->Status, IrpContext->Information )
                           , IrpContext->SrcFileFullPath.Buffer ) );
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
