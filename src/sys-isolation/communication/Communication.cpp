#include "Communication.hpp"

#include "callbacks/fltCreateFile.hpp"

#include "irpContext.hpp"
#include "utilities/bufferMgr.hpp"
#include "driverMgmt.hpp"

#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

struct _EVENT_NOTIFY_CONTEXT
{
    KSPIN_LOCK                                  Lock;
    KEVENT                                      NotifyEvent;
    KEVENT                                      ExitEvent;
    volatile BOOLEAN                            IsExit;

    LIST_ENTRY                                  ListHead;

} EVENT_NOTIFY_CONTEXT;

///////////////////////////////////////////////////////////////////////////////
/// MSG_FS_NOTIFY_TYPE

NTSTATUS InitializeNotifyEventWorker()
{
    NTSTATUS Status = STATUS_SUCCESS;

    RtlZeroMemory( &EVENT_NOTIFY_CONTEXT, sizeof( _EVENT_NOTIFY_CONTEXT ) );

    KeInitializeSpinLock( &EVENT_NOTIFY_CONTEXT.Lock );
    KeInitializeEvent( &EVENT_NOTIFY_CONTEXT.NotifyEvent, SynchronizationEvent, FALSE );
    KeInitializeEvent( &EVENT_NOTIFY_CONTEXT.ExitEvent, NotificationEvent, FALSE );
    InitializeListHead( &EVENT_NOTIFY_CONTEXT.ListHead );

    // 현재 프로세스의 수를 구할 때 DISPATCH_LEVEL 이상이 아니면 수행도중 컨텍스트 스위칭이 아니라 부정확한 값이 반환될 수 있다
    ULONG uCount = 0; KIRQL oldIrql = 0;
    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
    uCount = KeGetCurrentProcessorNumber();
    KeLowerIrql( oldIrql );

    if( uCount <= 4 )
        uCount = uCount * 2;

    for( ULONG idx = 0; idx < uCount; ++idx )
    {
        HANDLE ThreadHandle = NULL;

        Status = PsCreateSystemThread( &ThreadHandle, ( ACCESS_MASK )0L, NULL, NULL, NULL, NotifyEventWorker, NULL );
        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] %s %s Status=0x%08x\n", __FUNCTION__, "Create System Thread FAILED", Status ) );
            break;
        }

        ZwClose( ThreadHandle );
    }

    Status = STATUS_SUCCESS;
    return Status;
}

NTSTATUS QueueNotifyEvent( FS_NOTIFY_ITEM* NotifyEventItem )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT( NotifyEventItem != NULLPTR );
    if( NotifyEventItem == NULLPTR )
        return STATUS_INVALID_PARAMETER;

    ExInterlockedInsertTailList( &EVENT_NOTIFY_CONTEXT.ListHead, &NotifyEventItem->ListEntry, &EVENT_NOTIFY_CONTEXT.Lock );
    KeSetEvent( &EVENT_NOTIFY_CONTEXT.NotifyEvent, ( KPRIORITY )0, FALSE );

    return Status;
}

void NotifyEventWorker( PVOID Context )
{
    UNREFERENCED_PARAMETER( Context );

    do
    {
        KeSetPriorityThread( KeGetCurrentThread(), HIGH_PRIORITY );

        NTSTATUS Status = STATUS_WAIT_0;
        PVOID WaitObjects[] = { &EVENT_NOTIFY_CONTEXT.NotifyEvent, &EVENT_NOTIFY_CONTEXT.ExitEvent };

        while( EVENT_NOTIFY_CONTEXT.IsExit != FALSE )
        {
            // Count 가 THREAD_WAIT_OBJECTS 이하라면 WaitBlockArray 는 NULL 이 가능함
            Status = KeWaitForMultipleObjects( _countof( WaitObjects ), WaitObjects,
                                               WaitAny, Executive, KernelMode,
                                               FALSE, NULL, NULL );

            if( EVENT_NOTIFY_CONTEXT.IsExit != FALSE )
                break;

            if( Status != STATUS_WAIT_0 )
                break;

            PLIST_ENTRY item = NULL;
            FS_NOTIFY_ITEM* notify = NULL;

            while( TRUE )
            {
                item = ExInterlockedRemoveHeadList( &EVENT_NOTIFY_CONTEXT.ListHead, &EVENT_NOTIFY_CONTEXT.Lock );
                if( item == NULL )
                    break;

                notify = CONTAINING_RECORD( item, FS_NOTIFY_ITEM, ListEntry );
                NotifyFSEventToClient( notify );
            }

            if( EVENT_NOTIFY_CONTEXT.IsExit != FALSE )
                break;
        }
        
    } while( false );
}

