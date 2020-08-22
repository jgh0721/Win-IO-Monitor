#include "WinIOMonitor.hpp"

#include "deviceMgmt.hpp"
#include "WinIOMonitor_Filter.hpp"
#include "utilities/bufferMgr.hpp"

#include "utilities/osInfoMgr.hpp"
#include "utilities/contextMgr.hpp"
#include "utilities/procNameMgr.hpp"

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
        nsW32API::InitializeNtOsKrnlAPI( &nsW32API::NtOsKrnlAPIMgr );

        IF_FALSE_BREAK( Status, InitializeGlobalContext( DriverObject ) );
        IF_FALSE_BREAK( Status, InitializeFeatures( &GlobalContext ) );
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

    StopProcessNotify();

    RemoveControlDevice( GlobalContext );

    ExDeleteNPagedLookasideList( &GlobalContext.FileNameLookasideList );
    ExDeleteNPagedLookasideList( &GlobalContext.ProcNameLookasideList );
    ExDeleteNPagedLookasideList( &GlobalContext.SendPacketLookasideList );
    ExDeleteNPagedLookasideList( &GlobalContext.ReplyPacketLookasideList );
}

NTSTATUS InitializeGlobalContext( PDRIVER_OBJECT DriverObject )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        RtlZeroMemory( &GlobalContext, sizeof( CTX_GLOBAL_DATA ) );
        GlobalContext.DriverObject = DriverObject;

        IF_FALSE_BREAK( Status, CreateControlDevice( GlobalContext ) );

        ExInitializeNPagedLookasideList( &GlobalContext.FileNameLookasideList,
                                         NULL, NULL, 0,
                                         POOL_FILENAME_SIZE, POOL_FILENAME_TAG, 0 );

        ExInitializeNPagedLookasideList( &GlobalContext.ProcNameLookasideList,
                                         NULL, NULL, 0,
                                         POOL_PROCNAME_SIZE, POOL_PROCNAME_TAG, 0 );

        ExInitializeNPagedLookasideList( &GlobalContext.SendPacketLookasideList,
                                         NULL, NULL, 0,
                                         POOL_MSG_SEND_SIZE, POOL_MSG_SEND_TAG, 0 );

        ExInitializeNPagedLookasideList( &GlobalContext.ReplyPacketLookasideList,
                                         NULL, NULL, 0,
                                         POOL_MSG_REPLY_SIZE, POOL_MSG_REPLY_TAG, 0 );

        AllocateBuffer<WCHAR>( BUFFER_FILENAME );

    } while( false );

    return Status;
}

NTSTATUS InitializeFeatures( CTX_GLOBAL_DATA* GlobalContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        IF_FALSE_BREAK( Status, StartProcessNotify() );
        
    } while( false );

    return Status;
}
