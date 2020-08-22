#ifndef WINIOMONITOR_WINIOMONITOR_HPP
#define WINIOMONITOR_WINIOMONITOR_HPP

#include "fltBase.hpp"
#include "utilities/contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C NTSTATUS FLTAPI DriverEntry( __in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath );

EXTERN_C VOID DriverUnload( __in PDRIVER_OBJECT DriverObject );

///////////////////////////////////////////////////////////////////////////////

NTSTATUS InitializeGlobalContext( __in PDRIVER_OBJECT DriverObject );
NTSTATUS InitializeFeatures( __in CTX_GLOBAL_DATA* GlobalContext );

#endif //WINIOMONITOR_WINIOMONITOR_HPP
