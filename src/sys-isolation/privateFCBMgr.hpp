#ifndef HDR_PRIVATE_FCB_MGR
#define HDR_PRIVATE_FCB_MGR

#include "fltBase.hpp"
#include "privateFCBMgr_Defs.hpp"

#include "utilities/contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS InitializeFCB( __in FCB* Fcb );
NTSTATUS UninitializeFCB( __in FCB* Fcb );

// VCB = Volume Control Block,
// 격리필터는 자신이 파일시스템인 것 처럼 작동하기 때문에, 마치 파일시스템의 VCB 에서 검색하는 것과 같다 
FCB* Vcb_SearchFCB( __in CTX_INSTANCE_CONTEXT* InstanceContext, __in const WCHAR* wszFileName );
VOID Vcb_InsertFCB( __in CTX_INSTANCE_CONTEXT* InstanceContext, __in FCB* Fcb );
VOID Vcb_DeleteFCB( __in CTX_INSTANCE_CONTEXT* InstanceContext, __in FCB* Fcb );

#endif // HDR_PRIVATE_FCB_MGR