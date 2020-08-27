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

    ULONG                           ProcessId;
    TyGenericBuffer< WCHAR >        ProcessFullPath;
    WCHAR*                          ProcessFileName;        // This just point to ProcessFullPath, dont free this buffer!
    TyGenericBuffer< WCHAR >        SrcFileFullPath;
    TyGenericBuffer< WCHAR >        DstFileFullPath;

    bool isSendTo;
    bool isControl;

    CTX_STREAM_CONTEXT*             StreamContext;

} IRP_CONTEXT, *PIRP_CONTEXT;

PIRP_CONTEXT CreateIrpContext( __in PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects );
void CloseIrpContext( __in PIRP_CONTEXT& IrpContext );

void PrintIrpContext( __in PIRP_CONTEXT IrpContext );

#endif // HDR_WINIOMONITOR_IRP_CONTEXT