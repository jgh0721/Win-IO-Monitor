#include "Communication.hpp"

#include "callbacks/fltCreateFile.hpp"

#include "irpContext.hpp"
#include "utilities/bufferMgr.hpp"
#include "driverMgmt.hpp"
#include "utilities/osInfoMgr.hpp"

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

    ULONG uCount = 1;

    if( nsUtils::VerifyVersionInfoEx( 6, "<" ) == true )
        uCount = KeNumberProcessors;
    else
    {
        uCount = GlobalNtOsKrnlMgr.KeQueryActiveProcessorCount( NULL );
    }

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

    KeSetPriorityThread( KeGetCurrentThread(), HIGH_PRIORITY );

    NTSTATUS Status = STATUS_WAIT_0;
    PVOID WaitObjects[] = { &EVENT_NOTIFY_CONTEXT.NotifyEvent, &EVENT_NOTIFY_CONTEXT.ExitEvent };
    
    while( EVENT_NOTIFY_CONTEXT.IsExit == FALSE )
    {
        // Count 가 THREAD_WAIT_OBJECTS 이하라면 WaitBlockArray 는 NULL 이 가능함
        Status = KeWaitForMultipleObjects( _countof( WaitObjects ), WaitObjects,
                                           WaitAny, Executive, KernelMode,
                                           FALSE, NULL, NULL );
        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> %s KeWaitForMultipleObjects FAILED Status=0x%08x,%s \n", __FUNCTION__,
                       Status, ntkernel_error_category::find_ntstatus( Status )->message ) );

			ASSERT( false );                       
            break;
        }

        if( EVENT_NOTIFY_CONTEXT.IsExit != FALSE )
            break;

        if( Status == STATUS_WAIT_1 )
            break;

        if( Status != STATUS_WAIT_0 )
            continue;

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

    PsTerminateSystemThread( STATUS_SUCCESS );
}

NTSTATUS NotifyFSEventToClient( FS_NOTIFY_ITEM* NotifyEventItem )
{
    NTSTATUS Status = STATUS_SUCCESS;

    do
    {
        if( NotifyEventItem == NULLPTR )
            break;

        if( FeatureContext.IsRunning <= 0 )
            break;

        LONG EvtID = CreateEvtID();
        PFLT_PORT ClientPort = NULLPTR;

        if( NotifyEventItem->SendPacket.Buffer->MessageType == PROC_WAS_CREATED || 
            NotifyEventItem->SendPacket.Buffer->MessageType == PROC_WAS_TERMINATED )
            ClientPort = GetClientPort( EvtID, true );
        else
            ClientPort = GetClientPort( EvtID, false );

        if( GlobalContext.Filter == NULLPTR || ClientPort == NULLPTR )
            break;

        LARGE_INTEGER TimeOutMs;
        TimeOutMs.QuadPart = RELATIVE( MILLISECONDS( 1000 ) );

        Status = FltSendMessage( GlobalContext.Filter, &ClientPort,
                                 NotifyEventItem->SendPacket.Buffer, 
                                 NotifyEventItem->SendPacket.Buffer->MessageSize,
                                 NULL, 0, &TimeOutMs );

        if( NT_SUCCESS( Status ) && Status == STATUS_TIMEOUT )
        {
            KdPrint( ( "[WinIOSol] EVENT %s|FltSendMessage Timeout|Status=0x%08x|PID=%d|TID=%d\n",
                       __FUNCTION__, Status, NotifyEventItem->SendPacket.Buffer->ProcessId, ( ULONG )PsGetCurrentThreadId() ) );
        }

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EVENT FltSendMessage Failed|0x%08x\n", Status ) );
        }

    } while( false );

    if( NotifyEventItem != NULLPTR )
    {
        DeallocateBuffer( &NotifyEventItem->SendPacket );
        ExFreePool( NotifyEventItem );
    }

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

        SendPacketSize += CchProcessFullPath * sizeof( WCHAR );
        SendPacketSize += CchSrcFileFullpath * sizeof( WCHAR );

        Packet = AllocateBuffer<MSG_SEND_PACKET>( BUFFER_MSG_SEND, SendPacketSize );
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
        Packet.Buffer->ThreadId = HandleToUlong(PsGetCurrentThreadId());

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
        Packet.Buffer->OffsetOfSrcFileFullPath = Packet.Buffer->OffsetOfProcessFullPath + Packet.Buffer->LengthOfProcessFullPath;
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfSrcFileFullPath ) ),
                            Packet.Buffer->LengthOfSrcFileFullPath,
                            L"%s", IrpContext->SrcFileFullPath.Buffer );

        if( nsUtils::EndsWithW( IrpContext->ProcessFileName, L"winfstest.exe" ) != NULLPTR )
        {
            IrpContext->Result.Buffer->Result = MSG_JUDGE_ALLOW;
            IrpContext->Result.Buffer->Parameters.CreateResult.IsUseIsolation = TRUE;

            IrpContext->Result.Buffer->Parameters.CreateResult.IsUseContainor = TRUE;
            wcscpy( IrpContext->Result.Buffer->Parameters.CreateResult.NameChangeSuffix, METADATA_DEFAULT_CONTAINOR_SUFFIX );

            IrpContext->Result.Buffer->Parameters.CreateResult.IsUseEncryption = FALSE;

            IrpContext->Result.Buffer->Parameters.CreateResult.IsUseSolutionMetaData = TRUE;
            IrpContext->Result.Buffer->Parameters.CreateResult.SolutionMetaDataSize = 7168;
            IrpContext->Result.Buffer->Parameters.CreateResult.SolutionMetaData;

            Status = STATUS_SUCCESS;
        }
        else
        {
            ULONG ReplyLength = sizeof( MSG_REPLY_PACKET );

            Status = FltSendMessage( GlobalContext.Filter, &ClientPort,
                                     Packet.Buffer, Packet.Buffer->MessageSize,
                                     IrpContext->Result.Buffer, &ReplyLength,
                                     &FeatureContext.TimeOutMs );

            if( ( NT_SUCCESS( Status ) && Status == STATUS_TIMEOUT ) || ( !NT_SUCCESS( Status ) ) )
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
    }
    __finally
    {
        DeallocateBuffer( &Packet );
    }

    return Status;
}

