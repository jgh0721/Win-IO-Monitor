#include "privateFCBMgr.hpp"

#include "callbacks/fltCreateFile.hpp"

#include "irpContext.hpp"
#include "metadata/Metadata.hpp"
#include "utilities/bufferMgr.hpp"
#include "utilities/fltUtilities.hpp"

#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define FCB_NODE_TYPE_TAG 'TzFB'

FCB* AllocateFcb()
{
    auto Fcb = (FCB*)ExAllocateFromNPagedLookasideList( &GlobalContext.FcbLookasideList );
    if( Fcb != NULLPTR )
        RtlZeroMemory( Fcb, sizeof( FCB ) );

    return Fcb;
}

CCB* AllocateCcb()
{
    auto Ccb = ( CCB* )ExAllocateFromNPagedLookasideList( &GlobalContext.CcbLookasideList );
    if( Ccb != NULLPTR )
        RtlZeroMemory( Ccb, sizeof( CCB ) );

    return Ccb;
}

void DeallocateFcb( FCB*& Fcb )
{
    if( Fcb == NULLPTR )
        return;

    ExFreeToNPagedLookasideList( &GlobalContext.FcbLookasideList, Fcb );
    Fcb = NULLPTR;
}

void DeallocateCcb( CCB*& Ccb )
{
    if( Ccb == NULLPTR )
        return;

    if( Ccb->LowerFileHandle != INVALID_HANDLE_VALUE )
        FltClose( Ccb->LowerFileHandle );
    Ccb->LowerFileHandle = INVALID_HANDLE_VALUE;

    if( Ccb->LowerFileObject != NULLPTR )
        ObDereferenceObject( Ccb->LowerFileObject );
    Ccb->LowerFileObject = NULLPTR;

    DeallocateBuffer( &Ccb->ProcessFileFullPath );
    DeallocateBuffer( &Ccb->SrcFileFullPath );

    ExFreeToNPagedLookasideList( &GlobalContext.CcbLookasideList, Ccb );
    Ccb = NULLPTR;
}

