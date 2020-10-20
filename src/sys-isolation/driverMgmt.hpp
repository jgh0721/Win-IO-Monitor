#ifndef HDR_ISOLATION_DRIVER_MGMT
#define HDR_ISOLATION_DRIVER_MGMT

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _FEATURE_CONTEXT
{
    LONG                IsRunning;
    ULONG               CntlProcessId;      // Connected Process Id

} FEATURE_CONTEXT, *PFEATURE_CONTEXT;

extern FEATURE_CONTEXT FeatureContext;

NTSTATUS InitializeFeatureMgr();
NTSTATUS UninitializeFeatureMgr();

#endif // HDR_ISOLATION_DRIVER_MGMT