NTSTATUS CheckEventFileCleanup( IRP_CONTEXT* IrpContext )
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

        SendPacketSize += CchProcessFullPath * sizeof( WCHAR );
        SendPacketSize += CchSrcFileFullpath * sizeof( WCHAR );

        Packet = AllocateBuffer<MSG_SEND_PACKET>( BUFFER_MSG_SEND, SendPacketSize );
        if( Packet.Buffer == NULLPTR || IrpContext->Result.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        Packet.Buffer->MessageSize = SendPacketSize;
        Packet.Buffer->MessageCate = MSG_CATE_FILESYSTEM;
        Packet.Buffer->MessageType = FS_PRE_CLEANUP;
        KeQuerySystemTime( &Packet.Buffer->EventTime );

        Packet.Buffer->ProcessId = IrpContext->ProcessId;
        Packet.Buffer->ThreadId = HandleToULong( PsGetCurrentThreadId() );
        Packet.Buffer->Parameters.Clean.IsModified = FlagOn( IrpContext->Fcb->Flags, FCB_STATE_FILE_MODIFIED ) != FALSE;

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", IrpContext->ProcessFullPath.Buffer );

        Packet.Buffer->LengthOfSrcFileFullPath = CchSrcFileFullpath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfSrcFileFullPath = Packet.Buffer->OffsetOfProcessFullPath + Packet.Buffer->LengthOfProcessFullPath;
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfSrcFileFullPath ) ),
                            Packet.Buffer->LengthOfSrcFileFullPath,
                            L"%s", IrpContext->SrcFileFullPath.Buffer );

        ULONG ReplyLength = sizeof( MSG_REPLY_PACKET );

        Status = FltSendMessage( GlobalContext.Filter, &ClientPort,
                                 Packet.Buffer, Packet.Buffer->MessageSize,
                                 IrpContext->Result.Buffer, &ReplyLength,
                                 &FeatureContext.TimeOutMs );

        if( ( NT_SUCCESS( Status ) && Status == STATUS_TIMEOUT ) || ( !NT_SUCCESS( Status ) ) )
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

