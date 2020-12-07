#ifndef HDR_ISOLATION_CACHEMGR_CALLBACKS
#define HDR_ISOLATION_CACHEMGR_CALLBACKS

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*!
    캐시관리자에 의해 시스템 스레드에서 호출된다

    해당 콜백들은 캐시관리자에서 스트림을 읽고/쓰기 전에 락 획득/해제를 위해 호출하는 콜백이다 
*/

EXTERN_C_BEGIN

BOOLEAN CcAcquireForLazyWrite( IN PVOID Context, IN BOOLEAN Wait );
VOID CcReleaseFromLazyWrite( IN PVOID Context );
BOOLEAN CcAcquireForReadAhead( IN PVOID Context, IN BOOLEAN Wait );
VOID CcReleaseFromReadAhead( IN PVOID Context );

EXTERN_C_END

#endif // HDR_ISOLATION_CACHEMGR_CALLBACKS