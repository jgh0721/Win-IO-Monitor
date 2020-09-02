#include "irpContext.hpp"

#include "utilities/contextMgr.hpp"
#include "utilities/procNameMgr.hpp"
#include "utilities/fltUtilities.hpp"

#include "WinIOMonitor_W32API.hpp"
#include "WinIOMonitor_Event.hpp"
#include "policies/processFilter.hpp"

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
        IrpContext->MsgType     = ConvertIRPMajorFuncToFSTYPE( Data );

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
        else if( MajorFunction == IRP_MJ_SET_INFORMATION )
        {
            Status = CtxGetOrSetContext( FltObjects, FltObjects->FileObject, ( PFLT_CONTEXT* )&IrpContext->StreamContext, FLT_STREAM_CONTEXT );
        }

        if( IrpContext->StreamContext == NULLPTR )
        {
            Status = CtxGetContext( FltObjects, FltObjects->FileObject, FLT_STREAM_CONTEXT, ( PFLT_CONTEXT* )&IrpContext->StreamContext );

            if( !NT_SUCCESS( Status ) || IrpContext->StreamContext == NULLPTR )
            {
                KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d %s IRP=%4s%17s Line=%d %s Status=0x%08x\n",
                             IrpContext->EvtID, __FUNCTION__
                             , IsPreIO == true ? "Pre" : "Post", nsW32API::ConvertIRPMajorFunction( IrpContext->Data->Iopb->MajorFunction )
                             , __LINE__, "CtxGetContext FAILED", Status ) );
            }
        }

        IrpContext->ProcessId = FltGetRequestorProcessId( Data );
        SearchProcessInfo( IrpContext->ProcessId, &IrpContext->ProcessFullPath, &IrpContext->ProcessFileName );

        if( IrpContext->StreamContext != NULLPTR )
        {
            if( IrpContext->StreamContext->FileFullPath.Buffer == NULLPTR )
            {
                IrpContext->StreamContext->FileFullPath = nsUtils::ExtractFileFullPath( FltObjects->FileObject, InstanceContext,
                                                                                        ( MajorFunction == IRP_MJ_CREATE ) && ( IsPreIO == true ) );
            }

            if( IrpContext->StreamContext->FileFullPath.Buffer != NULLPTR )
            {
                IrpContext->SrcFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, IrpContext->StreamContext->FileFullPath.BufferSize );
                RtlStringCbCopyW( IrpContext->SrcFileFullPath.Buffer, IrpContext->SrcFileFullPath.BufferSize, IrpContext->StreamContext->FileFullPath.Buffer );
            }
        }
        else
        {
            IrpContext->SrcFileFullPath = nsUtils::ExtractFileFullPath( FltObjects->FileObject, InstanceContext,
                                                                        ( MajorFunction == IRP_MJ_CREATE ) && ( IsPreIO == true ) );
        }
        
        if( MajorFunction == IRP_MJ_SET_INFORMATION )
        {
            const auto& FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

            if( IsPreIO == true )
            {
                if( FileInformationClass == nsW32API::FileRenameInformation || 
                    FileInformationClass == nsW32API::FileRenameInformationEx )
                {
                    if( FileInformationClass == nsW32API::FileRenameInformation )
                    {
                        auto InfoBuffer = ( nsW32API::FILE_RENAME_INFORMATION* )( Data->Iopb->Parameters.SetFileInformation.InfoBuffer );
                        Status = FltGetDestinationFileNameInformation( FltObjects->Instance, FltObjects->FileObject, InfoBuffer->RootDirectory, 
                                                              InfoBuffer->FileName, InfoBuffer->FileNameLength, 
                                                              FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                                                              &DestinationFileName
                        );
                    }
                    else if( FileInformationClass == nsW32API::FileRenameInformationEx )
                    {
                        auto InfoBuffer = ( nsW32API::FILE_RENAME_INFORMATION_EX* )( Data->Iopb->Parameters.SetFileInformation.InfoBuffer );
                        Status = FltGetDestinationFileNameInformation( FltObjects->Instance, FltObjects->FileObject, InfoBuffer->RootDirectory,
                                                              InfoBuffer->FileName, InfoBuffer->FileNameLength,
                                                              FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                                                              &DestinationFileName
                        );
                    }

                    if( !NT_SUCCESS( Status ) )
                        break;

                    IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, DestinationFileName->Name.Length + sizeof( WCHAR ) );
                    if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    RtlStringCbCopyW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, DestinationFileName->Name.Buffer );
                }

                if( FileInformationClass == nsW32API::FileLinkInformation ||
                    FileInformationClass == nsW32API::FileLinkInformationEx )
                {
                    if( FileInformationClass == nsW32API::FileLinkInformation )
                    {
                        auto InfoBuffer = ( nsW32API::FILE_LINK_INFORMATION* )( Data->Iopb->Parameters.SetFileInformation.InfoBuffer );
                        Status = FltGetDestinationFileNameInformation( FltObjects->Instance, FltObjects->FileObject, InfoBuffer->RootDirectory,
                                                                       InfoBuffer->FileName, InfoBuffer->FileNameLength,
                                                                       FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                                                                       &DestinationFileName
                        );
                    }
                    else if( FileInformationClass == nsW32API::FileLinkInformationEx )
                    {
                        auto InfoBuffer = ( nsW32API::FILE_LINK_INFORMATION_EX* )( Data->Iopb->Parameters.SetFileInformation.InfoBuffer );
                        Status = FltGetDestinationFileNameInformation( FltObjects->Instance, FltObjects->FileObject, InfoBuffer->RootDirectory,
                                                                       InfoBuffer->FileName, InfoBuffer->FileNameLength,
                                                                       FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                                                                       &DestinationFileName
                        );
                    }

                    if( !NT_SUCCESS( Status ) )
                        break;

                    IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, DestinationFileName->Name.Length + sizeof( WCHAR ) );
                    if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    RtlStringCbCopyW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, DestinationFileName->Name.Buffer );
                }

            } // IsPreIO == true
        }

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

    if( IrpContext->StreamContext )
        CtxReleaseContext( IrpContext->StreamContext );

    if( IrpContext->ProcessFilterHandle != NULLPTR )
        ProcessFilter_CloseHandle( IrpContext->ProcessFilterHandle );

    ExFreePool( IrpContext );
    IrpContext = NULLPTR;
}

