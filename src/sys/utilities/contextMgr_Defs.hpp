#ifndef HDR_UTIL_CONTEXTMGR_DEFS
#define HDR_UTIL_CONTEXTMGR_DEFS

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _CTX_GLOBAL_DATA
{
    PDRIVER_OBJECT                  DriverObject;
    PDEVICE_OBJECT                  DeviceObject;       // Control Device Object
    PFLT_FILTER                     Filter;

    PFLT_PORT                       ServerPort;
    PFLT_PORT                       ClientPort;

    ULONG                           DebugLevel;

    NPAGED_LOOKASIDE_LIST           FileNameLookasideList;
    NPAGED_LOOKASIDE_LIST           ProcNameLookasideList;
    NPAGED_LOOKASIDE_LIST           SendPacketLookasideList;
    NPAGED_LOOKASIDE_LIST           ReplyPacketLookasideList;

} CTX_GLOBAL_DATA, * PCTX_GLOBAL_DATA;

extern CTX_GLOBAL_DATA GlobalContext;

#endif // HDR_UTIL_CONTEXTMGR_DEFS