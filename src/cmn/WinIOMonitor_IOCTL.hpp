#ifndef HDR_WINIOMONITOR_IOCTL
#define HDR_WINIOMONITOR_IOCTL

#if defined(USE_ON_KERNEL)
#include "fltBase.hpp"
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define WINIOMONITOR_DEVICE_TYPE 0x8227

#ifndef CTL_CODE

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#endif

#ifndef METHOD_BUFFERED
#define METHOD_BUFFERED                 0
#endif

#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS                 0
#endif

#define IOCTL_SET_TIMEOUT                       CTL_CODE( WINIOMONITOR_DEVICE_TYPE, 0x800 + 1, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_GET_TIMEOUT                       CTL_CODE( WINIOMONITOR_DEVICE_TYPE, 0x800 + 2, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_SET_FILTERING
#define IOCTL_GET_FILTERING

#define IOCTL_SET_GLOBAL_OPTS

#define IOCTL_ADD_PROCESS_POLICY                CTL_CODE( WINIOMONITOR_DEVICE_TYPE, 0x800 + 10, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_DEL_PROCESS_POLICY_BY_PID         CTL_CODE( WINIOMONITOR_DEVICE_TYPE, 0x800 + 11, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_DEL_PROCESS_POLICY_BY_MASK        CTL_CODE( WINIOMONITOR_DEVICE_TYPE, 0x800 + 12, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_GET_PROCESS_POLICY_COUNT          CTL_CODE( WINIOMONITOR_DEVICE_TYPE, 0x800 + 13, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_CLEAR_PROCESS_POLICY              CTL_CODE( WINIOMONITOR_DEVICE_TYPE, 0x800 + 14, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_GET_NOTIFY_EVENTS

#endif // HDR_WINIOMONITOR_IOCTL