NTSTATUS NotifyFSEventToClient( FS_NOTIFY_ITEM* NotifyEventItem )
{
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<MSG_REPLY_PACKET> Reply;
    
    do
    {
        if( NotifyEventItem == NULLPTR )
            break;

        if( FeatureContext.IsRunning <= 0 )
            break;

        LONG EvtID = CreateEvtID();
        PFLT_PORT ClientPort = GetClientPort( EvtID );

        if( GlobalContext.Filter == NULLPTR || ClientPort == NULLPTR )
            break;

        ULONG uReplySize = MSG_REPLY_PACKET_SIZE;
        Reply = AllocateBuffer<MSG_REPLY_PACKET>( BUFFER_MSG_REPLY );
        
        Status = FltSendMessage( GlobalContext.Filter, &ClientPort,
                                 NotifyEventItem->SendPacket.Buffer, 
                                 NotifyEventItem->SendPacket.Buffer->MessageSize,
                                 Reply.Buffer, ( PULONG )&uReplySize, NULL );

        if( NT_SUCCESS( Status ) && Status == STATUS_TIMEOUT )
        {
            KdPrint( ( "[WinIOSol] EVENT %s|FltSendMessage Timeout|Status=0x%08x|PID=%d|TID=%d\n",
                       __FUNCTION__, Status, NotifyEventItem->SendPacket.Buffer->ProcessId, ( ULONG )PsGetCurrentThreadId() ) );
        }

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EVENT FltSendMessage Failed|0x%08x", Status ) );
        }

    } while( false );

    if( NotifyEventItem != NULLPTR )
    {
        DeallocateBuffer( &NotifyEventItem->SendPacket );
        ExFreePool( NotifyEventItem );
    }

    if( Reply.Buffer != NULLPTR )
        DeallocateBuffer( &Reply );

    return Status;
}

NTSTATUS UninitializeNotifyEventWorker()
{
    NTSTATUS Status = STATUS_SUCCESS;

    KeSetEvent( &EVENT_NOTIFY_CONTEXT.ExitEvent, ( KPRIORITY )0, FALSE );

    // 전송하지 못 한 리스트에 있는 모든 항목 제거
    auto Head = &EVENT_NOTIFY_CONTEXT.ListHead;
    while( IsListEmpty( Head ) != FALSE )
    {
        auto Current = ExInterlockedRemoveHeadList( Head, &EVENT_NOTIFY_CONTEXT.Lock );
        auto Item = CONTAINING_RECORD( Current, FS_NOTIFY_ITEM, ListEntry );

        RemoveEntryList( &Item->ListEntry );
        DeallocateBuffer( &Item->SendPacket );
        ExFreePool( Item );
    }

    return Status;
}

///////////////////////////////////////////////////////////////////////////////
/// MSG_FS_TYPE

