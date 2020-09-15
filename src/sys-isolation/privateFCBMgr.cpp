#include "privateFCBMgr.hpp"

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

NTSTATUS InitializeFCB( FCB* Fcb )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        ASSERT( Fcb != NULLPTR );
        if( Fcb == NULLPTR )
            break;

        RtlZeroMemory( Fcb, sizeof( FCB ) );

        FsRtlSetupAdvancedHeader( &Fcb->AdvFcbHeader, &Fcb->FastMutex );
        ExInitializeFastMutex( &Fcb->FastMutex );

        Fcb->NodeTag = FCB_NODE_TYPE_TAG;
        Fcb->NodeSize = sizeof( FCB );

        ExInitializeResourceLite( &( Fcb->MainResource ) );
        ExInitializeResourceLite( &( Fcb->PagingIoResource ) );

        FltInitializeFileLock( &Fcb->FileLock );
        FsRtlInitializeOplock( &Fcb->FileOplock );

        ///////////////////////////////////////////////////////////////////////
        
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