NTSTATUS InitializeFcbAndCcb( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        if( IrpContext == NULLPTR )
            break;

        IrpContext->Fcb = AllocateFcb();
        IrpContext->Ccb = AllocateCcb();

        if( IrpContext->Fcb == NULLPTR || IrpContext->Ccb == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        auto Fcb = IrpContext->Fcb;
        auto Ccb = IrpContext->Ccb;

        ExInitializeFastMutex( &Fcb->FastMutex );
        FsRtlSetupAdvancedHeader( &Fcb->AdvFcbHeader, &Fcb->FastMutex );

        Fcb->NodeTag = FCB_NODE_TYPE_TAG;
        Fcb->NodeSize = sizeof( FCB );

        ExInitializeResourceLite( &( Fcb->MainResource ) );
        ExInitializeResourceLite( &( Fcb->PagingIoResource ) );

        FltInitializeFileLock( &Fcb->FileLock );
        FsRtlInitializeOplock( &Fcb->FileOplock );

        InterlockedIncrement( &Fcb->OpnCount );
        InterlockedIncrement( &Fcb->ClnCount );
        InterlockedIncrement( &Fcb->RefCount );

        /// AdvFcbHeader 설정

        Fcb->AdvFcbHeader.NodeTypeCode = Fcb->NodeTag;
        Fcb->AdvFcbHeader.NodeByteSize = Fcb->NodeSize;

        // TODO: 향후 Fast IO 를 지원할 때 변경한다
        Fcb->AdvFcbHeader.IsFastIoPossible = FastIoIsQuestionable;

        Fcb->AdvFcbHeader.Resource = &Fcb->MainResource;
        Fcb->AdvFcbHeader.PagingIoResource = &Fcb->PagingIoResource;

        const auto Args = ( CREATE_ARGS* )IrpContext->Params;

        if( Args->MetaDataInfo.MetaData.Type == METADATA_UNK_TYPE)
        {
            Fcb->AdvFcbHeader.FileSize = Args->FileSize;
            Fcb->AdvFcbHeader.ValidDataLength = Args->FileSize;
            Fcb->AdvFcbHeader.AllocationSize = Args->FileAllocationSize;
        }
        else
        {
            // 파일에 메타데이터가 설정되어있다면 메타데이터에 기록된 파일크기로 FCB 를 초기화한다

            Fcb->AdvFcbHeader.FileSize.QuadPart = Args->MetaDataInfo.MetaData.ContentSize;
            Fcb->AdvFcbHeader.ValidDataLength = Fcb->AdvFcbHeader.FileSize;
            Fcb->AdvFcbHeader.AllocationSize.QuadPart = ROUND_TO_SIZE( Args->FileSize.QuadPart, IrpContext->InstanceContext->ClusterSize );
        }

        ///////////////////////////////////////////////////////////////////////

        Fcb->InstanceContext = IrpContext->InstanceContext;
        FltReferenceContext( Fcb->InstanceContext );

        /*! In IRP_MJ_CREATE

            1. Already Exists File!!
                1. this file type3
                2. this file not type3
            2. Not Exists yet!!
                1. created file with type3
                2. created file without type3 
        */

        if( IrpContext->ProcessFilter == NULLPTR )
        {
            // not visible Pretend File

            if( FlagOn( Args->FileStatus, FILE_ALREADY_EXISTS ) )
            {
                Fcb->FileFullPath = CloneBuffer( &IrpContext->SrcFileFullPath );
                Fcb->FileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->FileFullPath );
                Fcb->FileName = nsUtils::ReverseFindW( Fcb->FileFullPath.Buffer, L'\\' );

                if( Args->MetaDataInfo.MetaData.Type == METADATA_STB_TYPE )
                {
                    Fcb->PretendFileFullPath = CloneBuffer( &IrpContext->SrcFileFullPath );

                    auto Suffix = nsUtils::EndsWithW( Fcb->PretendFileFullPath.Buffer, Args->MetaDataInfo.MetaData.ContainorSuffix );
                    while( Suffix != NULLPTR && *Suffix != L'\0' )
                        *Suffix++ = L'\0';

                    Fcb->PretendFileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->PretendFileFullPath );
                    Fcb->FileName = nsUtils::ReverseFindW( Fcb->PretendFileFullPath.Buffer, L'\\' );
                }
            }
            else
            {
                if( Args->IsStubCodeOnCreate == FALSE )
                {
                    Fcb->FileFullPath = CloneBuffer( &IrpContext->SrcFileFullPath );
                    Fcb->FileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->FileFullPath );
                    Fcb->FileName = nsUtils::ReverseFindW( Fcb->FileFullPath.Buffer, L'\\' );

                    // NOTE: This file is not MetaData-Type3!!
                }
                else
                {
                    Fcb->FileFullPath = CloneBuffer( &Args->CreateFileName );
                    Fcb->FileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->FileFullPath );
                    Fcb->FileName = nsUtils::ReverseFindW( Fcb->FileFullPath.Buffer, L'\\' );

                    Fcb->PretendFileFullPath = CloneBuffer( &Args->CreateFileName );

                    auto Suffix = nsUtils::ReverseFindW( Fcb->PretendFileFullPath.Buffer, L'.' );
                    while( Suffix != NULLPTR && *Suffix != L'\0' )
                        *Suffix++ = L'\0';

                    Fcb->PretendFileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->PretendFileFullPath );
                    Fcb->FileName = nsUtils::ReverseFindW( Fcb->PretendFileFullPath.Buffer, L'\\' );
                }
            }  // if FILE_ALREADY_EXISTS
        }
        else
        {
            if( FlagOn( Args->FileStatus, FILE_ALREADY_EXISTS ) )
            {
                // not visible Real File

                if( Args->MetaDataInfo.MetaData.Type != METADATA_STB_TYPE )
                {
                    Fcb->FileFullPath = CloneBuffer( &IrpContext->SrcFileFullPath );
                    Fcb->FileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->FileFullPath );
                    Fcb->FileName = nsUtils::ReverseFindW( Fcb->FileFullPath.Buffer, L'\\' );
                }
                else
                {
                    Fcb->PretendFileFullPath = CloneBuffer( &IrpContext->SrcFileFullPath );
                    Fcb->PretendFileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->PretendFileFullPath );
                    Fcb->PretendFileName = nsUtils::ReverseFindW( Fcb->PretendFileFullPath.Buffer, L'\\' );

                    Fcb->FileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, Fcb->PretendFileFullPath.BufferSize + ( CONTAINOR_SUFFIX_MAX * sizeof(WCHAR) ) );
                    RtlCopyMemory( Fcb->FileFullPath.Buffer, Fcb->PretendFileFullPath.Buffer, Fcb->PretendFileFullPath.BufferSize );
                    RtlStringCbCatW( Fcb->FileFullPath.Buffer, Fcb->FileFullPath.BufferSize, Args->MetaDataInfo.MetaData.ContainorSuffix );
                    Fcb->FileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->FileFullPath );
                    Fcb->FileName = nsUtils::ReverseFindW( Fcb->FileFullPath.Buffer, L'\\' );
                }
            }
            else
            {
                if( Args->IsStubCodeOnCreate == FALSE )
                {
                    Fcb->FileFullPath = CloneBuffer( &IrpContext->SrcFileFullPath );
                    Fcb->FileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->FileFullPath );
                    Fcb->FileName = nsUtils::ReverseFindW( Fcb->FileFullPath.Buffer, L'\\' );

                    // NOTE: This file is not MetaData-Type3!!
                }
                else
                {
                    Fcb->FileFullPath = CloneBuffer( &Args->CreateFileName );
                    Fcb->FileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->FileFullPath );
                    Fcb->FileName = nsUtils::ReverseFindW( Fcb->FileFullPath.Buffer, L'\\' );

                    Fcb->PretendFileFullPath = CloneBuffer( &Args->CreateFileName );

                    auto Suffix = nsUtils::ReverseFindW( Fcb->PretendFileFullPath.Buffer, L'.' );
                    while( Suffix != NULLPTR && *Suffix != L'\0' )
                        *Suffix++ = L'\0';

                    Fcb->PretendFileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &Fcb->PretendFileFullPath );
                    Fcb->FileName = nsUtils::ReverseFindW( Fcb->PretendFileFullPath.Buffer, L'\\' );
                }
            } // if FILE_ALREADY_EXISTS

            if( ( Args->MetaDataInfo.MetaData.Type == METADATA_STB_TYPE || Args->IsStubCodeOnCreate != FALSE ) && Fcb->PretendFileFullPathWOVolume == NULLPTR )
                ASSERT( false );
        }

        ///////////////////////////////////////////////////////////////////////

        Ccb->ProcessId = IrpContext->ProcessId;
        Ccb->ProcessFileFullPath = CloneBuffer( &IrpContext->ProcessFullPath );
        Ccb->ProcessName = nsUtils::ReverseFindW( Ccb->ProcessFileFullPath.Buffer, L'\\' );
        
        Ccb->SrcFileFullPath = CloneBuffer( &IrpContext->SrcFileFullPath );

        Status = STATUS_SUCCESS;
    } while( false );

    if( !NT_SUCCESS( Status ) )
    {
        DeallocateFcb( IrpContext->Fcb );
        DeallocateCcb( IrpContext->Ccb );
    }

    return Status;
}

