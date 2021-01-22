#ifndef HDR_WINIOMONITOR_NAMES
#define HDR_WINIOMONITOR_NAMES

#if defined(USE_ON_KERNEL)
#include "base.hpp"
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define PORT_NAME                           L"\\WinIOMonFltPort"
#define PORT_PROC_NAME                      L"\\WinIOMonProcPort"
#define DEVICE_NAME                         L"\\WinIOMon"

#endif // HDR_WINIOMONITOR_NAMES