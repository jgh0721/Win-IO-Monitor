#ifndef HDR_DEVICE_MGMT
#define HDR_DEVICE_MGMT

#include "fltBase.hpp"
#include "utilities/contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS CreateControlDevice( __in CTX_GLOBAL_DATA& GlobalContext );
NTSTATUS RemoveControlDevice( __in CTX_GLOBAL_DATA& GlobalContext );

///////////////////////////////////////////////////////////////////////////////

#endif // HDR_DEVICE_MGMT