NTSTATUS CheckEventFileCreateTo( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    TyGenericBuffer<MSG_SEND_PACKET> Packet;

    ASSERT( IrpContext != NULLPTR );

    __try
    {
        if( IrpContext == NULLPTR )
            __leave;

        PFLT_PORT ClientPort = GetClientPort( IrpContext );

        if( GlobalContext.Filter == NULL || ClientPort == NULLPTR )
            __leave;

        if( IrpContext->Result.Buffer == NULLPTR )
        {
            IrpContext->Result = AllocateBuffer<MSG_REPLY_PACKET>( BUFFER_MSG_REPLY );
            if( IrpContext->Result.Buffer == NULLPTR )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }
        }

        unsigned int SendPacketSize = MSG_SEND_PACKET_SIZE;
        unsigned int CchProcessFullPath = nsUtils::strlength( IrpContext->ProcessFullPath.Buffer ) + 1;
        unsigned int CchSrcFileFullpath = nsUtils::strlength( IrpContext->SrcFileFullPath.Buffer ) + 1;

        if( Packet.Buffer == NULLPTR || IrpContext->Result.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        Packet.Buffer->MessageSize = SendPacketSize;
        Packet.Buffer->MessageCate = MSG_CATE_FILESYSTEM;
        Packet.Buffer->MessageType = FS_PRE_CREATE;
        KeQuerySystemTime( &Packet.Buffer->EventTime );

        Packet.Buffer->ProcessId = IrpContext->ProcessId;
        Packet.Buffer->ThreadId = (ULONG)PsGetCurrentThreadId();

        const auto Args = ( CREATE_ARGS* )IrpContext->Params;
        Packet.Buffer->FileType = Args->MetaDataInfo.MetaData.Type;

        Packet.Buffer->Parameters.Create.DesiredAccess                  = Args->CreateDesiredAccess;
        Packet.Buffer->Parameters.Create.FileAttributes;
        Packet.Buffer->Parameters.Create.ShareAccess                    = 0;
        Packet.Buffer->Parameters.Create.CreateDisposition              = Args->CreateDisposition;
        Packet.Buffer->Parameters.Create.CreateOptions                  = Args->CreateOptions;

        Packet.Buffer->Parameters.Create.IsAlreadyExists                = FlagOn( Args->FileStatus, FILE_ALREADY_EXISTS );
        Packet.Buffer->Parameters.Create.IsContainSolutionMetaData      = Args->SolutionMetaDataSize > 0;
        Packet.Buffer->Parameters.Create.SolutionMetaDataSize           = Args->SolutionMetaDataSize;
        if( Args->SolutionMetaDataSize > 0 )
            RtlCopyMemory( Packet.Buffer->Parameters.Create.SolutionMetaData, Args->SolutionMetaData, Args->SolutionMetaDataSize );

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof(WCHAR);
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", IrpContext->ProcessFullPath.Buffer );

        Packet.Buffer->LengthOfSrcFileFullPath = CchSrcFileFullpath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfSrcFileFullPath = Packet.Buffer->OffsetOfProcessFullPath = Packet.Buffer->LengthOfProcessFullPath;
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfSrcFileFullPath ) ),
                            Packet.Buffer->LengthOfSrcFileFullPath,
                            L"%s", IrpContext->SrcFileFullPath.Buffer );

        ULONG ReplyLength = IrpContext->Result.BufferSize;

        Status = FltSendMessage( GlobalContext.Filter, &ClientPort,
                                 Packet.Buffer, Packet.Buffer->MessageSize,
                                 IrpContext->Result.Buffer, &ReplyLength,
                                 &GlobalContext.TimeOutMs );

        if( (NT_SUCCESS( Status ) && Status == STATUS_TIMEOUT) || (!NT_SUCCESS( Status )) )
        {
            KdPrint( ( "[WinIOSol] %s EvtID=%09d %s %s %s Status=0x%08x,%s Proc=%06d,%ws\n"
                       , JudgeInOut( IrpContext ), IrpContext->EvtID, __FUNCTION__, "FltSendMessage", Status == STATUS_TIMEOUT ? "Timeout" : "FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->ProcessId, IrpContext->ProcessFileName ) );

            if( Status == STATUS_TIMEOUT )
                Status = STATUS_AUDIT_FAILED;

            __leave;
        }
    }
    __finally
    {
        DeallocateBuffer( &Packet );
    }

    return Status;
}

NTSTATUS NotifyEventFileRenameTo( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    TyGenericBuffer<MSG_SEND_PACKET> Packet;
    ASSERT( IrpContext != NULLPTR );

    __try
    {
        if( IrpContext == NULLPTR )
            __leave;

        unsigned int SendPacketSize = MSG_SEND_PACKET_SIZE;
        unsigned int CchProcessFullPath = nsUtils::strlength( IrpContext->ProcessFullPath.Buffer ) + 1;
        unsigned int CchSrcFileFullpath = nsUtils::strlength( IrpContext->SrcFileFullPath.Buffer ) + 1;
        unsigned int CchDstFileFullpath = nsUtils::strlength( IrpContext->DstFileFullPath.Buffer ) + 1;

        if( Packet.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        Packet.Buffer->MessageSize = SendPacketSize;
        Packet.Buffer->MessageCate = MSG_CATE_FILESYSTEM_NOTIFY;
        Packet.Buffer->MessageType = FILE_WAS_RENAMED;
        KeQuerySystemTime( &Packet.Buffer->EventTime );

        Packet.Buffer->ProcessId = IrpContext->ProcessId;
        Packet.Buffer->ThreadId = ( ULONG )PsGetCurrentThreadId();

        Packet.Buffer->Parameters.SetFileInformation.FileInformationClass = IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", IrpContext->ProcessFullPath.Buffer );

        Packet.Buffer->LengthOfSrcFileFullPath = CchSrcFileFullpath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfSrcFileFullPath = Packet.Buffer->OffsetOfProcessFullPath = Packet.Buffer->LengthOfProcessFullPath;
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfSrcFileFullPath ) ),
                            Packet.Buffer->LengthOfSrcFileFullPath,
                            L"%s", IrpContext->SrcFileFullPath.Buffer );

        Packet.Buffer->LengthOfDstFileFullPath = CchDstFileFullpath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfDstFileFullPath = Packet.Buffer->OffsetOfSrcFileFullPath = Packet.Buffer->LengthOfSrcFileFullPath;
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfDstFileFullPath ) ),
                            Packet.Buffer->LengthOfDstFileFullPath,
                            L"%s", IrpContext->DstFileFullPath.Buffer );

        auto NotifyItem = ( FS_NOTIFY_ITEM* )ExAllocatePool( NonPagedPool, sizeof( FS_NOTIFY_ITEM ) );
        NotifyItem->SendPacket = Packet;
        
        QueueNotifyEvent( NotifyItem );
    }
    __finally
    {
        // NOTE: 통지 이벤트는 추후 클라이언트에 이벤트를 전송한 후 해당 스레드에서 삭제한다
    }

    return Status;
}

