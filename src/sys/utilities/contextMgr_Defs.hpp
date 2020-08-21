#ifndef HDR_UTIL_CONTEXTMGR_DEFS
#define HDR_UTIL_CONTEXTMGR_DEFS

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _CTX_GLOBAL_DATA
{
    PDRIVER_OBJECT                  DriverObject;
    PFLT_FILTER                     Filter;

    ULONG                           DebugLevel;

} CTX_GLOBAL_DATA, * PCTX_GLOBAL_DATA;

#endif // HDR_UTIL_CONTEXTMGR_DEFS