void CheckEvent( IRP_CONTEXT* IrpContext, PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects )
{
    ASSERT( IrpContext != NULLPTR );
    if( IrpContext == NULLPTR )
        return;

    IrpContext->MsgType = ConvertIRPMajorFuncToFSTYPE( Data );

    NTSTATUS Match = ProcessFilter_Match( IrpContext->ProcessId, IrpContext->ProcessFullPath.Buffer, &IrpContext->ProcessFilterHandle, &IrpContext->ProcessFilterEntry );
    if( Match == STATUS_NOT_FOUND )
        return;

    auto ProcessFilter = ( ( PROCESS_FILTER_ENTRY* )IrpContext->ProcessFilterEntry );

    if( !BooleanFlagOn( ProcessFilter->FileIOFlags, IrpContext->MsgType ) )
        return;

    if( IsListEmpty( &ProcessFilter->ExcludeMaskListHead ) && 
        IsListEmpty( &ProcessFilter->IncludeMaskListHead ) )
    {
        IrpContext->isSendTo = true;
        return;
    }

    Match = ProcessFilter_MatchMask( &ProcessFilter->ExcludeMaskListHead, IrpContext->SrcFileFullPath.Buffer );
    if( Match == STATUS_SUCCESS )
        return;

    if( IrpContext->DstFileFullPath.Buffer != NULLPTR )
    {
        Match = ProcessFilter_MatchMask( &ProcessFilter->ExcludeMaskListHead, IrpContext->DstFileFullPath.Buffer );
        if( Match == STATUS_SUCCESS )
            return;
    }

    Match = ProcessFilter_MatchMask( &ProcessFilter->IncludeMaskListHead, IrpContext->SrcFileFullPath.Buffer );
    if( Match == STATUS_SUCCESS )
    {
        IrpContext->isSendTo = true;
        return;
    }

    if( IrpContext->DstFileFullPath.Buffer != NULLPTR )
    {
        Match = ProcessFilter_MatchMask( &ProcessFilter->IncludeMaskListHead, IrpContext->DstFileFullPath.Buffer );
        if( Match == STATUS_SUCCESS )
        {
            IrpContext->isSendTo = true;
            return;
        }
    }

}