NTSTATUS NotifyEventFileDeleteTo( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    TyGenericBuffer<MSG_SEND_PACKET> Packet;
    ASSERT( IrpContext != NULLPTR );

    __try
    {
        if( IrpContext == NULLPTR )
            __leave;

        unsigned int SendPacketSize = MSG_SEND_PACKET_SIZE;
        unsigned int CchProcessFullPath = nsUtils::strlength( IrpContext->ProcessFullPath.Buffer ) + 1;
        unsigned int CchSrcFileFullpath = nsUtils::strlength( IrpContext->SrcFileFullPath.Buffer ) + 1;

        if( Packet.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        Packet.Buffer->MessageSize = SendPacketSize;
        Packet.Buffer->MessageCate = MSG_CATE_FILESYSTEM_NOTIFY;
        Packet.Buffer->MessageType = FILE_WAS_DELETED;
        KeQuerySystemTime( &Packet.Buffer->EventTime );

        Packet.Buffer->ProcessId = IrpContext->ProcessId;
        Packet.Buffer->ThreadId = ( ULONG )PsGetCurrentThreadId();

        Packet.Buffer->Parameters.SetFileInformation.FileInformationClass = IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", IrpContext->ProcessFullPath.Buffer );

        Packet.Buffer->LengthOfSrcFileFullPath = CchSrcFileFullpath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfSrcFileFullPath = Packet.Buffer->OffsetOfProcessFullPath = Packet.Buffer->LengthOfProcessFullPath;
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfSrcFileFullPath ) ),
                            Packet.Buffer->LengthOfSrcFileFullPath,
                            L"%s", IrpContext->SrcFileFullPath.Buffer );

        auto NotifyItem = ( FS_NOTIFY_ITEM* )ExAllocatePool( NonPagedPool, sizeof( FS_NOTIFY_ITEM ) );
        NotifyItem->SendPacket = Packet;

        QueueNotifyEvent( NotifyItem );
    }
    __finally
    {
        // NOTE: 통지 이벤트는 추후 클라이언트에 이벤트를 전송한 후 해당 스레드에서 삭제한다
    }

    return Status;
}

