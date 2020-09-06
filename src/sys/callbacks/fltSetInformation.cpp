#include "fltSetInformation.hpp"


#include "irpContext.hpp"
#include "notifyMgr.hpp"
#include "WinIOMonitor_Event.hpp"
#include "WinIOMonitor_W32API.hpp"
#include "utilities/contextMgr.hpp"
#include "utilities/fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI WinIOPreSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                         PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    const auto& Parameters = Data->Iopb->Parameters.SetFileInformation;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS ) Parameters.FileInformationClass;

    auto IrpContext = CreateIrpContext( Data, FltObjects );

    if( IrpContext != NULLPTR )
    {
        PrintIrpContext( IrpContext );
        *CompletionContext = IrpContext;
        FltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }

    switch( FileInformationClass )
    {
        case FileBasicInformation: {} break;
        case FileRenameInformation: {} break;
        case FileLinkInformation: {} break;
        case FileDispositionInformation: {} break;
        case FilePositionInformation: {} break;
        case FileAllocationInformation: {} break;
        case FileEndOfFileInformation: {} break;
        case FileValidDataLengthInformation: {} break;
        case nsW32API::FileDispositionInformationEx: {} break;
        case nsW32API::FileRenameInformationEx: {} break;
    }


    switch( FileInformationClass )
    {
        case FileDispositionInformation:
        case nsW32API::FileDispositionInformationEx: {

            //
            //  Race detection logic. The NumOps field in the StreamContext
            //  counts the number of in-flight changes to delete disposition
            //  on the stream.
            //
            //  If there's already some operations in flight, don't bother
            //  doing postop. Since there will be no postop, this value won't
            //  be decremented, staying forever 2 or more, which is one of
            //  the conditions for checking deletion at post-cleanup.
            //
            if( IrpContext != NULLPTR && IrpContext->StreamContext != NULLPTR )
            {
                BOOLEAN race = ( InterlockedIncrement( &IrpContext->StreamContext->NumOps ) > 1 );
            }

        } break;
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI WinIOPostSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                           PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    auto IrpContext = ( PIRP_CONTEXT )CompletionContext;

    const auto& Parameters = Data->Iopb->Parameters.SetFileInformation;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS ) Parameters.FileInformationClass;

    switch( FileInformationClass )
    {
        case FileBasicInformation: {} break;
        case FileRenameInformation: {} break;
        case FileLinkInformation: {} break;
        case FileDispositionInformation: {} break;
        case FilePositionInformation: {} break;
        case FileAllocationInformation: {} break;
        case FileEndOfFileInformation: {} break;
        case FileValidDataLengthInformation: {} break;
        case nsW32API::FileDispositionInformationEx: {} break;
        case nsW32API::FileRenameInformationEx: {} break;
    }

    switch( FileInformationClass )
    {
        case FileRenameInformation: 
        case nsW32API::FileRenameInformationEx: {
            // if file rename was failed, it's ignore
            if( !NT_SUCCESS( Data->IoStatus.Status ) )
                break;

            CheckEvent( IrpContext, IrpContext->Data, IrpContext->FltObjects, FILE_WAS_RENAMED );
            if( IrpContext->isSendTo == true )
            {
                ULONG PacketSize = sizeof( MSG_SEND_PACKET ) + IrpContext->ProcessFullPath.BufferSize + IrpContext->SrcFileFullPath.BufferSize + IrpContext->DstFileFullPath.BufferSize;
                auto NotifyItem = AllocateNotifyItem( PacketSize );

                if( NotifyItem != NULLPTR )
                {
                    auto SendPacket = NotifyItem->SendPacket;

                    SendPacket->MessageSize = PacketSize;
                    SendPacket->MessageCategory = MSG_CATE_FILESYSTEM_NOTIFY;
                    SendPacket->MessageType = FILE_WAS_RENAMED;
                    SendPacket->IsNotified = TRUE;
                    KeQuerySystemTime( &SendPacket->EventTime );

                    SendPacket->ProcessId = IrpContext->ProcessId;

                    SendPacket->LengthOfSrcFileFullPath = ( nsUtils::strlength( IrpContext->SrcFileFullPath.Buffer ) + 1 ) * sizeof( WCHAR );
                    SendPacket->OffsetOfSrcFileFullPath = sizeof( MSG_SEND_PACKET ); 
                    SendPacket->LengthOfDstFileFullPath = ( nsUtils::strlength( IrpContext->DstFileFullPath.Buffer ) + 1 ) * sizeof( WCHAR );
                    SendPacket->OffsetOfDstFileFullPath = SendPacket->OffsetOfSrcFileFullPath + SendPacket->LengthOfSrcFileFullPath;
                    SendPacket->LengthOfProcessFullPath = ( nsUtils::strlength( IrpContext->ProcessFullPath.Buffer ) + 1 ) * sizeof( WCHAR );
                    SendPacket->OffsetOfProcessFullPath = SendPacket->OffsetOfDstFileFullPath + SendPacket->LengthOfDstFileFullPath;
                        
                    RtlStringCbCopyW( ( PWCH )Add2Ptr( SendPacket, SendPacket->OffsetOfSrcFileFullPath ), SendPacket->LengthOfSrcFileFullPath,
                                      IrpContext->SrcFileFullPath.Buffer );
                    RtlStringCbCopyW( ( PWCH )Add2Ptr( SendPacket, SendPacket->OffsetOfDstFileFullPath ), SendPacket->LengthOfDstFileFullPath,
                                      IrpContext->DstFileFullPath.Buffer );
                    RtlStringCbCopyW( ( PWCH )Add2Ptr( SendPacket, SendPacket->OffsetOfProcessFullPath ), SendPacket->LengthOfProcessFullPath,
                                      IrpContext->ProcessFullPath.Buffer );

                    SendPacket->LengthOfContents = 0;

                    AppendNotifyItem( NotifyItem );
                }
            }

        } break;

        case FileDispositionInformation:
        case nsW32API::FileDispositionInformationEx: {

            //
            //  Reaching a postop for FileDispositionInformation/FileDispositionInformationEx means we
            //  MUST have a stream context passed in the CompletionContext.
            //

            if( NT_SUCCESS( Data->IoStatus.Status ) )
            {

                //
                //  No synchronization is needed to set the SetDisp field,
                //  because in case of races, the NumOps field will be perpetually
                //  positive, and it being positive is already an indication this
                //  file is a delete candidate, so it will be checked at post-
                //  -cleanup regardless of the value of SetDisp.
                //

                //
                //  Using FileDispositinInformationEx -
                //    FILE_DISPOSITION_ON_CLOSE controls delete on close
                //    or set disposition behavior. It uses FILE_DISPOSITION_INFORMATION_EX structure.
                //    FILE_DISPOSITION_ON_CLOSE is set - Set or clear DeleteOnClose
                //    depending on FILE_DISPOSITION_DELETE flag.
                //    FILE_DISPOSITION_ON_CLOSE is NOT set - Set or clear disposition information
                //    depending on the flag FILE_DISPOSITION_DELETE.
                //
                //
                //   Using FileDispositionInformation -
                //    Controls only set disposition information behavior. It uses FILE_DISPOSITION_INFORMATION structure.
                //

                if( Data->Iopb->Parameters.SetFileInformation.FileInformationClass == nsW32API::FileDispositionInformationEx )
                {

                    ULONG flags = ( ( nsW32API::PFILE_DISPOSITION_INFORMATION_EX )Data->Iopb->Parameters.SetFileInformation.InfoBuffer )->Flags;

                    if( FlagOn( flags, FILE_DISPOSITION_ON_CLOSE ) )
                    {

                        IrpContext->StreamContext->DeleteOnClose = BooleanFlagOn( flags, FILE_DISPOSITION_DELETE );

                    }
                    else
                    {

                        IrpContext->StreamContext->SetDisp = BooleanFlagOn( flags, FILE_DISPOSITION_DELETE );
                    }

                }
                else
                {

                    IrpContext->StreamContext->SetDisp = ( ( PFILE_DISPOSITION_INFORMATION )Data->Iopb->Parameters.SetFileInformation.InfoBuffer )->DeleteFile;
                }
            }

            //
            //  Now that the operation is over, decrement NumOps.
            //

            InterlockedDecrement( &IrpContext->StreamContext->NumOps );

        } break;
    }

    CloseIrpContext( IrpContext );

    return FLT_POSTOP_FINISHED_PROCESSING;
}