ULONG ConvertIRPMajorFuncToFSTYPE( PFLT_CALLBACK_DATA Data )
{
    ULONG FsType = 0;
    auto IsPreIO = BooleanFlagOn( Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION ) == FALSE;

    if( IsPreIO == true )
    {
        switch( Data->Iopb->MajorFunction )
        {
            case IRP_MJ_CREATE:
                return FS_PRE_CREATE;
            case IRP_MJ_READ:
                return FS_PRE_READ;
            case IRP_MJ_WRITE:
                return FS_PRE_WRITE;
            case IRP_MJ_QUERY_INFORMATION:
                return FS_PRE_QUERY_INFORMATION;
            case IRP_MJ_SET_INFORMATION:
                return FS_PRE_SET_INFORMATION;
            case IRP_MJ_DIRECTORY_CONTROL:
                return FS_PRE_DIRECTORY_CONTROL;
            case IRP_MJ_QUERY_SECURITY:
                return FS_PRE_QUERY_INFORMATION;
            case IRP_MJ_SET_SECURITY:
                return FS_PRE_SET_SECURITY;
            case IRP_MJ_CLEANUP:
                return FS_PRE_CLEANUP;
            case IRP_MJ_CLOSE:
                return FS_PRE_CLOSE;
        }
    }
    else
    {
        switch( Data->Iopb->MajorFunction )
        {
            case IRP_MJ_CREATE:
                return FS_POST_CREATE;
            case IRP_MJ_READ:
                return FS_POST_READ;
            case IRP_MJ_WRITE:
                return FS_POST_WRITE;
            case IRP_MJ_QUERY_INFORMATION:
                return FS_POST_QUERY_INFORMATION;
            case IRP_MJ_SET_INFORMATION:
                return FS_POST_SET_INFORMATION;
            case IRP_MJ_DIRECTORY_CONTROL:
                return FS_POST_DIRECTORY_CONTROL;
            case IRP_MJ_QUERY_SECURITY:
                return FS_POST_QUERY_INFORMATION;
            case IRP_MJ_SET_SECURITY:
                return FS_POST_SET_SECURITY;
            case IRP_MJ_CLEANUP:
                return FS_POST_CLEANUP;
            case IRP_MJ_CLOSE:
                return FS_POST_CLOSE;
        }
    }

    return FsType;
}

