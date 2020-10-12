#include "WinIOIsolation.hpp"

#include "deviceCntl.hpp"
#include "deviceMgmt.hpp"
#include "WinIOIsolation_Filter.hpp"
#include "irpContext_Defs.hpp"
#include "privateFCBMgr_Defs.hpp"

#include "utilities/osInfoMgr.hpp"
#include "utilities/contextMgr.hpp"
#include "utilities/procNameMgr.hpp"
#include "utilities/bufferMgr.hpp"
#include "utilities/volumeNameMgr.hpp"

#include "policies/GlobalFilter.hpp"
#include "policies/ProcessFilter.hpp"

#include "metadata/Metadata.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

NTSTATUS FLTAPI DriverEntry( PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath )
{
    KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOSol] %s DriverObject=%p|RegistryPath=%wZ\n",
                 __FUNCTION__, DriverObject, RegistryPath ) );

    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        nsUtils::InitializeOSInfo();
        GlobalNtOsKrnlMgr.Init();
        GlobalFltMgr.Init();

        PFAST_IO_DISPATCH FastIODispatch = ( PFAST_IO_DISPATCH )ExAllocatePoolWithTag( NonPagedPool, sizeof( FAST_IO_DISPATCH ), 'abcd' );
        if( FastIODispatch == NULLPTR )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[WinIOSol] %s %s\n", __FUNCTION__, "ExAllocatePoolWithTag FAILED" ) );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        IF_FAILED_BREAK( Status, InitializeGlobalContext( DriverObject ) );
        IF_FAILED_BREAK( Status, InitializeFeatures( &GlobalContext ) );
        IF_FAILED_BREAK( Status, InitializeMiniFilter( &GlobalContext ) );

        DriverObject->MajorFunction[ IRP_MJ_CREATE ]    = DeviceCreate;
        DriverObject->MajorFunction[ IRP_MJ_CLEANUP ]   = DeviceCleanup;
        DriverObject->MajorFunction[ IRP_MJ_CLOSE ]     = DeviceClose;

        DriverObject->DriverUnload                      = DriverUnload;

        RtlZeroMemory( FastIODispatch, sizeof( FAST_IO_DISPATCH ) );
        FastIODispatch->SizeOfFastIoDispatch = sizeof( FAST_IO_DISPATCH );
        FastIODispatch->FastIoDeviceControl = (PFAST_IO_DEVICE_CONTROL)FastIoDeviceControl;
        DriverObject->FastIoDispatch = FastIODispatch;

        Status = STATUS_SUCCESS;

    } while( false );

    return Status;
}

void DriverUnload( PDRIVER_OBJECT DriverObject )
{
    KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOSol] %s DriverObject=%p\n",
                 __FUNCTION__, DriverObject ) );

    /*!
        1. wehn invoke sc stop, only call DriverUnload
        2. when invoke fltmc unload, first call FilterUnloadcallback then call DriverUnload
    */
    if( GlobalContext.Filter != NULLPTR )
        MiniFilterUnload( 0 );

    UninitializeProcessFilter();
    UninitializeGlobalFilter();
    StopProcessNotify();

    RemoveControlDevice( GlobalContext );

    UninitializeVolumeNameMgr();
    UninitializeMetaDataMgr();

    ExDeleteNPagedLookasideList( &GlobalContext.DebugLookasideList );

    ExDeleteNPagedLookasideList( &GlobalContext.IrpContextLookasideList );
    ExDeleteNPagedLookasideList( &GlobalContext.FileNameLookasideList );
    ExDeleteNPagedLookasideList( &GlobalContext.ProcNameLookasideList );
    ExDeleteNPagedLookasideList( &GlobalContext.SendPacketLookasideList );
    ExDeleteNPagedLookasideList( &GlobalContext.ReplyPacketLookasideList );
    ExDeleteNPagedLookasideList( &GlobalContext.FcbLookasideList );
    ExDeleteNPagedLookasideList( &GlobalContext.CcbLookasideList );

    ExDeletePagedLookasideList( &GlobalContext.SwapReadLookasideList_1024 );
    ExDeletePagedLookasideList( &GlobalContext.SwapReadLookasideList_4096 );
    ExDeletePagedLookasideList( &GlobalContext.SwapReadLookasideList_8192 );
    ExDeletePagedLookasideList( &GlobalContext.SwapReadLookasideList_16384 );
    ExDeletePagedLookasideList( &GlobalContext.SwapReadLookasideList_65536 );

    ExDeletePagedLookasideList( &GlobalContext.SwapWriteLookasideList_1024 );
    ExDeletePagedLookasideList( &GlobalContext.SwapWriteLookasideList_4096 );
    ExDeletePagedLookasideList( &GlobalContext.SwapWriteLookasideList_8192 );
    ExDeletePagedLookasideList( &GlobalContext.SwapWriteLookasideList_16384 );
    ExDeletePagedLookasideList( &GlobalContext.SwapWriteLookasideList_65536 );

    if( DriverObject->FastIoDispatch != NULLPTR )
        ExFreePool( DriverObject->FastIoDispatch );
}