NTSTATUS UninitializeFCB( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        ASSERT( IrpContext != NULLPTR );
        if( IrpContext == NULLPTR )
            break;

        auto Fcb = IrpContext->Fcb;
        ASSERT( Fcb != NULLPTR );
        if( Fcb == NULLPTR )
            break;

        if( Fcb->MetaDataInfo != NULLPTR )
        {
            UninitializeMetaDataInfo( Fcb->MetaDataInfo );
        }

        if( Fcb->SolutionMetaData != NULLPTR && Fcb->SolutionMetaDataSize > 0 )
            ExFreePool( Fcb->SolutionMetaData );

        if( BooleanFlagOn( Fcb->Flags, FCB_STATE_DELETE_ON_CLOSE ) )
        {
            FILE_DISPOSITION_INFORMATION fdi;
            fdi.DeleteFile = TRUE;

            Status = FltSetInformationFile( Fcb->InstanceContext->Instance,
                                            Fcb->LowerFileObject,
                                            &fdi, sizeof( fdi ),
                                            ( FILE_INFORMATION_CLASS )nsW32API::FileDispositionInformation );
            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Status=0x%08x,%s Src=%ws\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltSetInformationFile FAILED"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , Fcb->FileFullPath.Buffer ) );
            }
        }

        FltReleaseContext( Fcb->InstanceContext );

        FltClose( Fcb->LowerFileHandle );
        Fcb->LowerFileHandle = NULLPTR;
        ObDereferenceObject( Fcb->LowerFileObject );
        Fcb->LowerFileObject = NULLPTR;

        ///////////////////////////////////////////////////////////////////////

        ExDeleteResourceLite( &Fcb->MainResource );
        ExDeleteResourceLite( &Fcb->PagingIoResource );

        FltUninitializeOplock( &Fcb->FileOplock );
        FltUninitializeFileLock( &Fcb->FileLock );

        FsRtlTeardownPerStreamContexts( &Fcb->AdvFcbHeader );

        ///////////////////////////////////////////////////////////////////////

        KdPrint( ( "[WinIOSol] << EvtID=%09d %s Src=%ws\n"
                   , IrpContext->EvtID, __FUNCTION__
                   , Fcb->FileFullPath.Buffer ) );

        DeallocateBuffer( &Fcb->FileFullPath );
        DeallocateBuffer( &Fcb->PretendFileFullPath );

        Status = STATUS_SUCCESS;
    } while( false );

    return Status;
}

