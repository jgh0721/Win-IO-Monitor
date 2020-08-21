#ifndef WINIOMONITOR_WINIOMONITOR_HPP
#define WINIOMONITOR_WINIOMONITOR_HPP

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C NTSTATUS FLTAPI DriverEntry( __in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath );

#endif //WINIOMONITOR_WINIOMONITOR_HPP
