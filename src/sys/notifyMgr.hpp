#ifndef HDR_WINIOMONITOR_NOTIFY
#define HDR_WINIOMONITOR_NOTIFY

#include "fltBase.hpp"
#include "notifyMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS InitializeNotifyMgr();
VOID CloseNotifyMgr();

NOTIFY_ITEM* AllocateNotifyItem( __in ULONG SendPacketSize );
VOID FreeNoitifyItem( __in NOTIFY_ITEM* NotifyItem );
VOID AppendNotifyItem( __in NOTIFY_ITEM* NotifyItem );

NTSTATUS CollectNotifyItem( __inout PVOID Buffer, __in ULONG BufferSize, __out ULONG* WrittenBytes, __out ULONG* NotifyItemCount );

#endif // HDR_WINIOMONITOR_NOTIFY