FCB* Vcb_SearchFCB( __in IRP_CONTEXT* IrpContext, const WCHAR* wszFileName )
{
    FCB*            Fcb = NULLPTR;
    PLIST_ENTRY     ListHead = &IrpContext->InstanceContext->FcbListHead;
    PLIST_ENTRY     Current = NULLPTR;

    for( Current = ListHead->Flink; Current != ListHead; Current = Current->Flink )
    {
        auto item = CONTAINING_RECORD( Current, FCB, ListEntry );

        if( IrpContext->ProcessFilter == NULLPTR )
        {
            if( _wcsicmp( wszFileName, item->FileFullPathWOVolume ) != 0 )
                continue;
        }
        else
        {
            if( ( item->MetaDataInfo == NULLPTR ) ||
                ( item->MetaDataInfo != NULLPTR && item->MetaDataInfo->MetaData.Type != METADATA_STB_TYPE ) )
            {
                if( _wcsicmp( wszFileName, item->FileFullPathWOVolume ) != 0 )
                    continue;
            }
            else
            {
                if( _wcsicmp( wszFileName, item->PretendFileFullPathWOVolume ) != 0 )
                    continue;
            }
        }

        Fcb = item;
        break;
    }

    return Fcb;
}

void Vcb_InsertFCB( CTX_INSTANCE_CONTEXT* InstanceContext, FCB* Fcb )
{
    InsertTailList( &InstanceContext->FcbListHead, &Fcb->ListEntry );
}

void Vcb_DeleteFCB( CTX_INSTANCE_CONTEXT* InstanceContext, FCB* Fcb )
{
    UNREFERENCED_PARAMETER( InstanceContext );
    RemoveEntryList( &Fcb->ListEntry );
}

bool IsOwnFileObject( FILE_OBJECT* FileObject )
{
    if( FileObject == NULLPTR )
        return false;

    if( FileObject->FsContext == NULLPTR )
        return false;

    auto Fcb = ( PFCB )FileObject->FsContext;

    if( Fcb->NodeTag != FCB_NODE_TYPE_TAG ||
        Fcb->NodeSize != sizeof( FCB ) )
        return false;

    return true;
}

bool IsOwnFile( IRP_CONTEXT* IrpContext, const WCHAR* FileFullPathWOVolume, METADATA_DRIVER* MetaDataInfo )
{
    NTSTATUS Status = STATUS_SUCCESS;
    bool IsOwnFileByFilter = false;
    HANDLE FileHandle = NULL;
    FILE_OBJECT* FileObject = NULLPTR;

    AcquireCmnResource( IrpContext, INST_SHARED );

    do
    {
        // TODO: 향후 캐시시스템을 구축하여 락을 최소화하여야한다

        ASSERT( IrpContext != NULLPTR && FileFullPathWOVolume != NULLPTR );
        if( IrpContext == NULLPTR || FileFullPathWOVolume == NULLPTR )
            break;

        if( ARGUMENT_PRESENT( MetaDataInfo ) )
            RtlZeroMemory( MetaDataInfo, METADATA_DRIVER_SIZE );

        auto Fcb = Vcb_SearchFCB( IrpContext, FileFullPathWOVolume );

        if( Fcb != NULLPTR )
        {
            if( ARGUMENT_PRESENT( MetaDataInfo ) && Fcb->MetaDataInfo != NULLPTR )
                RtlCopyMemory( MetaDataInfo, Fcb->MetaDataInfo, METADATA_DRIVER_SIZE );

            IsOwnFileByFilter = true;
            break;
        }

        ReleaseCmnResource( IrpContext, INST_SHARED );

        IO_STATUS_BLOCK IoStatus;

        Status = FltCreateFileOwn( IrpContext, FileFullPathWOVolume, &FileHandle, &FileObject, &IoStatus );

        if( !NT_SUCCESS( Status ) || IoStatus.Information != FILE_OPENED )
        {
            if( Status != STATUS_FILE_IS_A_DIRECTORY )
            {
                KdPrint( ( "[WinIOSol] %s EvtID=%09d %s Status=0x%08x,%s Src=%ws\n"
                           , ">>", IrpContext->EvtID, __FUNCTION__
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , FileFullPathWOVolume
                           ) );
            }
            break;
        }

        if( GetFileMetaDataInfo( IrpContext, FileObject, MetaDataInfo ) != METADATA_UNK_TYPE )
        {
            IsOwnFileByFilter = true;
            break;
        }

    } while( false );

    if( FileHandle != NULL )
        FltClose( FileHandle );

    if( FileObject != NULLPTR )
        ObDereferenceObject( FileObject );

    ReleaseCmnResource( IrpContext, INST_SHARED );
    return IsOwnFileByFilter;
}
