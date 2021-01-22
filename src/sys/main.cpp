
#include "base.hpp"

NTSTATUS Main( DRIVER_OBJECT* DriverObject, UNICODE_STRING* RegistryPath )
{
    KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "" ) );

    return 0;
}