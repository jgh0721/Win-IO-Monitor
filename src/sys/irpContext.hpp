#ifndef HDR_WINIOMONITOR_IRP_CONTEXT
#define HDR_WINIOMONITOR_IRP_CONTEXT

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _IRP_CONTEXT
{
    bool isSendTo;
    bool isControl;
    
} IRP_CONTEXT, *PIRP_CONTEXT;

#endif // HDR_WINIOMONITOR_IRP_CONTEXT