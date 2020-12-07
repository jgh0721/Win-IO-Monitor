#ifndef HDR_ISOLATION_IRPCONTEXT
#define HDR_ISOLATION_IRPCONTEXT

#include "fltBase.hpp"
#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

LONG            CreateEvtID();

IRP_CONTEXT*    CreateIrpContext( __in PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects );
VOID            CloseIrpContext( __in PIRP_CONTEXT IrpContext );
VOID            PrintIrpContext( __in PIRP_CONTEXT IrpContext, __in bool isForceResult = false );
VOID            PrintIrpContextEx( __in PIRP_CONTEXT IrpContext, __in bool isForceResult = false );

FORCEINLINE const char* JudgeInOut( __in PIRP_CONTEXT IrpContext, __in_opt bool isForceResult = false )
{
    if( isForceResult == true )
        return "<<";

    if( BooleanFlagOn( IrpContext->Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION ) == FALSE )
        return ">>";

    return "<<";
}

void            PrintIrpContextCREATE( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextREAD( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextWRITE( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextQUERY_INFORMATION( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextSET_INFORMATION( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextQUERY_SECURITY( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextSET_SECURITY( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextDIRECTORY_CONTROL( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextCLEANUP( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextCLOSE( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );

void            PrintIrpContextQUERY_VOLUME_INFORMATION( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextFILE_SYSTEM_CONTROL( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextLOCK_CONTROL( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );

enum TyEnCmnRsrc
{
    FCB_MAIN_EXCLUSIVE          = 0x1,
    FCB_MAIN_SHARED             = 0x2,

    FCB_PGIO_EXCLUSIVE          = 0x4,
    FCB_PGIO_SHARED             = 0x8,

    INST_EXCLUSIVE              = 0x10,
    INST_SHARED                 = 0x20,
};

VOID AcquireCmnResource( __in PIRP_CONTEXT IrpContext, __in LONG RsrcFlags );
VOID ReleaseCmnResource( __in PIRP_CONTEXT IrpContext );
VOID ReleaseCmnResource( __in PIRP_CONTEXT IrpContext, __in LONG RsrcFlags );

#define IF_DONT_CONTINUE_PROCESS_LEAVE( IrpContext ) \
    if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS ) ) \
        __leave;

#define IF_DONT_CONTINUE_PROCESS_BREAK( IrpContext ) \
    if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_DONT_CONT_PROCESS ) ) \
        break;

#define AssignCmnResult( IrpContext, Result ) \
    IrpContext->Status = Result; \
    SetFlag( IrpContext->CompleteStatus, COMPLETE_IOSTATUS_STATUS )

#define AssignCmnResultInfo( IrpContext, Info ) \
    IrpContext->Information = Info; \
    SetFlag( IrpContext->CompleteStatus, COMPLETE_IOSTATUS_INFORMATION )

#define AssignCmnFltResult( IrpContext, FltResult ) \
    IrpContext->PreFltStatus = FltResult; \
    SetFlag( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS )

#define CHECK_EVENT_GLOBAL_DIR_FILTER   0x1
#define CHECK_EVENT_PROCESS_ONLY        0x2
// this include CHECK_EVENT_PROCESS_ONLY
#define CHECK_EVENT_PROCESS_DIR_FILTER  0x4

// 다른 조건과 결합하여 최종 계산
#define CHECK_EVENT_IS_ENCRYPTED        0x10
#define CHECK_EVENT_IS_RENAME           0x20
// 해당 파일이 암호화된 파일이라면 무조건 대상으로 선정
#define CHECK_EVENT_IS_ENCRYPTED_ALWAYS 0x40

/*!
    각 IRP 함수에 따라, 클라이언트에 이벤트를 전송해야하는지 확인

    클라이언트에 이벤트를 전송해야한다면 IrpContext 의 isConcerned 를 true 로 설정한다
*/
NTSTATUS CheckEventTo( __in IRP_CONTEXT* IrpContext );

NTSTATUS CheckEventToWithCREATE( __in IRP_CONTEXT* IrpContext, __in ULONG CheckFlags );
NTSTATUS CheckEventToWithDIRECTORY_CONTROL( __in IRP_CONTEXT* IrpContext, __in ULONG CheckFlags );
NTSTATUS CheckEventToWithQUERY_INFORMATION( __in IRP_CONTEXT* IrpContext, __in ULONG CheckFlags );
NTSTATUS CheckEventToWithSET_INFORMATION( __in IRP_CONTEXT* IrpContext, __in ULONG CheckFlags );
NTSTATUS CheckEventToWithCLEAN( __in IRP_CONTEXT* IrpContext, __in ULONG CheckFlags );

FLT_PREOP_CALLBACK_STATUS CommonPostProcess( __in PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __inout_opt PVOID* CompletionContext,
                                             __in IRP_CONTEXT* IrpContext, __in FLT_PREOP_CALLBACK_STATUS FltStatus );

#endif // HDR_ISOLATION_IRPCONTEXT