#ifndef HDR_WINIOMONITOR_API
#define HDR_WINIOMONITOR_API

#if defined(USE_ON_KERNEL)
#include "base.hpp"
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif


DWORD SetTimeOutMs( __in ULONG uTimeOutMs );


#endif // HDR_WINIOMONITOR_API