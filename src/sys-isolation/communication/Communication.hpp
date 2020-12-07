#ifndef HDR_ISOLATION_COMMUNICATION
#define HDR_ISOLATION_COMMUNICATION

#include "fltBase.hpp"
#include "Communication_Defs.hpp"

#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////
/// MSG_FS_NOTIFY_TYPE

NTSTATUS    InitializeNotifyEventWorker();
NTSTATUS    QueueNotifyEvent( __in FS_NOTIFY_ITEM* NotifyEventItem );
VOID        NotifyEventWorker( __in PVOID Context );
NTSTATUS    NotifyFSEventToClient( __in FS_NOTIFY_ITEM* NotifyEventItem );
NTSTATUS    UninitializeNotifyEventWorker();

///////////////////////////////////////////////////////////////////////////////
/// MSG_FS_TYPE

/*!
    클라이언트에 이벤트를 전송하고 처리 방향을 점검함

    * IRP_MJ_CREATE
        Status = STATUS_SUCCESS, 해당 요청 허용,
                                 이외의 값을 설정하면 해당 값을 파일시스템에 돌려주면 완료처리됨
                                 (예) STATUS_ACCESS_DENIED )

    * IRP_MJ_SET_INFORMATION
        Status = STATUS_SUCCESS, 해당 요청 허용,
                                 이외의 값을 설정하면 해당 값을 파일시스템에 돌려주면 완료처리됨
                                 (예) STATUS_ACCESS_DENIED )

    @return NTSTATUS, 해당 함수의 실행결과, This is not returned NTSTATUS by Client,
                      클라이언트가 반환한 NTSTATUS 가 아님

    ※ 클라이언트가 반환한 응답값은 IrpContext 의 Result 에 기록된다
*/
NTSTATUS    CheckEventFileCreateTo( __in IRP_CONTEXT* IrpContext );
NTSTATUS    CheckEventFileOpenTo( __in IRP_CONTEXT* IrpContext );
NTSTATUS    CheckEventFileCleanup( __in IRP_CONTEXT* IrpContext );
NTSTATUS    CheckEventFileClose( __in IRP_CONTEXT* IrpContext );

///////////////////////////////////////////////////////////////////////////////
/// MSG_FS_NOTIFY_TYPE

NTSTATUS    NotifyEventFileRenameTo( __in IRP_CONTEXT* IrpContext );
NTSTATUS    NotifyEventFileDeleteTo( __in IRP_CONTEXT* IrpContext );

///////////////////////////////////////////////////////////////////////////////
/// MSG_PROC_TYPE

NTSTATUS    CheckEventProcCreateTo( __in ULONG ProcessId, __in ULONG ParentProcessId, __in TyGenericBuffer<WCHAR>* ProcessFileFullPath, __in_z const wchar_t* ProcessFileName, __out TyGenericBuffer<MSG_REPLY_PACKET>* Reply );
NTSTATUS    CheckEventProcTerminateTo( __in ULONG ProcessId, __in ULONG ParentProcessId, __in TyGenericBuffer<WCHAR>* ProcessFileFullPath, __in_z const wchar_t* ProcessFileName );

///////////////////////////////////////////////////////////////////////////////
/// Utilities

PFLT_PORT   GetClientPort( __in IRP_CONTEXT* IrpContext, bool IsProcPort = false );
PFLT_PORT   GetClientPort( __in LONG Seed, bool IsProcPort = false );

#endif // HDR_ISOLATION_COMMUNICATION