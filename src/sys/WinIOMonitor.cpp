#include "WinIOMonitor.hpp"

#include "deviceMgmt.hpp"
#include "WinIOMonitor_Filter.hpp"

#include "utilities/osInfoMgr.hpp"
#include "utilities/contextMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

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
        IF_FALSE_BREAK( Status, InitializeMiniFilter( &GlobalContext ) );

        DriverObject->MajorFunction[ IRP_MJ_CREATE ]    = DeviceCreate;
        DriverObject->MajorFunction[ IRP_MJ_CLEANUP ]   = DeviceCleanup;
        DriverObject->MajorFunction[ IRP_MJ_CLOSE ]     = DeviceClose;

        DriverObject->DriverUnload = DriverUnload;

    } while( false );

    return Status;
}

void DriverUnload( PDRIVER_OBJECT DriverObject )
{
    KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] %s DriverObject=%p\n",
                 __FUNCTION__, DriverObject ) );

    RemoveControlDevice( GlobalContext );

}

NTSTATUS InitializeGlobalContext( PDRIVER_OBJECT DriverObject )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        RtlZeroMemory( &GlobalContext, sizeof( CTX_GLOBAL_DATA ) );
        GlobalContext.DriverObject = DriverObject;

        IF_FALSE_BREAK( Status, CreateControlDevice( GlobalContext ) );
        
    } while( false );

    return Status;
}
