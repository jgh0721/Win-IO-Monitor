#include "WinIOMonitor_Filter.hpp"

#include "utilities/osInfoMgr.hpp"
#include "callbacks/callbacks.hpp"
#include "utilities/contextMgr.hpp"

#include "WinIOMonitor_W32API.hpp"
#include "WinIOMonitor_Names.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define CTX_STRING_TAG                          'tSxC'
#define CTX_RESOURCE_TAG                        'cRxC'
#define CTX_INSTANCE_CONTEXT_TAG                'cIxC'
#define CTX_VOLUME_CONTEXT_TAG                  'cVxc'
#define CTX_FILE_CONTEXT_TAG                    'cFxC'
#define CTX_STREAM_CONTEXT_TAG                  'cSxC'
#define CTX_STREAMHANDLE_CONTEXT_TAG            'cHxC'
#define CTX_TRANSACTION_CONTEXT_TAG             'cTxc'

///////////////////////////////////////////////////////////////////////////////

CONST FLT_OPERATION_REGISTRATION FilterCallbacks[] = {
    { IRP_MJ_CREATE,                        0,      WinIOPreCreate,      WinIOPostCreate    },

    { IRP_MJ_CREATE_NAMED_PIPE,             0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_CLOSE,                         0,      WinIOPreClose,      WinIOPostClose    },

    { IRP_MJ_READ,                          0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_WRITE,                         0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_QUERY_INFORMATION,             0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_SET_INFORMATION,               0,      WinIOPreSetInformation,      WinIOPostSetInformation    },

    { IRP_MJ_QUERY_EA,                      0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_SET_EA,                        0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_FLUSH_BUFFERS,                 0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,      0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_SET_VOLUME_INFORMATION,        0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_DIRECTORY_CONTROL,             0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_FILE_SYSTEM_CONTROL,           0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_DEVICE_CONTROL,                0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,       0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_SHUTDOWN,                      0,      FilterPreOperationPassThrough,      NULL    },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,                  0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_CLEANUP,                       0,      WinIOPreCleanup,      WinIOPostCleanup    },

    { IRP_MJ_CREATE_MAILSLOT,               0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_QUERY_SECURITY,                0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_SET_SECURITY,                  0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_QUERY_QUOTA,                   0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_SET_QUOTA,                     0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_PNP,                           0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,       0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,       0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,                     0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,                     0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,                      0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,                      0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,                 0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_NETWORK_QUERY_OPEN,                        0,      WinIOPreNetworkQueryOpen,      WinIOPostNetworkQueryOpen    },

    { IRP_MJ_MDL_READ,                                  0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_MDL_READ_COMPLETE,                         0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_PREPARE_MDL_WRITE,                         0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_MDL_WRITE_COMPLETE,                        0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_VOLUME_MOUNT,                              0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_VOLUME_DISMOUNT,                           0,      FilterPreOperationPassThrough,      FilterPostOperationPassThrough    },

    { IRP_MJ_OPERATION_END } };

CONST FLT_CONTEXT_REGISTRATION FilterContextXP[] = {
    {
        FLT_INSTANCE_CONTEXT,
        0,
        (PFLT_CONTEXT_CLEANUP_CALLBACK ) CtxInstanceContextCleanupCallback,
        CTX_INSTANCE_CONTEXT_SIZE,
        CTX_INSTANCE_CONTEXT_TAG
    },

    {
        FLT_STREAM_CONTEXT,
        0,
        ( PFLT_CONTEXT_CLEANUP_CALLBACK )CtxStreamContextCleanupCallback,
        CTX_STREAM_CONTEXT_SIZE,
        CTX_STREAM_CONTEXT_TAG
    },

    {
        FLT_CONTEXT_END
    }
};

CONST FLT_CONTEXT_REGISTRATION FilterContextVista[] = {
    {
        FLT_INSTANCE_CONTEXT,
        0,
        ( PFLT_CONTEXT_CLEANUP_CALLBACK )CtxInstanceContextCleanupCallback,
        CTX_INSTANCE_CONTEXT_SIZE,
        CTX_INSTANCE_CONTEXT_TAG
    },

    {
        FLT_STREAM_CONTEXT,
        0,
        ( PFLT_CONTEXT_CLEANUP_CALLBACK )CtxStreamContextCleanupCallback,
        CTX_STREAM_CONTEXT_SIZE,
        CTX_STREAM_CONTEXT_TAG
    },

    {
        FLT_CONTEXT_END
    }
};

const nsW32API::FLT_REGISTRATION_XP FilterRegistrationXP = {
    sizeof( nsW32API::FLT_REGISTRATION_XP ),            //  Size
    FLT_REGISTRATION_VERSION,                           //  Version
    0,                                                  //  Flags

    FilterContextXP,
    FilterCallbacks,

    MiniFilterUnload,

    InstanceSetup,
    InstanceQueryTeardown,
    InstanceTeardownStart,
    InstanceTeardownComplete
};

