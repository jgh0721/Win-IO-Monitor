#include "privateFCBMgr.hpp"

#include "utilities/bufferMgr.hpp"
#include "fltCmnLibs.hpp"
#include "callbacks/fltCreateFile.hpp"

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
        Fcb->AdvFcbHeader.IsFastIoPossible = FastIoIsNotPossible;

        Fcb->AdvFcbHeader.Resource = &Fcb->MainResource;
        Fcb->AdvFcbHeader.PagingIoResource = &Fcb->PagingIoResource;

        Fcb->AdvFcbHeader.AllocationSize = ( ( CREATE_ARGS* )IrpContext->Params )->FileAllocationSize;
        Fcb->AdvFcbHeader.FileSize = ( ( CREATE_ARGS* )IrpContext->Params )->FileSize;
        Fcb->AdvFcbHeader.ValidDataLength = ( ( CREATE_ARGS* )IrpContext->Params )->FileSize;

        ///////////////////////////////////////////////////////////////////////

        Fcb->FileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, IrpContext->SrcFileFullPath.BufferSize );
        RtlCopyMemory( Fcb->FileFullPath.Buffer, IrpContext->SrcFileFullPath.Buffer, Fcb->FileFullPath.BufferSize );
        Fcb->FileName = nsUtils::ReverseFindW( Fcb->FileFullPath.Buffer, L'\\' );

        if( Fcb->FileFullPath.Buffer != NULLPTR )
        {
            if( Fcb->FileFullPath.Buffer[ 1 ] == L':' )
                Fcb->FileFullPathWOVolume = &Fcb->FileFullPath.Buffer[ 2 ];
            else
            {
                auto cchDeviceName = nsUtils::strlength( IrpContext->InstanceContext->DeviceNameBuffer );
                if( cchDeviceName > 0 )
                    Fcb->FileFullPathWOVolume = &Fcb->FileFullPath.Buffer[ cchDeviceName ];
            }

            if( Fcb->FileFullPathWOVolume == NULLPTR )
                Fcb->FileFullPathWOVolume = Fcb->FileFullPath.Buffer;
        }

        Fcb->InstanceContext = IrpContext->InstanceContext;
        FltReferenceContext( Fcb->InstanceContext );

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

        if( BooleanFlagOn( Fcb->Flags, FILE_DELETE_ON_CLOSE ) )
        {
            FILE_DISPOSITION_INFORMATION fdi;
            fdi.DeleteFile = TRUE;

            FltSetInformationFile( Fcb->InstanceContext->Instance,
                                   Fcb->LowerFileObject,
                                   &fdi, sizeof( fdi ),
                                   (FILE_INFORMATION_CLASS)nsW32API::FileDispositionInformation );
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

        KdPrint( ( "[WinIOSol] EvtID=%09d %s Src=%ws\n"
                   , IrpContext->EvtID, __FUNCTION__
                   , Fcb->FileFullPath.Buffer ) );

        DeallocateBuffer( &Fcb->FileFullPath );

        Status = STATUS_SUCCESS;
    } while( false );

    return Status;
}

FCB* Vcb_SearchFCB( CTX_INSTANCE_CONTEXT* InstanceContext, const WCHAR* wszFileName )
{
    FCB*            Fcb = NULLPTR;
    PLIST_ENTRY     ListHead = &InstanceContext->FcbListHead;
    PLIST_ENTRY     Current = NULLPTR;

    for( Current = ListHead->Flink; Current != ListHead; Current = Current->Flink )
    {
        auto item = CONTAINING_RECORD( Current, FCB, ListEntry );

        if( _wcsicmp( wszFileName, item->FileFullPath.Buffer ) != 0 )
            continue;

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
