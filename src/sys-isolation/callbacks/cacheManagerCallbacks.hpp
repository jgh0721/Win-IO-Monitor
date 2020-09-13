#ifndef HDR_ISOLATION_CACHEMGR_CALLBACKS
#define HDR_ISOLATION_CACHEMGR_CALLBACKS

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

BOOLEAN CcAcquireForLazyWrite( IN PVOID Context, IN BOOLEAN Wait );
VOID CcReleaseFromLazyWrite( IN PVOID Context );
BOOLEAN CcAcquireForReadAhead( IN PVOID Context, IN BOOLEAN Wait );
VOID CcReleaseFromReadAhead( IN PVOID Context );

EXTERN_C_END

#endif // HDR_ISOLATION_CACHEMGR_CALLBACKS