NTSTATUS InitializeGlobalContext( PDRIVER_OBJECT DriverObject )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        RtlZeroMemory( &GlobalContext, sizeof( CTX_GLOBAL_DATA ) );
        GlobalContext.DriverObject = DriverObject;
        GlobalContext.TimeOutMs.QuadPart = RELATIVE( MILLISECONDS( 3000 ) );

        IF_FAILED_BREAK( Status, CreateControlDevice( GlobalContext ) );

        ExInitializeNPagedLookasideList( &GlobalContext.DebugLookasideList,
                                         NULL, NULL, 0,
                                         1024, POOL_IRPCONTEXT_TAG, 0 );

        ExInitializeNPagedLookasideList( &GlobalContext.IrpContextLookasideList,
                                         NULL, NULL, 0,
                                         sizeof(IRP_CONTEXT), POOL_IRPCONTEXT_TAG, 0 );

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

        ExInitializeNPagedLookasideList( &GlobalContext.FcbLookasideList,
                                         NULL, NULL, 0,
                                         sizeof( FCB ), POOL_FCB_TAG, 0 );

        ExInitializeNPagedLookasideList( &GlobalContext.CcbLookasideList,
                                         NULL, NULL, 0,
                                         sizeof( CCB ), POOL_FCB_TAG, 0 );

        ExInitializePagedLookasideList( &GlobalContext.SwapReadLookasideList_1024, NULL, NULL, 0, 
                                        BUFFER_SWAP_READ_1024_SIZE, POOL_READ_TAG, 0 );
        ExInitializePagedLookasideList( &GlobalContext.SwapReadLookasideList_4096, NULL, NULL, 0,
                                        BUFFER_SWAP_READ_4096_SIZE, POOL_READ_TAG, 0 );
        ExInitializePagedLookasideList( &GlobalContext.SwapReadLookasideList_8192, NULL, NULL, 0,
                                        BUFFER_SWAP_READ_8192_SIZE, POOL_READ_TAG, 0 );
        ExInitializePagedLookasideList( &GlobalContext.SwapReadLookasideList_16384, NULL, NULL, 0,
                                        BUFFER_SWAP_READ_16384_SIZE, POOL_READ_TAG, 0 );
        ExInitializePagedLookasideList( &GlobalContext.SwapReadLookasideList_65536, NULL, NULL, 0,
                                        BUFFER_SWAP_READ_65536_SIZE, POOL_READ_TAG, 0 );

        ExInitializePagedLookasideList( &GlobalContext.SwapWriteLookasideList_1024, NULL, NULL, 0,
                                        BUFFER_SWAP_WRITE_1024_SIZE, POOL_WRITE_TAG, 0 );
        ExInitializePagedLookasideList( &GlobalContext.SwapWriteLookasideList_4096, NULL, NULL, 0,
                                        BUFFER_SWAP_WRITE_4096_SIZE, POOL_WRITE_TAG, 0 );
        ExInitializePagedLookasideList( &GlobalContext.SwapWriteLookasideList_8192, NULL, NULL, 0,
                                        BUFFER_SWAP_WRITE_8192_SIZE, POOL_WRITE_TAG, 0 );
        ExInitializePagedLookasideList( &GlobalContext.SwapWriteLookasideList_16384, NULL, NULL, 0,
                                        BUFFER_SWAP_WRITE_16384_SIZE, POOL_WRITE_TAG, 0 );
        ExInitializePagedLookasideList( &GlobalContext.SwapWriteLookasideList_65536, NULL, NULL, 0,
                                        BUFFER_SWAP_WRITE_65536_SIZE, POOL_WRITE_TAG, 0 );

        Status = STATUS_SUCCESS;

    } while( false );

    KdPrint( ( "[WinIOSol] %s Line=%d Status=0x%08x\n", __FUNCTION__, __LINE__, Status ) );
    return Status;
}

NTSTATUS InitializeFeatures( CTX_GLOBAL_DATA* GlobalContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        IF_FAILED_BREAK( Status, InitializeMetaDataMgr() );
        IF_FAILED_BREAK( Status, InitializeVolumeNameMgr() );

        IF_FAILED_BREAK( Status, StartProcessNotify() );
        IF_FAILED_BREAK( Status, InitializeGlobalFilter() );
        IF_FAILED_BREAK( Status, InitializeProcessFilter() );

        // for Testing Purpose only
        GlobalFilter_Add( L"*isolationtest*", true );

    } while( false );

    KdPrint( ( "[WinIOSol] %s Line=%d Status=0x%08x\n", __FUNCTION__, __LINE__, Status ) );
    return Status;
}
