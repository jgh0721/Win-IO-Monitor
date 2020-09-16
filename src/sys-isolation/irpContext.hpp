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
VOID            PrintIrpContext( __in PIRP_CONTEXT IrpContext );

#endif // HDR_ISOLATION_IRPCONTEXT