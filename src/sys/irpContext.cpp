#include "irpContext.hpp"

#include "utilities/contextMgr.hpp"
#include "utilities/procNameMgr.hpp"
#include "utilities/fltUtilities.hpp"

#include "WinIOMonitor_W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

LONG volatile GlobalEvtID = 0;

///////////////////////////////////////////////////////////////////////////////

LONG CreateEvtID()
{
    return InterlockedIncrement( &GlobalEvtID );
}

PIRP_CONTEXT CreateIrpContext( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects )
{
    NTSTATUS Status = STATUS_SUCCESS;
    auto IrpContext = (PIRP_CONTEXT) ExAllocatePool( NonPagedPool, sizeof( IRP_CONTEXT ) );

    PFLT_FILE_NAME_INFORMATION DestinationFileName = NULLPTR;
    CTX_INSTANCE_CONTEXT* InstanceContext = NULLPTR;

    do
    {
        if( IrpContext == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory( IrpContext, sizeof( IRP_CONTEXT ) );

        IrpContext->Data        = Data;
        IrpContext->FltObjects  = FltObjects;

        IrpContext->EvtID       = CreateEvtID();

        Status = CtxGetContext( FltObjects, NULLPTR, FLT_INSTANCE_CONTEXT, ( PFLT_CONTEXT* )&InstanceContext );
        if( !NT_SUCCESS( Status ) || InstanceContext == NULLPTR )
        {
            Status = STATUS_INTERNAL_ERROR;
            break;
        }

        const auto& MajorFunction = Data->Iopb->MajorFunction;
        auto IsPreIO = BooleanFlagOn( Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION ) == FALSE;

        //IRP_MJ_CREATE;
        //IRP_MN_QUERY_DIRECTORY;

        if( Data->Iopb->MajorFunction == IRP_MJ_CREATE )
        {
            if( IsPreIO == true )
            {
                Status = CtxAllocateContext( FltObjects->Filter, FLT_STREAM_CONTEXT, ( PFLT_CONTEXT* )&IrpContext->StreamContext );
                if( !NT_SUCCESS( Status ) )
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
            }
        }

        if( IrpContext->StreamContext == NULLPTR )
        {
            CtxGetContext( FltObjects, FltObjects->FileObject, FLT_STREAM_CONTEXT, ( PFLT_CONTEXT* )&IrpContext->StreamContext );
        }

        IrpContext->ProcessId = FltGetRequestorProcessId( Data );
        SearchProcessInfo( IrpContext->ProcessId, &IrpContext->ProcessFullPath, &IrpContext->ProcessFileName );

        if( IrpContext->StreamContext->FileFullPath.Buffer == NULLPTR )
        {
            IrpContext->StreamContext->FileFullPath = nsUtils::ExtractFileFullPath( FltObjects->FileObject, InstanceContext, 
                                                                                    (MajorFunction == IRP_MJ_CREATE) && (IsPreIO == true) );
        }

        if( IrpContext->StreamContext->FileFullPath.Buffer != NULLPTR )
        {
            IrpContext->SrcFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, IrpContext->StreamContext->FileFullPath.BufferSize );
            RtlStringCbCopyW( IrpContext->SrcFileFullPath.Buffer, IrpContext->SrcFileFullPath.BufferSize, IrpContext->StreamContext->FileFullPath.Buffer );
        }
        
        //if( MajorFunction == IRP_MJ_SET_INFORMATION )
        //{
        //    const auto& FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

        //    if( IsPreIO != FALSE )
        //    {
        //        if( FileInformationClass == nsW32API::FileRenameInformation || 
        //            FileInformationClass == nsW32API::FileRenameInformationEx )
        //        {
        //            if( FileInformationClass == nsW32API::FileRenameInformation )
        //            {
        //                auto InfoBuffer = ( nsW32API::FILE_RENAME_INFORMATION* )( Data->Iopb->Parameters.SetFileInformation.InfoBuffer );
        //                Status = FltGetDestinationFileNameInformation( FltObjects->Instance, FltObjects->FileObject, InfoBuffer->RootDirectory, 
        //                                                      InfoBuffer->FileName, InfoBuffer->FileNameLength, 
        //                                                      FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        //                                                      &DestinationFileName
        //                );
        //            }
        //            else if( FileInformationClass == nsW32API::FileRenameInformationEx )
        //            {
        //                auto InfoBuffer = ( nsW32API::FILE_RENAME_INFORMATION_EX* )( Data->Iopb->Parameters.SetFileInformation.InfoBuffer );
        //                Status = FltGetDestinationFileNameInformation( FltObjects->Instance, FltObjects->FileObject, InfoBuffer->RootDirectory,
        //                                                      InfoBuffer->FileName, InfoBuffer->FileNameLength,
        //                                                      FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        //                                                      &DestinationFileName
        //                );
        //            }

        //            if( !NT_SUCCESS( Status ) )
        //                break;

        //            IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, DestinationFileName->Name.Length + sizeof( WCHAR ) );
        //            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
        //            {
        //                Status = STATUS_INSUFFICIENT_RESOURCES;
        //                break;
        //            }

        //            RtlStringCbCopyW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, DestinationFileName->Name.Buffer );
        //        }
        //    }
        //}

    } while( false );

    if( !NT_SUCCESS( Status ) )
    {
        if( IrpContext != NULLPTR )
        {
            ExFreePool( IrpContext );
            IrpContext = NULLPTR;
        }
    }

    if( DestinationFileName != NULLPTR )
        FltReleaseFileNameInformation( DestinationFileName );

    CtxReleaseContext( InstanceContext );

    return IrpContext;
}

