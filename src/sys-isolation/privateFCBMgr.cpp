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

NTSTATUS InitializeFCB( FCB* Fcb, __in IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        ASSERT( Fcb != NULLPTR );
        if( Fcb == NULLPTR )
            break;

        RtlZeroMemory( Fcb, sizeof( FCB ) );

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

        Fcb->AdvFcbHeader.AllocationSize    = ( ( CREATE_ARGS* )IrpContext->Params )->FileAllocationSize;
        Fcb->AdvFcbHeader.FileSize          = ( ( CREATE_ARGS* )IrpContext->Params )->FileSize;
        Fcb->AdvFcbHeader.ValidDataLength   = ( ( CREATE_ARGS* )IrpContext->Params )->FileSize;

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

        Status = STATUS_SUCCESS;
    } while( false );

    return Status;
}

NTSTATUS UninitializeFCB( FCB* Fcb )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
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

        KdPrint( ( "[WinIOSol] %s Src=%ws\n",
                   __FUNCTION__, Fcb->FileFullPath.Buffer ) );

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
