#include "irpContext.hpp"

#include "utilities/procNameMgr.hpp"
#include "utilities/contextMgr.hpp"
#include "utilities/contextMgr_Defs.hpp"

#include "fltCmnLibs.hpp"
#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

LONG volatile GlobalEvtID;

///////////////////////////////////////////////////////////////////////////////

LONG CreateEvtID()
{
    return InterlockedIncrement( &GlobalEvtID );
}

PIRP_CONTEXT CreateIrpContext( __in PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PIRP_CONTEXT            IrpContext = NULLPTR;
    const auto&             MajorFunction = Data->Iopb->MajorFunction;
    auto                    IsPreIO = BooleanFlagOn( Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION ) == FALSE;
    CTX_INSTANCE_CONTEXT*   InstanceContext = NULLPTR;
    PFLT_FILE_NAME_INFORMATION DestinationFileName = NULLPTR;

    __try
    {

        Status = CtxGetContext( FltObjects, NULLPTR, FLT_INSTANCE_CONTEXT, ( PFLT_CONTEXT* )&InstanceContext );
        if( !NT_SUCCESS( Status ) || InstanceContext == NULLPTR )
        {
            Status = STATUS_INTERNAL_ERROR;
            __leave;
        }

        IrpContext = ( PIRP_CONTEXT )ExAllocateFromNPagedLookasideList( &GlobalContext.IrpContextLookasideList );
        if( IrpContext == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory( IrpContext, sizeof( IRP_CONTEXT ) );

        IrpContext->Data            = Data;
        IrpContext->FltObjects      = FltObjects;
        IrpContext->DebugText       = (CHAR*)ExAllocateFromNPagedLookasideList( &GlobalContext.DebugLookasideList );
        RtlZeroMemory( IrpContext->DebugText, 1024 * sizeof( CHAR ) );

        IrpContext->EvtID           = CreateEvtID();
        IrpContext->InstanceContext = InstanceContext;
        FltReferenceContext( IrpContext->InstanceContext );

        if( IsOwnFileObject( FltObjects->FileObject ) == true )
        {
            IrpContext->Fcb = ( FCB* )FltObjects->FileObject->FsContext;
            IrpContext->Ccb = ( CCB* )FltObjects->FileObject->FsContext2;
            IrpContext->SrcFileFullPath = CloneBuffer( &IrpContext->Fcb->FileFullPath );

            if( IrpContext->Ccb != NULLPTR )
            {
                IrpContext->ProcessId = IrpContext->Ccb->ProcessId;
                IrpContext->ProcessFullPath = CloneBuffer( &IrpContext->Ccb->ProcessFileFullPath );
                IrpContext->ProcessFileName = nsUtils::ReverseFindW( IrpContext->ProcessFullPath.Buffer, L'\\' );
            }
        }

        if( IrpContext->ProcessFullPath.Buffer == NULLPTR )
        {
            IrpContext->ProcessId = FltGetRequestorProcessId( Data );
            SearchProcessInfo( IrpContext->ProcessId, &IrpContext->ProcessFullPath, &IrpContext->ProcessFileName );
        }

        if( MajorFunction == IRP_MJ_CREATE && IsPreIO == true )
        {
            /*!
                MSDN 의 권고문에는 InstanceSetupCallback 에서 수행하는 것을 권장하지만,
                몇몇 USB 장치의 볼륨에 대한 정보를 가져올 때 OS 가 응답없음에 빠지는 경우가 존재하여
                이곳에서 값을 가져옴
            */
            if( InstanceContext->IsVolumePropertySet == FALSE && FltObjects->Volume != NULL )
            {
                ULONG                      nReturnedLength = 0;

                FltGetVolumeProperties( FltObjects->Volume,
                                        &InstanceContext->VolumeProperties,
                                        sizeof( UCHAR ) * _countof( InstanceContext->Data ),
                                        &nReturnedLength );

                KeMemoryBarrier();

                InstanceContext->IsVolumePropertySet = TRUE;
            }
        }

        if( IrpContext->SrcFileFullPath.Buffer == NULLPTR )
        {
            IrpContext->SrcFileFullPath = nsUtils::ExtractFileFullPath( FltObjects->FileObject, InstanceContext,
                                                                        ( MajorFunction == IRP_MJ_CREATE ) && ( IsPreIO == true ) );
        }

        if( IrpContext->SrcFileFullPath.Buffer != NULLPTR )
        {
            if( IrpContext->SrcFileFullPath.Buffer[ 1 ] == L':' )
                IrpContext->SrcFileFullPathWOVolume = &IrpContext->SrcFileFullPath.Buffer[ 2 ];
            else
            {
                auto cchDeviceName = nsUtils::strlength( InstanceContext->DeviceNameBuffer );
                if( cchDeviceName > 0 )
                    IrpContext->SrcFileFullPathWOVolume = &IrpContext->SrcFileFullPath.Buffer[ cchDeviceName ];
            }

            if( IrpContext->SrcFileFullPathWOVolume == NULLPTR )
                IrpContext->SrcFileFullPathWOVolume = IrpContext->SrcFileFullPath.Buffer;
        }

        switch( MajorFunction )
        {
            case IRP_MJ_SET_INFORMATION: {
                if( IsPreIO == false )
                    break;

                const auto& FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
                switch( FileInformationClass )
                {
                    case nsW32API::FileRenameInformation:
                    case nsW32API::FileRenameInformationEx: {

                        if( FileInformationClass == nsW32API::FileRenameInformation )
                        {
                            auto InfoBuffer = ( nsW32API::FILE_RENAME_INFORMATION* )( Data->Iopb->Parameters.SetFileInformation.InfoBuffer );
                            Status = FltGetDestinationFileNameInformation( FltObjects->Instance, FltObjects->FileObject, InfoBuffer->RootDirectory,
                                                                           InfoBuffer->FileName, InfoBuffer->FileNameLength,
                                                                           FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                                                                           &DestinationFileName
                            );
                        }
                        else if( FileInformationClass == nsW32API::FileRenameInformationEx )
                        {
                            auto InfoBuffer = ( nsW32API::FILE_RENAME_INFORMATION_EX* )( Data->Iopb->Parameters.SetFileInformation.InfoBuffer );
                            Status = FltGetDestinationFileNameInformation( FltObjects->Instance, FltObjects->FileObject, InfoBuffer->RootDirectory,
                                                                           InfoBuffer->FileName, InfoBuffer->FileNameLength,
                                                                           FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                                                                           &DestinationFileName
                            );
                        }

                        if( !NT_SUCCESS( Status ) )
                            break;

                        IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, DestinationFileName->Name.Length + sizeof( WCHAR ) );
                        if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
                        {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }

                        RtlStringCbCopyW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, DestinationFileName->Name.Buffer );

                    } break;
                } // switch FileInformationClass

            } break;
        } // switch MajorFunction

    }
    __finally
    {
        CtxReleaseContext( InstanceContext );

        if( DestinationFileName != NULLPTR )
            FltReleaseFileNameInformation( DestinationFileName );
    }

    return IrpContext;
}

