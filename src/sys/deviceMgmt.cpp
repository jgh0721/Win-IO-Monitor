#include "deviceMgmt.hpp"

#include "WinIOMonitor_Names.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

NTSTATUS CreateControlDevice( CTX_GLOBAL_DATA& GlobalContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    UNICODE_STRING DeviceName;
    UNICODE_STRING SymbolicDeviceName;

    do
    {
        RtlInitUnicodeString( &DeviceName, L"\\FileSystem\\Filters" DEVICE_NAME );

        Status = IoCreateDevice( GlobalContext.DriverObject, 0, &DeviceName,
                                 FILE_DEVICE_UNKNOWN, 0, FALSE,
                                 &GlobalContext.DeviceObject );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[WinIOMon] %s %s Status=0x%08x\n",
                         __FUNCTION__, "IoCreateDevice FAILED", Status ) );
            break;
        }

        RtlInitUnicodeString( &SymbolicDeviceName, L"\\DosDevices" DEVICE_NAME );

        Status = IoCreateSymbolicLink( &SymbolicDeviceName, &DeviceName );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[WinIOMon] %s %s Status=0x%08x\n",
                         __FUNCTION__, "IoCreateSymbolicLink FAILED", Status ) );
            break;
        }

    } while( false );

    return Status;
}

NTSTATUS RemoveControlDevice( CTX_GLOBAL_DATA& GlobalContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    UNICODE_STRING SymbolicDeviceName;

    do
    {
        RtlInitUnicodeString( &SymbolicDeviceName, L"\\DosDevices" DEVICE_NAME );

        Status = IoDeleteSymbolicLink( &SymbolicDeviceName );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[WinIOMon] %s %s Status=0x%08x\n",
                         __FUNCTION__, "IoDeleteSymbolicLink FAILED", Status ) );
            break;
        }

        IoDeleteDevice( GlobalContext.DeviceObject );
        
    } while( false );

    return Status;
}
