#ifndef HDR_ISOLATION_IRPCONTEXT
#define HDR_ISOLATION_IRPCONTEXT

#include "fltBase.hpp"
#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

LONG            CreateEvtID();

PIRP_CONTEXT    CreateIrpContext( __in PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects );
VOID            CloseIrpContext( __in PIRP_CONTEXT IrpContext );
VOID            PrintIrpContext( __in PIRP_CONTEXT IrpContext, __in bool isForceResult = false );
VOID            PrintIrpContextEx( __in PIRP_CONTEXT IrpContext, __in bool isForceResult = false );

void            PrintIrpContextCREATE( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextREAD( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextWRITE( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextQUERY_INFORMATION( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
void            PrintIrpContextSET_INFORMATION( __in PIRP_CONTEXT IrpContext, __in bool IsResultMode = false );
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

#endif // HDR_ISOLATION_IRPCONTEXT