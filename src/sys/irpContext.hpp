#ifndef HDR_WINIOMONITOR_IRP_CONTEXT
#define HDR_WINIOMONITOR_IRP_CONTEXT

#include "fltBase.hpp"
#include "utilities/bufferMgr_Defs.hpp"
#include "utilities/contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

extern LONG volatile GlobalEvtID;

LONG CreateEvtID();

///////////////////////////////////////////////////////////////////////////////

typedef struct _IRP_CONTEXT
{
    PFLT_CALLBACK_DATA              Data;
    PCFLT_RELATED_OBJECTS           FltObjects;

    LONG                            EvtID;
    ULONG                           MsgType;

    ULONG                           ProcessId;
    TyGenericBuffer< WCHAR >        ProcessFullPath;
    WCHAR*                          ProcessFileName;        // This just point to ProcessFullPath, dont free this buffer!
    TyGenericBuffer< WCHAR >        SrcFileFullPath;
    TyGenericBuffer< WCHAR >        DstFileFullPath;

    bool                            isSendTo;
    bool                            isControl;

    CTX_STREAM_CONTEXT*             StreamContext;

    PVOID                           ProcessFilterHandle;    // dont use direcetly this pointer, just refcount 
    PVOID                           ProcessFilterEntry;

} IRP_CONTEXT, *PIRP_CONTEXT;

PIRP_CONTEXT CreateIrpContext( __in PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects );
void CloseIrpContext( __in PIRP_CONTEXT& IrpContext );

void CheckEvent( __inout IRP_CONTEXT* IrpContext, __in PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in ULONG NotifyEvent );

ULONG ConvertIRPMajorFuncToFSTYPE( __in PFLT_CALLBACK_DATA Data );

///////////////////////////////////////////////////////////////////////////////

void PrintIrpContext( __in const PIRP_CONTEXT IrpContext );

#endif // HDR_WINIOMONITOR_IRP_CONTEXT