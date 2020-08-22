#ifndef HDR_WINIOMONITOR_FILTER
#define HDR_WINIOMONITOR_FILTER

#include "fltBase.hpp"

#include "utilities/contextMgr_Defs.hpp"
#include "WinIOMonitor_W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

extern const FLT_OPERATION_REGISTRATION FilterCallbacks[];

extern const FLT_CONTEXT_REGISTRATION FilterContextXP[];
extern const FLT_CONTEXT_REGISTRATION FilterContextVista[];

extern const nsW32API::FLT_REGISTRATION_XP FilterRegistrationXP;
extern const nsW32API::FLT_REGISTRATION_VISTA FilterRegistrationVista;

///////////////////////////////////////////////////////////////////////////////

NTSTATUS InitializeMiniFilter( __in CTX_GLOBAL_DATA* GlobalContext );
NTSTATUS InitializeMiniFilterPort( __in CTX_GLOBAL_DATA* GlobalContext );

EXTERN_C NTSTATUS FLTAPI MiniFilterUnload( __in FLT_FILTER_UNLOAD_FLAGS Flags );

EXTERN_C NTSTATUS ClientConnectNotify( IN PFLT_PORT ClientPort,
                                       IN PVOID ServerPortCookie,
                                       IN PVOID ConnectionContext,
                                       IN ULONG SizeOfContext,
                                       OUT PVOID* ConnectionPortCookie );

EXTERN_C VOID ClientDisconnectNotify( IN PVOID ConnectionCookie );

EXTERN_C NTSTATUS ClientMessageNotify( IN PVOID PortCookie,
                                       IN PVOID InputBuffer OPTIONAL,
                                       IN ULONG InputBufferLength,
                                       OUT PVOID OutputBuffer OPTIONAL,
                                       IN ULONG OutputBufferLength,
                                       OUT PULONG ReturnOutputBufferLength );

#endif // HDR_WINIOMONITOR_FILTER