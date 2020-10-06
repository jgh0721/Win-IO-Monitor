#ifndef HDR_DRIVER_UTILITIES
#define HDR_DRIVER_UTILITIES

#include "fltBase.hpp"

#include "privateFCBMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FAST_IO_POSSIBLE CheckIsFastIOPossible( __in FCB* Fcb );
PVOID FltMapUserBuffer( __in PFLT_CALLBACK_DATA Data );

#endif // HDR_DRIVER_UTILITIES
