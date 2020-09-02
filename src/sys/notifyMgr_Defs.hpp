#ifndef HDR_WINIOMONITOR_NOTIFY_DEFS
#define HDR_WINIOMONITOR_NOTIFY_DEFS

#include "fltBase.hpp"

#include "WinIOMonitor_Event.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _NOTIFY_ITEM
{
    MSG_SEND_PACKET*    SendPacket;

    LIST_ENTRY          ListEntry;

} NOTIFY_ITEM, *PNOTIIFY_ITEM;

#endif // HDR_WINIOMONITOR_NOTIFY_DEFS