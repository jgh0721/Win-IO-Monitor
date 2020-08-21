#include "WinIOMonitor.hpp"

#include "utilities/osInfoMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

CONST FLT_OPERATION_REGISTRATION FilterCallbacks[] = {
    { IRP_MJ_OPERATION_END }
};

///////////////////////////////////////////////////////////////////////////////

NTSTATUS FLTAPI DriverEntry( PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath )
{
    KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] %s DriverObject=%p|RegistryPath=%wz\n",
                 __FUNCTION__, DriverObject, RegistryPath ) );

    NTSTATUS Status = STATUS_SUCCESS;

    do
    {
        nsUtils::InitializeOSInfo();
        IF_FALSE_BREAK( Status, InitializeGlobalContext( DriverObject ) );

        DriverObject->DriverUnload = DriverUnload;

    } while( false );

    return Status;
}

void DriverUnload( PDRIVER_OBJECT DriverObject )
{
    
}

NTSTATUS InitializeGlobalContext( PDRIVER_OBJECT DriverObject )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {

        
    } while( false );

    return Status;
}
