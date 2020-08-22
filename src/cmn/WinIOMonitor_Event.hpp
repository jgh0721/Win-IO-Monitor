#ifndef HDR_WINIOMONITOR_EVENT
#define HDR_WINIOMONITOR_EVENT

#if defined(USE_ON_KERNEL)
    #include "fltBase.hpp"
#else

    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif

    #include <windows.h>
    #include <winternl.h>
    #include <fltUser.h>
    #include <fltUserStructures.h>
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _MSG_SEND_PACKET
{
    ULONG                           MessageCategory;

} MSG_SEND_PACKET, *PMSG_SEND_PACKET;

typedef struct _MSG_REPLY_PACKET
{
    ULONG                           ReturnStatus;

} MSG_REPLY_PACKET, *PMSG_REPLY_PACKET;

#endif // HDR_WINIOMONITOR_EVENT