NTSTATUS CheckEventProcCreateTo( ULONG ProcessId, ULONG ParentProcessId, TyGenericBuffer<WCHAR>* ProcessFileFullPath, __in_z const wchar_t* ProcessFileName )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    TyGenericBuffer<MSG_SEND_PACKET> Packet;
    TyGenericBuffer<MSG_REPLY_PACKET> Reply;
    LONG EvtID = CreateEvtID();

    __try
    {
        PFLT_PORT ClientPort = GetClientPort( EvtID );

        if( GlobalContext.Filter == NULL || ClientPort == NULLPTR )
            __leave;

        Reply = AllocateBuffer<MSG_REPLY_PACKET>( BUFFER_MSG_REPLY );
        if( Reply.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        unsigned int SendPacketSize = MSG_SEND_PACKET_SIZE;
        unsigned int CchProcessFullPath = nsUtils::strlength( ProcessFileFullPath->Buffer ) + 1;

        if( Packet.Buffer == NULLPTR || Reply.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        Packet.Buffer->MessageSize = SendPacketSize;
        Packet.Buffer->MessageCate = MSG_CATE_PROCESS;
        Packet.Buffer->MessageType = PROC_WAS_CREATED;
        KeQuerySystemTime( &Packet.Buffer->EventTime );

        Packet.Buffer->ProcessId = ProcessId;
        Packet.Buffer->ThreadId = ( ULONG )PsGetCurrentThreadId();

        Packet.Buffer->Parameters.Process.IsCreate = TRUE;
        Packet.Buffer->Parameters.Process.ProcessId = ProcessId;
        Packet.Buffer->Parameters.Process.ParentProcessId = ParentProcessId;

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", ProcessFileFullPath->Buffer );

        ULONG ReplyLength = Reply.BufferSize;

        Status = FltSendMessage( GlobalContext.Filter, &ClientPort,
                                 Packet.Buffer, Packet.Buffer->MessageSize,
                                 Reply.Buffer, &ReplyLength,
                                 &GlobalContext.TimeOutMs );

        if( ( NT_SUCCESS( Status ) && Status == STATUS_TIMEOUT ) || ( !NT_SUCCESS( Status ) ) )
        {
            KdPrint( ( "[WinIOSol] %s EvtID=%09d %s %s %s Status=0x%08x,%s Proc=%06d,%ws\n"
                       , ">>", EvtID, __FUNCTION__, "FltSendMessage", Status == STATUS_TIMEOUT ? "Timeout" : "FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , ProcessId, ProcessFileName ) );

            if( Status == STATUS_TIMEOUT )
                Status = STATUS_AUDIT_FAILED;

            __leave;
        }
    }
    __finally
    {
        DeallocateBuffer( &Reply );
        DeallocateBuffer( &Packet );
    }

    return Status;
}

NTSTATUS CheckEventProcTerminateTo( ULONG ProcessId, ULONG ParentProcessId, TyGenericBuffer<WCHAR>* ProcessFileFullPath, __in_z const wchar_t* ProcessFileName )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    TyGenericBuffer<MSG_SEND_PACKET> Packet;
    TyGenericBuffer<MSG_REPLY_PACKET> Reply;
    LONG EvtID = CreateEvtID();

    __try
    {
        PFLT_PORT ClientPort = GetClientPort( EvtID );

        if( GlobalContext.Filter == NULL || ClientPort == NULLPTR )
            __leave;

        Reply = AllocateBuffer<MSG_REPLY_PACKET>( BUFFER_MSG_REPLY );
        if( Reply.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        unsigned int SendPacketSize = MSG_SEND_PACKET_SIZE;
        unsigned int CchProcessFullPath = nsUtils::strlength( ProcessFileFullPath->Buffer ) + 1;

        if( Packet.Buffer == NULLPTR || Reply.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        Packet.Buffer->MessageSize = SendPacketSize;
        Packet.Buffer->MessageCate = MSG_CATE_PROCESS;
        Packet.Buffer->MessageType = PROC_WAS_TERMINATED;
        KeQuerySystemTime( &Packet.Buffer->EventTime );

        Packet.Buffer->ProcessId = ProcessId;
        Packet.Buffer->ThreadId = ( ULONG )PsGetCurrentThreadId();

        Packet.Buffer->Parameters.Process.IsCreate = FALSE;
        Packet.Buffer->Parameters.Process.ProcessId = ProcessId;
        Packet.Buffer->Parameters.Process.ParentProcessId = ParentProcessId;

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", ProcessFileFullPath->Buffer );

        ULONG ReplyLength = Reply.BufferSize;

        Status = FltSendMessage( GlobalContext.Filter, &ClientPort,
                                 Packet.Buffer, Packet.Buffer->MessageSize,
                                 Reply.Buffer, &ReplyLength,
                                 &GlobalContext.TimeOutMs );

        if( ( NT_SUCCESS( Status ) && Status == STATUS_TIMEOUT ) || ( !NT_SUCCESS( Status ) ) )
        {
            KdPrint( ( "[WinIOSol] %s EvtID=%09d %s %s %s Status=0x%08x,%s Proc=%06d,%ws\n"
                       , ">>", EvtID, __FUNCTION__, "FltSendMessage", Status == STATUS_TIMEOUT ? "Timeout" : "FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , ProcessId, ProcessFileName ) );

            if( Status == STATUS_TIMEOUT )
                Status = STATUS_AUDIT_FAILED;

            __leave;
        }
    }
    __finally
    {
        DeallocateBuffer( &Reply );
        DeallocateBuffer( &Packet );
    }

    return Status;
}

PFLT_PORT GetClientPort( IRP_CONTEXT* IrpContext )
{
    return GetClientPort( IrpContext->EvtID );
}

PFLT_PORT GetClientPort( LONG Seed )
{
    int salt = 0;
    PFLT_PORT ClientPort = NULLPTR;

    while( ClientPort == NULLPTR )
    {
        auto idx = Seed % MAX_CLIENT_CONNECTION;
        ClientPort = GlobalContext.ClientPort[ idx ];

        if( ClientPort != NULLPTR )
            break;

        while( ClientPort == NULLPTR && salt < MAX_CLIENT_CONNECTION )
            ClientPort = GlobalContext.ClientPort[ salt++ ];
    }

    return ClientPort;
}
