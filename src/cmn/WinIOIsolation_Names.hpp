#ifndef HDR_WINIOMONITOR_NAMES
#define HDR_WINIOMONITOR_NAMES

#if defined(USE_ON_KERNEL)
#include "fltBase.hpp"
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define PORT_NAME                           L"\\WinIOIsltFltPort"
#define PORT_PROC_NAME                      L"\\WinIOIsltProcFltPort"
#define DEVICE_NAME                         L"\\WinIOIslt"

#endif // HDR_WINIOMONITOR_NAMES