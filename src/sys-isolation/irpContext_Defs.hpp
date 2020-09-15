#ifndef HDR_ISOLATION_IRPCONTEXT_DEFS
#define HDR_ISOLATION_IRPCONTEXT_DEFS

#include "fltBase.hpp"

#include "utilities/bufferMgr_Defs.hpp"
#include "utilities/contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

extern LONG volatile GlobalEvtID;

typedef struct _IRP_CONTEXT
{
    PFLT_CALLBACK_DATA                  Data;
    PCFLT_RELATED_OBJECTS               FltObjects;

    LONG                                EvtID;
    CTX_INSTANCE_CONTEXT*               InstanceContext;        // 반드시 CtxReleaseContext 를 호출해야한다

    ULONG                               ProcessId;
    TyGenericBuffer< WCHAR >            ProcessFullPath;
    WCHAR*                              ProcessFileName;        // This just point to ProcessFullPath, dont free this buffer!

    TyGenericBuffer< WCHAR >            SrcFileFullPath;
    WCHAR*                              SrcFileFullPathWOVolume;    // SrcFileFullPath 에서 드라이브 문자 또는 디바이스 이름등을 제외한 순수한 경로 및 이름( \ 로 시작한다 )
    TyGenericBuffer< WCHAR >            DstFileFullPath;

    bool                                IsAudit;

    PVOID                               Params;                 // 각 IRP 마다 사용하는 고유의 입력 / 출력 변수 구조체에 대한 포인터
    PVOID                               Result;

} IRP_CONTEXT, *PIRP_CONTEXT;

#endif // HDR_ISOLATION_IRPCONTEXT_DEFS