NTSTATUS CheckEventFileClose( IRP_CONTEXT* IrpContext )
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

        SendPacketSize += CchProcessFullPath * sizeof( WCHAR );
        SendPacketSize += CchSrcFileFullpath * sizeof( WCHAR );

        Packet = AllocateBuffer<MSG_SEND_PACKET>( BUFFER_MSG_SEND, SendPacketSize );
        if( Packet.Buffer == NULLPTR || IrpContext->Result.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        Packet.Buffer->MessageSize = SendPacketSize;
        Packet.Buffer->MessageCate = MSG_CATE_FILESYSTEM;
        Packet.Buffer->MessageType = FS_PRE_CLOSE;
        KeQuerySystemTime( &Packet.Buffer->EventTime );

        Packet.Buffer->ProcessId = IrpContext->ProcessId;
        Packet.Buffer->ThreadId = HandleToUlong( PsGetCurrentThreadId() );
        Packet.Buffer->Parameters.Close.IsModified = FlagOn( IrpContext->Fcb->Flags, FCB_STATE_FILE_MODIFIED ) != FALSE;

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", IrpContext->ProcessFullPath.Buffer );

        Packet.Buffer->LengthOfSrcFileFullPath = CchSrcFileFullpath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfSrcFileFullPath = Packet.Buffer->OffsetOfProcessFullPath + Packet.Buffer->LengthOfProcessFullPath;
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfSrcFileFullPath ) ),
                            Packet.Buffer->LengthOfSrcFileFullPath,
                            L"%s", IrpContext->SrcFileFullPath.Buffer );

        ULONG ReplyLength = sizeof( MSG_REPLY_PACKET );

        Status = FltSendMessage( GlobalContext.Filter, &ClientPort,
                                 Packet.Buffer, Packet.Buffer->MessageSize,
                                 IrpContext->Result.Buffer, &ReplyLength,
                                 &FeatureContext.TimeOutMs );

        if( ( NT_SUCCESS( Status ) && Status == STATUS_TIMEOUT ) || ( !NT_SUCCESS( Status ) ) )
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

        SendPacketSize += CchProcessFullPath * sizeof( WCHAR );
        SendPacketSize += CchSrcFileFullpath * sizeof( WCHAR );
        SendPacketSize += CchDstFileFullpath * sizeof( WCHAR );

        Packet = AllocateBuffer<MSG_SEND_PACKET>( BUFFER_MSG_SEND, SendPacketSize );

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
        Packet.Buffer->ThreadId = HandleToULong( PsGetCurrentThreadId() );

        Packet.Buffer->Parameters.SetFileInformation.FileInformationClass = IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", IrpContext->ProcessFullPath.Buffer );

        Packet.Buffer->LengthOfSrcFileFullPath = CchSrcFileFullpath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfSrcFileFullPath = Packet.Buffer->OffsetOfProcessFullPath + Packet.Buffer->LengthOfProcessFullPath;
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfSrcFileFullPath ) ),
                            Packet.Buffer->LengthOfSrcFileFullPath,
                            L"%s", IrpContext->SrcFileFullPath.Buffer );

        Packet.Buffer->LengthOfDstFileFullPath = CchDstFileFullpath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfDstFileFullPath = Packet.Buffer->OffsetOfSrcFileFullPath + Packet.Buffer->LengthOfSrcFileFullPath;
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

        SendPacketSize += CchProcessFullPath * sizeof( WCHAR );
        SendPacketSize += CchSrcFileFullpath * sizeof( WCHAR );

        Packet = AllocateBuffer<MSG_SEND_PACKET>( BUFFER_MSG_SEND, SendPacketSize );

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
        Packet.Buffer->ThreadId = HandleToULong( PsGetCurrentThreadId() );

        Packet.Buffer->Parameters.SetFileInformation.FileInformationClass = IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", IrpContext->ProcessFullPath.Buffer );

        Packet.Buffer->LengthOfSrcFileFullPath = CchSrcFileFullpath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfSrcFileFullPath = Packet.Buffer->OffsetOfProcessFullPath + Packet.Buffer->LengthOfProcessFullPath;
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

NTSTATUS CheckEventProcCreateTo( ULONG ProcessId, ULONG ParentProcessId, TyGenericBuffer<WCHAR>* ProcessFileFullPath, __in_z const wchar_t* ProcessFileName, TyGenericBuffer<MSG_REPLY_PACKET>* Reply )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    TyGenericBuffer<MSG_SEND_PACKET> Packet;
    LONG EvtID = CreateEvtID();

    __try
    {
        PFLT_PORT ClientPort = GetClientPort( EvtID, true );

        if( GlobalContext.Filter == NULL || ClientPort == NULLPTR || Reply == NULLPTR )
            __leave;

        if( Reply->Buffer == NULLPTR )
            *Reply = AllocateBuffer<MSG_REPLY_PACKET>( BUFFER_MSG_REPLY );

        unsigned int SendPacketSize = sizeof( MSG_SEND_PACKET );
        unsigned int CchProcessFullPath = nsUtils::strlength( ProcessFileFullPath->Buffer ) + 1;

        SendPacketSize += CchProcessFullPath * sizeof( WCHAR );

        Packet = AllocateBuffer<MSG_SEND_PACKET>( BUFFER_MSG_SEND, SendPacketSize );
        if( Packet.Buffer == NULLPTR || Reply->Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        Packet.Buffer->MessageSize = SendPacketSize;
        Packet.Buffer->MessageCate = MSG_CATE_PROCESS;
        Packet.Buffer->MessageType = PROC_WAS_CREATED;
        KeQuerySystemTime( &Packet.Buffer->EventTime );

        Packet.Buffer->ProcessId = ProcessId;
        Packet.Buffer->ThreadId = HandleToULong( PsGetCurrentThreadId() );

        Packet.Buffer->Parameters.Process.IsCreate = TRUE;
        Packet.Buffer->Parameters.Process.ProcessId = ProcessId;
        Packet.Buffer->Parameters.Process.ParentProcessId = ParentProcessId;

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", ProcessFileFullPath->Buffer );

        ULONG ReplyLength = sizeof( MSG_REPLY_PACKET );

        Status = FltSendMessage( GlobalContext.Filter, &ClientPort,
                                 Packet.Buffer, Packet.Buffer->MessageSize,
                                 Reply->Buffer, &ReplyLength,
                                 &FeatureContext.TimeOutMs );

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
        DeallocateBuffer( &Packet );
    }

    return Status;
}

