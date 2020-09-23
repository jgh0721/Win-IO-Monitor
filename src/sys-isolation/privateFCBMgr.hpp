#ifndef HDR_PRIVATE_FCB_MGR
#define HDR_PRIVATE_FCB_MGR

#include "fltBase.hpp"
#include "privateFCBMgr_Defs.hpp"
#include "irpContext_Defs.hpp"

#include "utilities/contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FCB* AllocateFcb();
CCB* AllocateCcb();
VOID DeallocateFcb( FCB*& Fcb );
VOID DeallocateCcb( CCB*& Ccb );

NTSTATUS InitializeFcbAndCcb( __in IRP_CONTEXT* IrpContext );
NTSTATUS UninitializeFCB( __in IRP_CONTEXT* IrpContext );

// VCB = Volume Control Block,
// 격리필터는 자신이 파일시스템인 것 처럼 작동하기 때문에, 마치 파일시스템의 VCB 에서 검색하는 것과 같다
// 아래 세 함수는 모두 InstanceContext 의 락을 획득하고 호출해야한다
FCB* Vcb_SearchFCB( __in CTX_INSTANCE_CONTEXT* InstanceContext, __in const WCHAR* wszFileName );
VOID Vcb_InsertFCB( __in CTX_INSTANCE_CONTEXT* InstanceContext, __in FCB* Fcb );
VOID Vcb_DeleteFCB( __in CTX_INSTANCE_CONTEXT* InstanceContext, __in FCB* Fcb );

bool IsOwnFileObject( __in FILE_OBJECT* FileObject );

#endif // HDR_PRIVATE_FCB_MGR