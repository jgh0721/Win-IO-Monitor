#include "irpContext.hpp"

#include "utilities/procNameMgr.hpp"
#include "utilities/contextMgr.hpp"
#include "utilities/contextMgr_Defs.hpp"

#include "fltCmnLibs_path.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

LONG volatile GlobalEvtID;

///////////////////////////////////////////////////////////////////////////////

LONG CreateEvtID()
{
    return InterlockedIncrement( &GlobalEvtID );
}

PIRP_CONTEXT CreateIrpContext( __in PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PIRP_CONTEXT            IrpContext = NULLPTR;
    const auto&             MajorFunction = Data->Iopb->MajorFunction;
    auto                    IsPreIO = BooleanFlagOn( Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION ) == FALSE;
    CTX_INSTANCE_CONTEXT*   InstanceContext = NULLPTR;
    PFLT_FILE_NAME_INFORMATION DestinationFileName = NULLPTR;

    __try
    {

        Status = CtxGetContext( FltObjects, NULLPTR, FLT_INSTANCE_CONTEXT, ( PFLT_CONTEXT* )&InstanceContext );
        if( !NT_SUCCESS( Status ) || InstanceContext == NULLPTR )
        {
            Status = STATUS_INTERNAL_ERROR;
            __leave;
        }

        IrpContext = ( PIRP_CONTEXT )ExAllocateFromNPagedLookasideList( &GlobalContext.IrpContextLookasideList );
        if( IrpContext == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory( IrpContext, sizeof( IRP_CONTEXT ) );

        IrpContext->Data            = Data;
        IrpContext->FltObjects      = FltObjects;

        IrpContext->EvtID           = CreateEvtID();
        IrpContext->InstanceContext = InstanceContext;
        FltReferenceContext( IrpContext->InstanceContext );

        IrpContext->ProcessId       = FltGetRequestorProcessId( Data );
        SearchProcessInfo( IrpContext->ProcessId, &IrpContext->ProcessFullPath, &IrpContext->ProcessFileName );

        IrpContext->SrcFileFullPath = nsUtils::ExtractFileFullPath( FltObjects->FileObject, InstanceContext,
                                                                    ( MajorFunction == IRP_MJ_CREATE ) && ( IsPreIO == true ) );

        switch( MajorFunction )
        {
            case IRP_MJ_SET_INFORMATION: {
                if( IsPreIO == false )
                    break;

                const auto& FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
                switch( FileInformationClass )
                {
                    case nsW32API::FileRenameInformation:
                    case nsW32API::FileRenameInformationEx: {

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

                    } break;
                } // switch FileInformationClass

            } break;
        } // switch MajorFunction

    }
    __finally
    {
        CtxReleaseContext( InstanceContext );

        if( DestinationFileName != NULLPTR )
            FltReleaseFileNameInformation( DestinationFileName );
    }

    return IrpContext;
}

VOID CloseIrpContext( __in PIRP_CONTEXT IrpContext )
{
    
}