NTSTATUS CheckEventProcTerminateTo( ULONG ProcessId, ULONG ParentProcessId, TyGenericBuffer<WCHAR>* ProcessFileFullPath, __in_z const wchar_t* ProcessFileName )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    TyGenericBuffer<MSG_SEND_PACKET> Packet;
    LONG EvtID = CreateEvtID();

    __try
    {
        PFLT_PORT ClientPort = GetClientPort( EvtID, true );

        if( GlobalContext.Filter == NULL || ClientPort == NULLPTR )
            __leave;

        unsigned int SendPacketSize = sizeof( MSG_SEND_PACKET );
        unsigned int CchProcessFullPath = nsUtils::strlength( ProcessFileFullPath->Buffer ) + 1;

        SendPacketSize += CchProcessFullPath * sizeof( WCHAR );

        Packet = AllocateBuffer<MSG_SEND_PACKET>( BUFFER_MSG_SEND, SendPacketSize );
        if( Packet.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        Packet.Buffer->MessageSize = SendPacketSize;
        Packet.Buffer->MessageCate = MSG_CATE_PROCESS;
        Packet.Buffer->MessageType = PROC_WAS_TERMINATED;
        KeQuerySystemTime( &Packet.Buffer->EventTime );

        Packet.Buffer->ProcessId = ProcessId;
        Packet.Buffer->ThreadId = HandleToULong( PsGetCurrentThreadId() );

        Packet.Buffer->Parameters.Process.IsCreate = FALSE;
        Packet.Buffer->Parameters.Process.ProcessId = ProcessId;
        Packet.Buffer->Parameters.Process.ParentProcessId = ParentProcessId;

        Packet.Buffer->LengthOfProcessFullPath = CchProcessFullPath * sizeof( WCHAR );
        Packet.Buffer->OffsetOfProcessFullPath = sizeof( MSG_SEND_PACKET );
        RtlStringCbPrintfW( ( PWCH )( Add2Ptr( Packet.Buffer, Packet.Buffer->OffsetOfProcessFullPath ) ),
                            Packet.Buffer->LengthOfProcessFullPath,
                            L"%s", ProcessFileFullPath->Buffer );

        auto NotifyItem = ( FS_NOTIFY_ITEM* )ExAllocatePool( NonPagedPool, sizeof( FS_NOTIFY_ITEM ) );
        NotifyItem->SendPacket = Packet;

        QueueNotifyEvent( NotifyItem );
    }
    __finally
    {
    }

    return Status;
}

PFLT_PORT GetClientPort( IRP_CONTEXT* IrpContext, bool IsProcPort /* = false */ )
{
    return GetClientPort( IrpContext->EvtID, IsProcPort );
}

PFLT_PORT GetClientPort( LONG Seed, bool IsProcPort /* = false */ )
{
    int salt = 0;
    PFLT_PORT ClientPort = NULLPTR;

    while( ClientPort == NULLPTR && salt < MAX_CLIENT_CONNECTION )
    {
        auto idx = Seed % MAX_CLIENT_CONNECTION;
        ClientPort = IsProcPort == true ? GlobalContext.ClientProcPort[ idx ] : GlobalContext.ClientPort[ idx ];
        
        if( ClientPort != NULLPTR )
            break;

        while( ClientPort == NULLPTR && salt < MAX_CLIENT_CONNECTION )
            ClientPort = IsProcPort == true ? GlobalContext.ClientProcPort[ salt++ ] : GlobalContext.ClientPort[ salt++ ];
    }

    return ClientPort;
}
