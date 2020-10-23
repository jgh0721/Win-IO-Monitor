#include "Communication.hpp"

#include "callbacks/fltCreateFile.hpp"

#include "irpContext.hpp"
#include "utilities/bufferMgr.hpp"

#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////
/// MSG_FS_TYPE

NTSTATUS CheckEventFileCreateTo( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
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

        Packet.Buffer->ProcessId = IrpContext->ProcessId;
        
        const auto Args = ( CREATE_ARGS* )IrpContext->Params;
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

            __leave;
        }
    }
    __finally
    {
        DeallocateBuffer( &Packet );
    }

    return Status;
}

PFLT_PORT GetClientPort( IRP_CONTEXT* IrpContext )
{
    auto idx = IrpContext->EvtID % MAX_CLIENT_CONNECTION;
    return GlobalContext.ClientPort[ idx ];
}