VOID CloseIrpContext( __in PIRP_CONTEXT IrpContext )
{
    if( IrpContext == NULLPTR )
        return;

    if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_IOSTATUS_STATUS ) )
        IrpContext->Data->IoStatus.Status = IrpContext->Status;

    if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_IOSTATUS_INFORMATION ) )
        IrpContext->Data->IoStatus.Information = IrpContext->Information;

    if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_FREE_LOWER_FILEOBJECT ) )
    {
        
    }

    if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_FREE_PGIO_RSRC ) )
        FltReleaseResource( &IrpContext->Fcb->PagingIoResource );

    if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_FREE_MAIN_RSRC ) )
        FltReleaseResource( &IrpContext->Fcb->MainResource );

    if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_FREE_INST_RSRC ) )
        FltReleaseResource( &IrpContext->InstanceContext->VcbLock );

    ExFreeToNPagedLookasideList( &GlobalContext.DebugLookasideList, IrpContext->DebugText );

    DeallocateBuffer( &IrpContext->ProcessFullPath );
    DeallocateBuffer( &IrpContext->SrcFileFullPath );
    DeallocateBuffer( &IrpContext->DstFileFullPath );

    CtxReleaseContext( IrpContext->InstanceContext );

    ExFreeToNPagedLookasideList( &GlobalContext.IrpContextLookasideList, IrpContext );
}

