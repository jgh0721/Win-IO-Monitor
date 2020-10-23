#ifndef HDR_ISOLATION_COMMUNICATION_DEFS
#define HDR_ISOLATION_COMMUNICATION_DEFS

#include "fltBase.hpp"
#include "utilities/bufferMgr_Defs.hpp"

#include "WinIOIsolation_Event.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////
/// MSG_FS_NOTIFY_TYPE

typedef struct _FS_NOTIFY
{
    KEVENT                      ExitEvent;
    KEVENT                      NotifyEvent;
    volatile BOOLEAN            IsExit;
    KSPIN_LOCK                  Lock;

    LIST_ENTRY                  ListHead;

} FS_NOTIFY, *PFS_NOTIFY;

typedef struct _FS_NOTIFY_ITEM
{
    TyGenericBuffer<MSG_SEND_PACKET> SendPacket;

    LIST_ENTRY                  ListEntry;

} FS_NOTIFY_ITEM, *PFS_NOTIFY_ITEM;

#endif // HDR_ISOLATION_COMMUNICATION_DEFS