void CloseIrpContext( PIRP_CONTEXT& IrpContext )
{
    if( IrpContext == NULLPTR )
        return;

    DeallocateBuffer( &IrpContext->ProcessFullPath );
    DeallocateBuffer( &IrpContext->SrcFileFullPath );
    DeallocateBuffer( &IrpContext->DstFileFullPath );

    if( IrpContext->StreamContext != NULLPTR )
        CtxReleaseContext( IrpContext->StreamContext );

    ExFreePool( IrpContext );
    IrpContext = NULLPTR;
}

void PrintIrpContext( PIRP_CONTEXT IrpContext )
{
    if( IrpContext == NULLPTR )
        return;

    switch( IrpContext->Data->Iopb->MajorFunction )
    {
        case IRP_MJ_CREATE: {

            auto Options = IrpContext->Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;
            auto FileAttributes = IrpContext->Data->Iopb->Parameters.Create.FileAttributes;
            auto ShareAccess = IrpContext->Data->Iopb->Parameters.Create.ShareAccess;

            auto SecurityContext = IrpContext->Data->Iopb->Parameters.Create.SecurityContext;
            auto DesiredAccess = SecurityContext->DesiredAccess;
            auto Disposition = (IrpContext->Data->Iopb->Parameters.Create.Options >> 24 ) & 0x000000ff;

            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d IRP=%17s Proc=%06d,%ws Src=%ws\n",
                         IrpContext->EvtID,
                         nsW32API::ConvertIRPMajorFunction( IrpContext->Data->Iopb->MajorFunction ), 
                         IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName,
                         IrpContext->SrcFileFullPath.Buffer == NULLPTR ? L"(null)" : IrpContext->SrcFileFullPath.Buffer
                         ) );

            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d \t>> Disposition=%s ShareAccess=%s\n",
                         IrpContext->EvtID
                         , nsW32API::ConvertCreateDisposition( Disposition )
                         , nsW32API::ConvertCreateShareAccess( ShareAccess )
                         ) );

        } break;
        case IRP_MJ_CREATE_NAMED_PIPE: { } break;
        case IRP_MJ_CLOSE: { } break;
        case IRP_MJ_READ: { } break;
        case IRP_MJ_WRITE: { } break;
        case IRP_MJ_QUERY_INFORMATION: { } break;
        case IRP_MJ_SET_INFORMATION: { } break;
        case IRP_MJ_QUERY_EA: { } break;
        case IRP_MJ_SET_EA: { } break;
        case IRP_MJ_FLUSH_BUFFERS: { } break;
        case IRP_MJ_QUERY_VOLUME_INFORMATION: { } break;
        case IRP_MJ_SET_VOLUME_INFORMATION: { } break;
        case IRP_MJ_DIRECTORY_CONTROL: { } break;
        case IRP_MJ_FILE_SYSTEM_CONTROL: { } break;
        case IRP_MJ_DEVICE_CONTROL: { } break;
        case IRP_MJ_INTERNAL_DEVICE_CONTROL: { } break;
        case IRP_MJ_SHUTDOWN: { } break;
        case IRP_MJ_LOCK_CONTROL: { } break;
        case IRP_MJ_CLEANUP: { } break;
        case IRP_MJ_CREATE_MAILSLOT: { } break;
        case IRP_MJ_QUERY_SECURITY: { } break;
        case IRP_MJ_SET_SECURITY: { } break;
        case IRP_MJ_POWER: { } break;
        case IRP_MJ_SYSTEM_CONTROL: { } break;
        case IRP_MJ_DEVICE_CHANGE: { } break;
        case IRP_MJ_QUERY_QUOTA: { } break;
        case IRP_MJ_SET_QUOTA: { } break;
        case IRP_MJ_PNP: { } break;
    }

    //KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d IRP=%17s Proc=%06d,%ws \n",
    //             IrpContext->EvtID,
    //             nsW32API::ConvertIRPMajorFunction( IrpContext->Data->Iopb->MajorFunction ), 
    //             IrpContext->ProcessId, IrpContext->ProcessFileName 
    //             ) );

}
