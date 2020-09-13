#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define FCB_NODE_TYPE_TAG 'TzFB'

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
