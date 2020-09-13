#include "cacheManagerCallbacks.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

BOOLEAN CcAcquireForLazyWrite( PVOID Context, BOOLEAN Wait )
{
    return FALSE;
}

void CcReleaseFromLazyWrite( PVOID Context )
{
}

BOOLEAN CcAcquireForReadAhead( PVOID Context, BOOLEAN Wait )
{
    return FALSE;
}

void CcReleaseFromReadAhead( PVOID Context )
{
}