const nsW32API::FLT_REGISTRATION_VISTA FilterRegistrationVista = {
    sizeof( nsW32API::FLT_REGISTRATION_VISTA ),         //  Size
    FLT_REGISTRATION_VERSION_0202,                      //  Version
    0,                                                  //  Flags

    FilterContextVista,
    FilterCallbacks,

    MiniFilterUnload,

    InstanceSetup,
    InstanceQueryTeardown,
    InstanceTeardownStart,
    InstanceTeardownComplete
};

NTSTATUS InitializeMiniFilter( CTX_GLOBAL_DATA* GlobalContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        if( nsUtils::VerifyVersionInfoEx( 5, "<" ) == true )
            break;

        if( nsUtils::VerifyVersionInfoEx( 5, "=" ) == true )
        {
            Status = FltRegisterFilter( GlobalContext->DriverObject, ( PFLT_REGISTRATION )&FilterRegistrationXP, &GlobalContext->Filter );
        }

        if( nsUtils::VerifyVersionInfoEx( 6, ">=" ) == true )
        {
            Status = FltRegisterFilter( GlobalContext->DriverObject, ( PFLT_REGISTRATION )&FilterRegistrationVista, &GlobalContext->Filter );
        }

        if( !NT_SUCCESS( Status ) )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[WinIOMon] %s %s Status=0x%08x\n",
                         __FUNCTION__, "FltRegisterFilter FAILED", Status ) );
            break;
        }

        // NOTE: Note that in Windows 2000 and Windows XP, before FltGetRoutineAddress is called at least one minifilter on the system must call FltRegisterFilter.
        // The call to FltRegisterFilter is necessary to initialize global data structures.
        nsW32API::InitializeFltMgrAPI( &nsW32API::FltMgrAPIMgr );

        IF_FALSE_BREAK( Status, InitializeMiniFilterPort( GlobalContext ) );

        Status = FltStartFiltering( GlobalContext->Filter );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[WinIOMon] %s %s Status=0x%08x\n",
                         __FUNCTION__, "FltStartFiltering FAILED", Status ) );
            break;
        }

    } while( false );

    if( !NT_SUCCESS( Status ) )
    {
        if( GlobalContext->Filter != NULLPTR )
        {
            FltUnregisterFilter( GlobalContext->Filter );
            GlobalContext->Filter = NULLPTR;
        }
    }

    KdPrint( ( "[WinIOMon] %s Line=%d Status=0x%08x\n", __FUNCTION__, __LINE__, Status ) );
    return Status;
}

NTSTATUS InitializeMiniFilterPort( CTX_GLOBAL_DATA* GlobalContext )
{
    KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] %s GlobalContext=%p", __FUNCTION__, GlobalContext ) );
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PSECURITY_DESCRIPTOR SD = NULLPTR;

    do
    {
        Status = FltBuildDefaultSecurityDescriptor( &SD, FLT_PORT_ALL_ACCESS );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[WinIOMon] %s %s Status=0x%08x\n",
                         __FUNCTION__, "FltBuildDefaultSecurityDescriptor FAILED", Status ) );
            break;
        }

        UNICODE_STRING PortName;
        OBJECT_ATTRIBUTES OA;

        RtlInitUnicodeString( &PortName, PORT_NAME );
        InitializeObjectAttributes( &OA, &PortName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, SD );

        Status = FltCreateCommunicationPort( GlobalContext->Filter, &GlobalContext->ServerPort, 
                                             &OA, NULL,
                                             ClientConnectNotify,
                                             ClientDisconnectNotify,
                                             ClientMessageNotify,
                                             64 );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[WinIOMon] %s %s Status=0x%08x PortName=%wZ\n",
                         __FUNCTION__, "FltCreateCommunicationPort FAILED", Status, &PortName ) );
            break;
        }

    } while( false );

    if( SD != NULLPTR )
        FltFreeSecurityDescriptor( SD );

    return Status;
}

NTSTATUS FLTAPI MiniFilterUnload( FLT_FILTER_UNLOAD_FLAGS Flags )
{
    KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] %s Flags=%d", __FUNCTION__, Flags ) );

    if( GlobalContext.ServerPort != NULLPTR )
        FltCloseCommunicationPort( GlobalContext.ServerPort );
    GlobalContext.ServerPort = NULLPTR;

    FltUnregisterFilter( GlobalContext.Filter );
    GlobalContext.Filter = NULLPTR;

    return STATUS_SUCCESS;
}

NTSTATUS ClientConnectNotify( PFLT_PORT ClientPort, PVOID ServerPortCookie, PVOID ConnectionContext,
                              ULONG SizeOfContext, PVOID* ConnectionPortCookie )
{
    *ConnectionPortCookie = ClientPort;
    return STATUS_SUCCESS;
}

void ClientDisconnectNotify( PVOID ConnectionCookie )
{
    KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] %s ConnectionCookie=%d", __FUNCTION__, ConnectionCookie ) );

    FltCloseClientPort( GlobalContext.Filter, &GlobalContext.ClientPort );
    GlobalContext.ClientPort = NULLPTR;
}

NTSTATUS ClientMessageNotify( PVOID PortCookie, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer,
                              ULONG OutputBufferLength, PULONG ReturnOutputBufferLength )
{
    return STATUS_SUCCESS;
}