void PrintIrpContext( const PIRP_CONTEXT IrpContext )
{
    if( IrpContext == NULLPTR )
        return;

    auto IsPreIO = BooleanFlagOn( IrpContext->Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION ) == FALSE;

    switch( IrpContext->Data->Iopb->MajorFunction )
    {
        case IRP_MJ_CREATE: {

            auto Options = IrpContext->Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;
            auto FileAttributes = IrpContext->Data->Iopb->Parameters.Create.FileAttributes;
            auto ShareAccess = IrpContext->Data->Iopb->Parameters.Create.ShareAccess;

            auto SecurityContext = IrpContext->Data->Iopb->Parameters.Create.SecurityContext;
            auto DesiredAccess = SecurityContext->DesiredAccess;
            auto Disposition = (IrpContext->Data->Iopb->Parameters.Create.Options >> 24 ) & 0x000000ff;

            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d IRP=%4s%-17s Proc=%06d,%ws Src=%ws\n",
                         IrpContext->EvtID
                         , IsPreIO == true ? "Pre" : "Post", nsW32API::ConvertIRPMajorFunction( IrpContext->Data->Iopb->MajorFunction )
                         , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                         , IrpContext->SrcFileFullPath.Buffer == NULLPTR ? L"(null)" : IrpContext->SrcFileFullPath.Buffer
                         ) );

            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d        >> Disposition=%s ShareAccess=%s\n",
                         IrpContext->EvtID
                         , nsW32API::ConvertCreateDisposition( Disposition )
                         , nsW32API::ConvertCreateShareAccess( ShareAccess )
                         ) );

        } break;
        case IRP_MJ_CREATE_NAMED_PIPE: { } break;
        case IRP_MJ_CLOSE: {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d IRP=%4s%-17s Proc=%06d,%ws Src=%ws\n"
                         , IrpContext->EvtID
                         , IsPreIO == true ? "Pre" : "Post", nsW32API::ConvertIRPMajorFunction( IrpContext->Data->Iopb->MajorFunction )
                         , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                         , IrpContext->SrcFileFullPath.Buffer == NULLPTR ? L"(null)" : IrpContext->SrcFileFullPath.Buffer
                         ) );
        } break;
        case IRP_MJ_READ: { } break;
        case IRP_MJ_WRITE: { } break;
        case IRP_MJ_QUERY_INFORMATION: { } break;
        case IRP_MJ_SET_INFORMATION: {
            const auto& FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d IRP=%4s%-17s CLASS=%-45s ResultStatus=0x%08x\n",
                         IrpContext->EvtID
                         , IsPreIO == true ? "Pre" : "Post", nsW32API::ConvertIRPMajorFunction( IrpContext->Data->Iopb->MajorFunction )
                         , nsW32API::ConvertFileInformationClassTo( FileInformationClass )
                         , IrpContext->Data->IoStatus.Status
                         ) );

            if( IsPreIO == true )
            {
                switch( FileInformationClass )
                {
                    case nsW32API::FileRenameInformation: {
                        const auto InfoBuffer = ( nsW32API::FILE_RENAME_INFORMATION* )( IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer );

                        KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d        >> Flags=0x%08x Src=%ws Dst=%ws\n",
                                     IrpContext->EvtID
                                     , InfoBuffer->ReplaceIfExists != FALSE ? FILE_RENAME_REPLACE_IF_EXISTS : FALSE
                                     , IrpContext->SrcFileFullPath.Buffer != NULLPTR ? IrpContext->SrcFileFullPath.Buffer : L"(null)"
                                     , IrpContext->DstFileFullPath.Buffer != NULLPTR ? IrpContext->DstFileFullPath.Buffer : L"(null)"
                                     ) );
                    } break;

                    case nsW32API::FileLinkInformation: {
                        const auto InfoBuffer = ( nsW32API::FILE_LINK_INFORMATION* )( IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer );

                        KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d        >> Flags=0x%08x Src=%ws Dst=%ws\n",
                                     IrpContext->EvtID
                                     , InfoBuffer->ReplaceIfExists != FALSE ? FILE_LINK_REPLACE_IF_EXISTS : FALSE
                                     , IrpContext->SrcFileFullPath.Buffer != NULLPTR ? IrpContext->SrcFileFullPath.Buffer : L"(null)"
                                     , IrpContext->DstFileFullPath.Buffer != NULLPTR ? IrpContext->DstFileFullPath.Buffer : L"(null)"
                                     ) );
                    } break;

                    case nsW32API::FileDispositionInformation: {
                        const auto InfoBuffer = ( nsW32API::FILE_DISPOSITION_INFORMATION* )( IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer );

                        KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d        >> Flags=0x%08x Name=%ws\n"
                                     , IrpContext->EvtID
                                     , InfoBuffer->DeleteFile == TRUE ? FILE_DISPOSITION_DELETE : FILE_DISPOSITION_DO_NOT_DELETE
                                     , IrpContext->SrcFileFullPath.Buffer != NULLPTR ? IrpContext->SrcFileFullPath.Buffer : L"(null)"
                                     ) );

                    } break;

                    case nsW32API::FileAllocationInformation: {
                        const auto InfoBuffer = ( nsW32API::FILE_ALLOCATION_INFORMATION* )( IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer );

                        KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d        >> AllocationSize=%I64d Name=%ws\n"
                                     , IrpContext->EvtID
                                     , InfoBuffer->AllocationSize.QuadPart
                                     , IrpContext->SrcFileFullPath.Buffer != NULLPTR ? IrpContext->SrcFileFullPath.Buffer : L"(null)"
                                     ) );
                    } break;

                    case nsW32API::FileEndOfFileInformation: {
                        const auto InfoBuffer = ( nsW32API::FILE_END_OF_FILE_INFORMATION* )( IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer );

                        KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d        >> EOF=%I64d AdvanceOnly=%d Name=%ws\n"
                                     , IrpContext->EvtID
                                     , InfoBuffer->EndOfFile.QuadPart
                                     , IrpContext->Data->Iopb->Parameters.SetFileInformation.AdvanceOnly
                                     , IrpContext->SrcFileFullPath.Buffer != NULLPTR ? IrpContext->SrcFileFullPath.Buffer : L"(null)"
                                     ) );

                    } break;

                    case nsW32API::FileDispositionInformationEx: {
                        const auto InfoBuffer = ( nsW32API::FILE_DISPOSITION_INFORMATION_EX* )( IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer );

                        KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d        >> Flags=0x%08x Name=%ws\n"
                                     , IrpContext->EvtID
                                     , InfoBuffer->Flags
                                     , IrpContext->SrcFileFullPath.Buffer != NULLPTR ? IrpContext->SrcFileFullPath.Buffer : L"(null)"
                                     ) );

                    } break;

                    case nsW32API::FileRenameInformationEx: {
                        const auto InfoBuffer = ( nsW32API::FILE_RENAME_INFORMATION_EX* )( IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer );

                        KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d        >> Flags=0x%08x Src=%ws Dst=%ws\n",
                                     IrpContext->EvtID
                                     , InfoBuffer->Flags
                                     , IrpContext->SrcFileFullPath.Buffer != NULLPTR ? IrpContext->SrcFileFullPath.Buffer : L"(null)"
                                     , IrpContext->DstFileFullPath.Buffer != NULLPTR ? IrpContext->DstFileFullPath.Buffer : L"(null)"
                                     ) );
                    } break;

                    case nsW32API::FileLinkInformationEx: {
                        const auto InfoBuffer = ( nsW32API::FILE_LINK_INFORMATION_EX* )( IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer );

                        KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d        >> Flags=0x%08x Src=%ws Dst=%ws\n",
                                     IrpContext->EvtID
                                     , InfoBuffer->Flags
                                     , IrpContext->SrcFileFullPath.Buffer != NULLPTR ? IrpContext->SrcFileFullPath.Buffer : L"(null)"
                                     , IrpContext->DstFileFullPath.Buffer != NULLPTR ? IrpContext->DstFileFullPath.Buffer : L"(null)"
                                     ) );
                    } break;

                }
            }   // if IsPreIO == true
            else
            {
                switch( FileInformationClass )
                {
                    case nsW32API::FileRenameInformation: {
                        
                    } break;
                    case nsW32API::FileRenameInformationEx: {
                        
                    } break;
                }
            }

        } break;
        case IRP_MJ_QUERY_EA: { } break;
        case IRP_MJ_SET_EA: { } break;
        case IRP_MJ_FLUSH_BUFFERS: { } break;
        case IRP_MJ_QUERY_VOLUME_INFORMATION: { } break;
        case IRP_MJ_SET_VOLUME_INFORMATION: { } break;
        case IRP_MJ_DIRECTORY_CONTROL: {
            const auto& FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;

            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d IRP=%4s%-17s CLASS=%-45s Proc=%06d,%ws Src=%ws ResultStatus=0x%08x\n",
                         IrpContext->EvtID
                         , IsPreIO == true ? "Pre" : "Post", nsW32API::ConvertIRPMajorFunction( IrpContext->Data->Iopb->MajorFunction )
                         , nsW32API::ConvertFileInformationClassTo( FileInformationClass )
                         , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                         , IrpContext->SrcFileFullPath.Buffer == NULLPTR ? L"(null)" : IrpContext->SrcFileFullPath.Buffer
                         , IrpContext->Data->IoStatus.Status
                         ) );
        } break;
        case IRP_MJ_FILE_SYSTEM_CONTROL: { } break;
        case IRP_MJ_DEVICE_CONTROL: { } break;
        case IRP_MJ_INTERNAL_DEVICE_CONTROL: { } break;
        case IRP_MJ_SHUTDOWN: { } break;
        case IRP_MJ_LOCK_CONTROL: { } break;
        case IRP_MJ_CLEANUP: {
            KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOMon] EvtID=%09d IRP=%4s%-17s Proc=%06d,%ws Src=%ws\n",
                         IrpContext->EvtID
                         , IsPreIO == true ? "Pre" : "Post", nsW32API::ConvertIRPMajorFunction( IrpContext->Data->Iopb->MajorFunction )
                         , IrpContext->ProcessId, IrpContext->ProcessFileName == NULLPTR ? L"(null)" : IrpContext->ProcessFileName
                         , IrpContext->SrcFileFullPath.Buffer == NULLPTR ? L"(null)" : IrpContext->SrcFileFullPath.Buffer
                         ) );
        } break;
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