VOID PrintIrpContext( __in PIRP_CONTEXT IrpContext )
{
    if( IrpContext == NULLPTR ) return;

    const auto& IsPreIO = BooleanFlagOn( IrpContext->Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION ) == FALSE;
    const auto& MajorFunction = IrpContext->Data->Iopb->MajorFunction;
    const auto& MinorFunction = IrpContext->Data->Iopb->MinorFunction;

    switch( MajorFunction )
    {
        case IRP_MJ_CREATE: {
            auto CreateOptions = IrpContext->Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;
            auto Disposition = ( IrpContext->Data->Iopb->Parameters.Create.Options >> 24 ) & 0x000000ff;
            auto SecurityContext = IrpContext->Data->Iopb->Parameters.Create.SecurityContext;
            auto CreateDesiredAccess = SecurityContext->DesiredAccess;

            KdPrint( ( "[WinIOSol] EvtID=%09d IRP=%s Proc=%06d,%ws Src=%ws\n"
                       , IrpContext->EvtID
                       , FltGetIrpName( MajorFunction )
                       , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );

            RtlStringCbCatA( IrpContext->DebugText, 1024, "OpFlags=" );
            nsW32API::PrintOutOperationFlags( IrpContext->DebugText, 1024, IrpContext->Data->Iopb->OperationFlags );
            RtlStringCbCatA( IrpContext->DebugText, 1024, " " );

            RtlStringCbCatA( IrpContext->DebugText, 1024, "DesiredAccess=" );
            nsW32API::PrintOutCreateDesiredAccess( IrpContext->DebugText, 1024, CreateDesiredAccess );
            RtlStringCbCatA( IrpContext->DebugText, 1024, " " );

            RtlStringCbCatA( IrpContext->DebugText, 1024, "CreateOptions=" );
            nsW32API::PrintOutCreateOptions( IrpContext->DebugText, 1024, CreateOptions );

            KdPrint( ( "[WinIOSol] EvtID=%09d       >> Disposition=%s ShareAccess=%s %s \n"
                       , IrpContext->EvtID
                       , nsW32API::ConvertCreateDisposition( Disposition )
                       , nsW32API::ConvertCreateShareAccess( IrpContext->Data->Iopb->Parameters.Create.ShareAccess )
                       , IrpContext->DebugText
                       ) );

        } break;
        case IRP_MJ_CREATE_NAMED_PIPE: {} break;
        case IRP_MJ_CLOSE: {} break;
        case IRP_MJ_READ: 
        case IRP_MJ_WRITE: {

            KdPrint( ( "[WinIOSol] EvtID=%09d IRP=%s Proc=%06d,%ws Src=%ws\n"
                       , IrpContext->EvtID
                       , FltGetIrpName( MajorFunction )
                       , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );

            RtlStringCbCatA( IrpContext->DebugText, 1024, "IrpFlags=" );
            nsW32API::PrintOutIrpFlags( IrpContext->DebugText, 1024, IrpContext->Data->Iopb->IrpFlags );
            RtlStringCbCatA( IrpContext->DebugText, 1024, " " );

            RtlStringCbCatA( IrpContext->DebugText, 1024, "OpFlags=" );
            nsW32API::PrintOutOperationFlags( IrpContext->DebugText, 1024, IrpContext->Data->Iopb->OperationFlags );
            RtlStringCbCatA( IrpContext->DebugText, 1024, " " );

            KdPrint( ( "[WinIOSol] EvtID=%09d       >> %s Key=%d Length=%d ByteOffset=%I64d\n"
                       , IrpContext->EvtID 
                       , IrpContext->DebugText
                       , IrpContext->Data->Iopb->Parameters.Write.Key, IrpContext->Data->Iopb->Parameters.Write.Length, IrpContext->Data->Iopb->Parameters.Write.ByteOffset.QuadPart
                       ) );

        } break;
        case IRP_MJ_QUERY_INFORMATION: {
            auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;

            KdPrint( ( "[WinIOSol] EvtID=%09d IRP=%s Class=%s Length=%d Proc=%06d,%ws Src=%ws\n"
                       , IrpContext->EvtID
                       , FltGetIrpName( MajorFunction )
                       , nsW32API::ConvertFileInformationClassTo( FileInformationClass )
                       , IrpContext->Data->Iopb->Parameters.QueryFileInformation.Length
                       , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );

        } break;
        case IRP_MJ_SET_INFORMATION: {
            auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

            KdPrint( ( "[WinIOSol] EvtID=%09d IRP=%s Class=%s Length=%d Proc=%06d,%ws Src=%ws\n"
                       , IrpContext->EvtID
                       , FltGetIrpName( MajorFunction )
                       , nsW32API::ConvertFileInformationClassTo( FileInformationClass )
                       , IrpContext->Data->Iopb->Parameters.SetFileInformation.Length
                       , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );

        } break;
        case IRP_MJ_QUERY_EA: {} break;
        case IRP_MJ_SET_EA: {} break;
        case IRP_MJ_FLUSH_BUFFERS: {} break;
        case IRP_MJ_QUERY_VOLUME_INFORMATION: {
            auto FsInformationClass = ( nsW32API::FS_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass;

            KdPrint( ( "[WinIOSol] EvtID=%09d IRP=%s Class=%s Proc=%06d,%ws Src=%ws\n"
                       , IrpContext->EvtID
                       , FltGetIrpName( MajorFunction )
                       , nsW32API::ConvertFsInformationClassTo( FsInformationClass )
                       , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );

        } break;
        case IRP_MJ_SET_VOLUME_INFORMATION: {} break;
        case IRP_MJ_DIRECTORY_CONTROL: {} break;
        case IRP_MJ_FILE_SYSTEM_CONTROL: {
            ULONG FsControlCode = 0;

            if( MinorFunction == IRP_MN_KERNEL_CALL || MinorFunction == IRP_MN_USER_FS_REQUEST )
                FsControlCode = IrpContext->Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode;

            KdPrint( ( "[WinIOSol] EvtID=%09d IRP=%s,%s FsControl=0x%08x,%s Proc=%06d,%ws Src=%ws\n"
                       , IrpContext->EvtID
                       , FltGetIrpName( MajorFunction ), nsW32API::ConvertIRPMinorFunction( MajorFunction, MinorFunction )
                       , FsControlCode, FsControlCode == 0 ? "" : nsW32API::ConvertFsControlCode( FsControlCode )
                       , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );

        } break;
        case IRP_MJ_DEVICE_CONTROL: {} break;
        case IRP_MJ_INTERNAL_DEVICE_CONTROL: {} break;
        case IRP_MJ_SHUTDOWN: {} break;
        case IRP_MJ_LOCK_CONTROL: {

            KdPrint( ( "[WinIOSol] EvtID=%09d IRP=%s,%s Proc=%06d,%ws Src=%ws\n"
                       , IrpContext->EvtID
                       , FltGetIrpName( MajorFunction ), nsW32API::ConvertIRPMinorFunction( MajorFunction, MinorFunction )
                       , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );

            auto Length = IrpContext->Data->Iopb->Parameters.LockControl.Length;
            auto Key = IrpContext->Data->Iopb->Parameters.LockControl.Key;
            auto ByteOffset = IrpContext->Data->Iopb->Parameters.LockControl.ByteOffset;
            auto ProcessId = IrpContext->Data->Iopb->Parameters.LockControl.ProcessId;
            auto FailImmediately = IrpContext->Data->Iopb->Parameters.LockControl.FailImmediately;
            auto ExclusiveLock = IrpContext->Data->Iopb->Parameters.LockControl.ExclusiveLock;

            KdPrint( ( "[WinIOSol] EvtID=%09d       >> Length=%I64d Key=%d ByteOffset=%I64d ProcessId=%d FailImmediately=%d ExclusiveLock=%d\n"
                       , IrpContext->EvtID
                       , IrpContext->DebugText
                       , Length->QuadPart, Key, ByteOffset.QuadPart, IrpContext->ProcessId, FailImmediately, ExclusiveLock
                       ) );

        } break;
        case IRP_MJ_CREATE_MAILSLOT: {} break;
        case IRP_MJ_QUERY_SECURITY: {} break;
        case IRP_MJ_SET_SECURITY: {} break;
        case IRP_MJ_POWER: {} break;
        case IRP_MJ_SYSTEM_CONTROL: {} break;
        case IRP_MJ_DEVICE_CHANGE: {} break;
        case IRP_MJ_QUERY_QUOTA: {} break;
        case IRP_MJ_SET_QUOTA: {} break;
        case IRP_MJ_PNP: {} break;
    }
}

VOID AcquireCmnResource( PIRP_CONTEXT IrpContext, LONG RsrcFlags )
{
    //ASSERT( IrpContext != NULLPTR );
    //if( IrpContext == NULLPTR )
    //    return;

    //ASSERT( IrpContext->Fcb != NULLPTR );
    //if( IrpContext->Fcb == NULLPTR )
    //    return;

    if( BooleanFlagOn( RsrcFlags, FCB_MAIN_EXCLUSIVE ) )
    {
        if( BooleanFlagOn( RsrcFlags, FCB_MAIN_SHARED ) )
        {
            KdBreakPoint();
            ASSERT( false );
            return;
        }

        FltAcquireResourceExclusive( &IrpContext->Fcb->MainResource );
        SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_MAIN_RSRC );
    }

    if( BooleanFlagOn( RsrcFlags, FCB_MAIN_SHARED ) )
    {
        if( BooleanFlagOn( RsrcFlags, FCB_MAIN_EXCLUSIVE ) )
        {
            KdBreakPoint();
            ASSERT( false );
            return;
        }

        FltAcquireResourceShared( &IrpContext->Fcb->MainResource );
        SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_MAIN_RSRC );
    }


    if( BooleanFlagOn( RsrcFlags, FCB_PGIO_EXCLUSIVE ) )
    {
        if( BooleanFlagOn( RsrcFlags, FCB_PGIO_SHARED ) )
        {
            KdBreakPoint();
            ASSERT( false );
            return;
        }

        FltAcquireResourceExclusive( &IrpContext->Fcb->PagingIoResource );
        SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_PGIO_RSRC );
    }

    if( BooleanFlagOn( RsrcFlags, FCB_PGIO_SHARED ) )
    {
        if( BooleanFlagOn( RsrcFlags, FCB_PGIO_EXCLUSIVE ) )
        {
            KdBreakPoint();
            ASSERT( false );
            return;
        }

        FltAcquireResourceShared( &IrpContext->Fcb->PagingIoResource );
        SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_PGIO_RSRC );
    }


    if( BooleanFlagOn( RsrcFlags, INST_EXCLUSIVE ) )
    {
        if( BooleanFlagOn( RsrcFlags, INST_SHARED ) )
        {
            KdBreakPoint();
            ASSERT( false );
            return;
        }

        FltAcquireResourceExclusive( &IrpContext->InstanceContext->VcbLock );
        SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_INST_RSRC );
    }

    if( BooleanFlagOn( RsrcFlags, INST_SHARED ) )
    {
        if( BooleanFlagOn( RsrcFlags, INST_EXCLUSIVE ) )
        {
            KdBreakPoint();
            ASSERT( false );
            return;
        }

        FltAcquireResourceShared( &IrpContext->InstanceContext->VcbLock );
        SetFlag( IrpContext->CompleteStatus, COMPLETE_FREE_INST_RSRC );
    }
}
