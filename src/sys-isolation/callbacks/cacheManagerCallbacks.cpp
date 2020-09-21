#include "cacheManagerCallbacks.hpp"

#include "privateFCBMgr.hpp"
#include "privateFCBMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

BOOLEAN CcAcquireForLazyWrite( PVOID Context, BOOLEAN Wait )
{
    auto Fcb = ( FCB* )Context;
    UNREFERENCED_PARAMETER( Wait );

    return FALSE;
}

void CcReleaseFromLazyWrite( PVOID Context )
{
}

BOOLEAN CcAcquireForReadAhead( PVOID Context, BOOLEAN Wait )
{
    auto Fcb = ( FCB* )Context;
    UNREFERENCED_PARAMETER( Wait );

    FltAcquireResourceShared( &Fcb->MainResource );

    KdPrint( ( "[WinIOSol] %s Open=%d Clean=%d Ref=%d Name=%ws\n"
               , __FUNCTION__
               , Fcb->OpnCount, Fcb->ClnCount, Fcb->RefCount
               , Fcb->FileFullPath.Buffer ) );

    return TRUE;
}

void CcReleaseFromReadAhead( PVOID Context )
{
    auto Fcb = ( FCB* )Context;

    FltReleaseResource( &Fcb->MainResource );

    KdPrint( ( "[WinIOSol] %s Open=%d Clean=%d Ref=%d Name=%ws\n"
               , __FUNCTION__
               , Fcb->OpnCount, Fcb->ClnCount, Fcb->RefCount
               , Fcb->FileFullPath.Buffer ) );
}
