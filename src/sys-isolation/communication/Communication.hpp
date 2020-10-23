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
VOID        UninitializeNotifyEventWorker();

///////////////////////////////////////////////////////////////////////////////
/// MSG_FS_TYPE

/*!
    클라이언트에 이벤트를 전송하고 처리 방향을 점검함

    * IRP_MJ_CREATE
        Status = STATUS_SUCCESS, 해당 요청 허용

    * IRP_MJ_SET_INFORMATION

    @return NTSTATUS, 해당 함수의 실행결과, This is not returned NTSTATUS by Client,
                      클라이언트가 반환한 NTSTATUS 가 아님
*/
NTSTATUS    CheckEventFileCreateTo( __in IRP_CONTEXT* IrpContext );
NTSTATUS    CheckEventFileOpenTo( __in IRP_CONTEXT* IrpContext );

///////////////////////////////////////////////////////////////////////////////
/// Utilities

PFLT_PORT   GetClientPort( __in IRP_CONTEXT* IrpContext );

#endif // HDR_ISOLATION_COMMUNICATION