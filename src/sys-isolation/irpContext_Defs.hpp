#ifndef HDR_ISOLATION_IRPCONTEXT_DEFS
#define HDR_ISOLATION_IRPCONTEXT_DEFS

#include "fltBase.hpp"

#include "privateFCBMgr_Defs.hpp"
#include "policies/ProcessFilter.hpp"
#include "utilities/bufferMgr_Defs.hpp"
#include "utilities/contextMgr_Defs.hpp"

#include "WinIOIsolation_Event.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

extern LONG volatile GlobalEvtID;
static const int DEBUG_TEXT_SIZE = 1024;

enum TyEnCompleteState
{
    COMPLETE_FREE_MAIN_RSRC             = 0x1,      // Fcb 의 MainResource 락 해제
    COMPLETE_FREE_INST_RSRC             = 0x2,      // InstanceContext 의 VcbLock 해제
    COMPLETE_FREE_PGIO_RSRC             = 0x4,      // Fcb 의 PagingIoResource 락 해제
                                                    // 만약 MainResource 와 PagingIoResource 가 동시에 있다면 PagingIoResource -> MainResource 순서로 해제한다
    COMPLETE_FREE_LOWER_FILEOBJECT      = 0x10,     // 해당 객체가 디렉토리이거나, 이미 FCB 에 객체가 설정되었거나 등의 이유로 Lower 객체가 필요하지 않음
                                                    // FCB 를 새로 할당해야함
    COMPLETE_FREE_PARAM                 = 0x20,     // IrpContext 내의 Params 변수에 대한 메모리를 해제해야함( ExFreePool, 사용 )
    COMPLETE_CRTE_INIT_FCB              = 0x100,    // FCB 를 초기화 해야함
    COMPLETE_CRTE_STREAM_CONTEXT        = 0x200,    // 해당 파일객체에 대해 StreamContext 를 생성해야한다
                                                    // 만약 해당 파일이 격리대상 파일이라면 Fcb 의 LowerFileObject 에 대해 StreamContext 를 생성한다

    COMPLETE_DONT_CONT_PROCESS          = 0x1000,   // 외부 함수 수행 후에 즉시 종료
    COMPLETE_IOSTATUS_STATUS            = 0x2000,   // Data->IoStatus.Status 에 Status 값을 대입한다       
    COMPLETE_IOSTATUS_INFORMATION       = 0x4000,   // Data->IoStatus.Information 에 Information 값을 대입한다
    COMPLETE_RETURN_FLTSTATUS           = 0x8000,   // PreFltStatus 값을 IRP 처리 결과로 반환한다

    COMPLETE_INTERNAL_ERROR             = 0x10000,  // 내부 오류로 인해 처리 불가
    COMPLETE_BYPASS_REQUEST             = 0x20000,  // FileSystem 으로 처리를 전달
    COMPLETE_FORWARD_POST_PROCESS       = 0x40000,  // Pre- 핸들러에서 Post- 핸들러로 처리를 넘김
                                                    // IrpContext 를 CompletionContext 로 넘기며, Post- 핸들러에서 IrpContext 를 정리한다
};

typedef struct _IRP_CONTEXT
{
    LONG                                EvtID;
    CHAR*                               DebugText;

    PFLT_CALLBACK_DATA                  Data;
    PCFLT_RELATED_OBJECTS               FltObjects;
    bool                                IsOwnObject;            // IRP_MJ_CREATE 를 제외하고, 해당 값이 TRUE 일 때만 Fcb 와 Ccb 가 유효하다

    CTX_STREAM_CONTEXT*                 StreamContext;
    CTX_INSTANCE_CONTEXT*               InstanceContext;        // 반드시 CtxReleaseContext 를 호출해야한다

    FCB*                                Fcb;
    CCB*                                Ccb;
    PVOID                               UserBuffer;

    ULONG                               ProcessId;
    TyGenericBuffer< WCHAR >            ProcessFullPath;
    WCHAR*                              ProcessFileName;        // This just point to ProcessFullPath, dont free this buffer!

    TyGenericBuffer< WCHAR >            SrcFileFullPath;
    WCHAR*                              SrcFileFullPathWOVolume;    // SrcFileFullPath 에서 드라이브 문자 또는 디바이스 이름등을 제외한 순수한 경로 및 이름( \ 로 시작한다 )
    WCHAR*                              SrcFileName;                // SrcFileFullPath 에서 경로 부분을 제외한 파일이름 + 확장자
    WCHAR*                              SrcFileFullExtension;       // SrcFileName 에서 첫번째 . 부터 
    WCHAR*                              SrcFileExtension;           // SrcFileName 에서 마지막 . 부터
    TyGenericBuffer< WCHAR >            DstFileFullPath;
    WCHAR*                              DstFileFullPathWOVolume;
    WCHAR*                              DstFileName;                // DstFileFullPath 에서 경로 부분을 제외한 파일이름 + 확장자 
    WCHAR*                              DstFileFullExtension;
    WCHAR*                              DstFileExtension;

    bool                                IsConcerned;

    PVOID                               Params;                 // 각 IRP 마다 사용하는 고유의 입력 / 출력 변수 구조체에 대한 포인터
    TyGenericBuffer<MSG_REPLY_PACKET>   Result;

    ///////////////////////////////////////////////////////////////////////////
    /// Global Filter && Process Filter

    BOOLEAN                             IsMatchedGlobalExclude;
    BOOLEAN                             IsMatchedGlobalInclude;

    HANDLE                              ProcessFilter;
    PROCESS_FILTER_ENTRY*               ProcessFilterEntry;
    PROCESS_FILTER_MASK_ENTRY*          ProcessFilterMaskItem;

    ///////////////////////////////////////////////////////////////////////////
    /// IRP 를 처리한 결과

    NTSTATUS                            Status;
    ULONG_PTR                           Information;
    FLT_PREOP_CALLBACK_STATUS           PreFltStatus;

    // TyEnCompleteState 의 조합
    // CloseIrpContext 에서 해당 값의 조합들을 이용하여 후처리를 진행한다
    ULONG                               CompleteStatus;

} IRP_CONTEXT, *PIRP_CONTEXT;

#endif // HDR_ISOLATION_IRPCONTEXT_DEFS