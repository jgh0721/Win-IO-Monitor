#include "WinIOMonitor_Filter.hpp"


#include "WinIOMonitor_Names.hpp"
#include "utilities/osInfoMgr.hpp"
#include "WinIOMonitor_W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////


CONST FLT_OPERATION_REGISTRATION FilterCallbacks[] = {
    { IRP_MJ_OPERATION_END }
};

CONST FLT_CONTEXT_REGISTRATION FilterContextXP[] = {
    {
        FLT_CONTEXT_END
    }
};

CONST FLT_CONTEXT_REGISTRATION FilterContextVista[] = {
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

    MiniFilterUnload
};

const nsW32API::FLT_REGISTRATION_VISTA FilterRegistrationVista = {
    sizeof( nsW32API::FLT_REGISTRATION_VISTA ),         //  Size
    FLT_REGISTRATION_VERSION_0202,                      //  Version
    0,                                                  //  Flags

    FilterContextVista,
    FilterCallbacks,

    MiniFilterUnload
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

NTSTATUS MiniFilterUnload( FLT_FILTER_UNLOAD_FLAGS Flags )
{
    KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] %s Flags=%d", __FUNCTION__, Flags ) );

    if( GlobalContext.ServerPort != NULLPTR )
        FltCloseCommunicationPort( GlobalContext.ServerPort );
    GlobalContext.ServerPort = NULLPTR;

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
}

NTSTATUS ClientMessageNotify( PVOID PortCookie, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer,
                              ULONG OutputBufferLength, PULONG ReturnOutputBufferLength )
{
    return STATUS_SUCCESS;
}
