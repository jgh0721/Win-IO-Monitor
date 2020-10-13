#include "fltDirectoryControl.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"
#include "utilities/bufferMgr.hpp"

#include "W32API.hpp"
#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreDirectoryControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                            PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {

    }
    __finally
    {

    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostDirectoryControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                              PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    __try
    {
        if( FileObject == NULLPTR )
            __leave;

        if( Data->IoStatus.Status != STATUS_SUCCESS )
            __leave;

        // MinorFunction == IRP_MN_QUERY_DIRECTORY, IRP_MN_NOTIFY_CHANGE_DIRECTORY 
        if( Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY )
            __leave;

        //
        //  Post operation callbacks may be called at DPC level.  This routine may be
        //  used to transfer completion processing to a "safe" IRQL level.  This
        //  routine will determine if it is safe to call the "SafePostCallback" now
        //  or if it must post the request to a worker thread.  If posting to a worker
        //  thread is needed it determines it is safe to do so (some operations can
        //  not be posted like paging IO).
        //

        if( FltDoCompletionProcessingWhenSafe( Data, FltObjects, CompletionContext, Flags, 
                                               FilterPostDirectoryControlWhenSafe, &FltStatus ) != FALSE )
        {
            __leave;
        }
        else
        {
            Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
            Data->IoStatus.Information = 0;
        }
    }
    __finally
    {

    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FilterPostDirectoryControlWhenSafe( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;
    IRP_CONTEXT*                                IrpContext = NULLPTR;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    __try
    {
        if( FileObject == NULLPTR )
            __leave;

        if( Data->IoStatus.Status != STATUS_SUCCESS )
            __leave;

        // MinorFunction == IRP_MN_QUERY_DIRECTORY, IRP_MN_NOTIFY_CHANGE_DIRECTORY 
        if( Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );

        auto FileInformationClass = (nsW32API::FILE_INFORMATION_CLASS)Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;

        switch( FileInformationClass )
        {
            case FileDirectoryInformation: {
                TuneFileDirectoryInformation( IrpContext );
            } break;
            case FileFullDirectoryInformation: {
                TuneFileFullDirectoryInformation( IrpContext );
            } break;
            case FileBothDirectoryInformation: {
                TuneFileBothDirectoryInformation( IrpContext );
            } break;
            case FileNamesInformation: {} break;
            case FileObjectIdInformation: {} break;
            case FileQuotaInformation: {} break;
            case FileReparsePointInformation: {} break;
            case FileIdBothDirectoryInformation: {
                TuneFileIdBothDirectoryInformation( IrpContext );
            } break;
            case FileIdFullDirectoryInformation: {
                TuneFileIdFullDirectoryInformation( IrpContext );
            } break;
            case FileIdGlobalTxDirectoryInformation: {} break;
            case nsW32API::FileIdExtdDirectoryInformation: {} break;
            case nsW32API::FileIdExtdBothDirectoryInformation: {} break;
        }
    }
    __finally
    {
        if( IrpContext != NULLPTR )
            PrintIrpContext( IrpContext, true );

        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

NTSTATUS TuneFileDirectoryInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = IrpContext->Data->IoStatus.Information;

    __try
    {
        if( IrpContext->UserBuffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            Information = 0;
            __leave;
        }

        auto InfoBuffer = ( FILE_DIRECTORY_INFORMATION* )IrpContext->UserBuffer;

        ULONG Offset = 0;
        ULONG RequiredSize = 0;
        METADATA_DRIVER MetaDataInfo;
        ULONG ConcernedType = 0;

        do
        {
            Offset = InfoBuffer->NextEntryOffset;
            RequiredSize = IrpContext->SrcFileFullPath.BufferSize + InfoBuffer->FileNameLength + sizeof( WCHAR );   // including null char

            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
                IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
            else
            {
                if( IrpContext->DstFileFullPath.BufferSize < RequiredSize )
                {
                    DeallocateBuffer( &IrpContext->DstFileFullPath );
                    IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
                }
            }

            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                Information = 0;
                __leave;
            }

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbPrintfW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize,
                                L"%s\\%.*s", 
                                IrpContext->SrcFileFullPathWOVolume, InfoBuffer->FileNameLength / sizeof(WCHAR), InfoBuffer->FileName );

            ConcernedType = IsConcernedFile( IrpContext, &IrpContext->DstFileFullPath, &MetaDataInfo );
            if( ConcernedType != CONCERNED_NONE )
            {
                switch( ConcernedType )
                {
                    case CONCERNED_SIZE: {
                        CorrectFileSize( IrpContext, &MetaDataInfo, &InfoBuffer->AllocationSize, &InfoBuffer->EndOfFile );
                    } break;
                }
            }

            InfoBuffer = ( FILE_DIRECTORY_INFORMATION* )Add2Ptr( InfoBuffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS TuneFileFullDirectoryInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = IrpContext->Data->IoStatus.Information;

    __try
    {
        if( IrpContext->UserBuffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            Information = 0;
            __leave;
        }

        auto InfoBuffer = ( FILE_FULL_DIR_INFORMATION* )IrpContext->UserBuffer;

        ULONG Offset = 0;
        ULONG RequiredSize = 0;
        METADATA_DRIVER MetaDataInfo;
        ULONG ConcernedType = 0;

        do
        {
            Offset = InfoBuffer->NextEntryOffset;
            RequiredSize = IrpContext->SrcFileFullPath.BufferSize + InfoBuffer->FileNameLength + sizeof( WCHAR );   // including null char

            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
                IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
            else
            {
                if( IrpContext->DstFileFullPath.BufferSize < RequiredSize )
                {
                    DeallocateBuffer( &IrpContext->DstFileFullPath );
                    IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
                }
            }

            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                Information = 0;
                __leave;
            }

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbPrintfW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize,
                                L"%s\\%.*s",
                                IrpContext->SrcFileFullPathWOVolume, InfoBuffer->FileNameLength / sizeof( WCHAR ), InfoBuffer->FileName );

            ConcernedType = IsConcernedFile( IrpContext, &IrpContext->DstFileFullPath, &MetaDataInfo );
            if( ConcernedType != CONCERNED_NONE )
            {
                switch( ConcernedType )
                {
                    case CONCERNED_SIZE:
                    {
                        CorrectFileSize( IrpContext, &MetaDataInfo, &InfoBuffer->AllocationSize, &InfoBuffer->EndOfFile );
                    } break;
                }
            }

            InfoBuffer = ( FILE_FULL_DIR_INFORMATION* )Add2Ptr( InfoBuffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS TuneFileBothDirectoryInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = IrpContext->Data->IoStatus.Information;

    __try
    {
        if( IrpContext->UserBuffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            Information = 0;
            __leave;
        }

        auto InfoBuffer = ( FILE_BOTH_DIR_INFORMATION* )IrpContext->UserBuffer;

        ULONG Offset = 0;
        ULONG RequiredSize = 0;
        METADATA_DRIVER MetaDataInfo;
        ULONG ConcernedType = 0;

        do
        {
            Offset = InfoBuffer->NextEntryOffset;
            RequiredSize = IrpContext->SrcFileFullPath.BufferSize + InfoBuffer->FileNameLength + sizeof( WCHAR );   // including null char

            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
                IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
            else
            {
                if( IrpContext->DstFileFullPath.BufferSize < RequiredSize )
                {
                    DeallocateBuffer( &IrpContext->DstFileFullPath );
                    IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
                }
            }

            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                Information = 0;
                __leave;
            }

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbPrintfW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize,
                                L"%s\\%.*s",
                                IrpContext->SrcFileFullPathWOVolume, InfoBuffer->FileNameLength / sizeof( WCHAR ), InfoBuffer->FileName );

            ConcernedType = IsConcernedFile( IrpContext, &IrpContext->DstFileFullPath, &MetaDataInfo );
            if( ConcernedType != CONCERNED_NONE )
            {
                switch( ConcernedType )
                {
                    case CONCERNED_SIZE:
                    {
                        CorrectFileSize( IrpContext, &MetaDataInfo, &InfoBuffer->AllocationSize, &InfoBuffer->EndOfFile );
                    } break;
                }
            }

            InfoBuffer = ( FILE_BOTH_DIR_INFORMATION* )Add2Ptr( InfoBuffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS TuneFileIdBothDirectoryInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = IrpContext->Data->IoStatus.Information;

    __try
    {
        if( IrpContext->UserBuffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            Information = 0;
            __leave;
        }

        auto InfoBuffer = ( FILE_ID_BOTH_DIR_INFORMATION* )IrpContext->UserBuffer;

        ULONG Offset = 0;
        ULONG RequiredSize = 0;
        METADATA_DRIVER MetaDataInfo;
        ULONG ConcernedType = 0;

        do
        {
            Offset = InfoBuffer->NextEntryOffset;
            RequiredSize = IrpContext->SrcFileFullPath.BufferSize + InfoBuffer->FileNameLength + sizeof( WCHAR );   // including null char

            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
                IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
            else
            {
                if( IrpContext->DstFileFullPath.BufferSize < RequiredSize )
                {
                    DeallocateBuffer( &IrpContext->DstFileFullPath );
                    IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
                }
            }

            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                Information = 0;
                __leave;
            }

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbPrintfW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize,
                                L"%s\\%.*s",
                                IrpContext->SrcFileFullPathWOVolume, InfoBuffer->FileNameLength / sizeof( WCHAR ), InfoBuffer->FileName );

            ConcernedType = IsConcernedFile( IrpContext, &IrpContext->DstFileFullPath, &MetaDataInfo );
            if( ConcernedType != CONCERNED_NONE )
            {
                switch( ConcernedType )
                {
                    case CONCERNED_SIZE:
                    {
                        CorrectFileSize( IrpContext, &MetaDataInfo, &InfoBuffer->AllocationSize, &InfoBuffer->EndOfFile );
                    } break;
                }
            }

            InfoBuffer = ( FILE_ID_BOTH_DIR_INFORMATION* )Add2Ptr( InfoBuffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

NTSTATUS TuneFileIdFullDirectoryInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = IrpContext->Data->IoStatus.Information;

    __try
    {
        if( IrpContext->UserBuffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            Information = 0;
            __leave;
        }

        auto InfoBuffer = ( FILE_ID_FULL_DIR_INFORMATION* )IrpContext->UserBuffer;

        ULONG Offset = 0;
        ULONG RequiredSize = 0;
        METADATA_DRIVER MetaDataInfo;
        ULONG ConcernedType = 0;

        do
        {
            Offset = InfoBuffer->NextEntryOffset;
            RequiredSize = IrpContext->SrcFileFullPath.BufferSize + InfoBuffer->FileNameLength + sizeof( WCHAR );   // including null char

            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
                IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
            else
            {
                if( IrpContext->DstFileFullPath.BufferSize < RequiredSize )
                {
                    DeallocateBuffer( &IrpContext->DstFileFullPath );
                    IrpContext->DstFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
                }
            }

            if( IrpContext->DstFileFullPath.Buffer == NULLPTR )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                Information = 0;
                __leave;
            }

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbPrintfW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize,
                                L"%s\\%.*s",
                                IrpContext->SrcFileFullPathWOVolume, InfoBuffer->FileNameLength / sizeof( WCHAR ), InfoBuffer->FileName );

            ConcernedType = IsConcernedFile( IrpContext, &IrpContext->DstFileFullPath, &MetaDataInfo );
            if( ConcernedType != CONCERNED_NONE )
            {
                switch( ConcernedType )
                {
                    case CONCERNED_SIZE:
                    {
                        CorrectFileSize( IrpContext, &MetaDataInfo, &InfoBuffer->AllocationSize, &InfoBuffer->EndOfFile );
                    } break;
                }
            }

            InfoBuffer = ( FILE_ID_FULL_DIR_INFORMATION* )Add2Ptr( InfoBuffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {
        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, Information );
    }

    return Status;
}

ULONG IsConcernedFile( IRP_CONTEXT* IrpContext, TyGenericBuffer<WCHAR>* FileFullPath, METADATA_DRIVER* MetaDataInfo )
{
    ULONG ConcernedType = CONCERNED_NONE;

    do
    {
        ASSERT( IrpContext != NULLPTR && FileFullPath != NULLPTR && MetaDataInfo != NULLPTR );
        if( IrpContext == NULLPTR || FileFullPath == NULLPTR || MetaDataInfo == NULLPTR )
            break;

        if( nsUtils::EndsWithW( FileFullPath->Buffer, L"\\." ) != NULLPTR ||
            nsUtils::EndsWithW( FileFullPath->Buffer, L"\\.." ) != NULLPTR )
            break;

        //if( nsUtils::stricmp( IrpContext->ProcessFileName, L"Syn.exe" ) == 0 && 
        //    nsUtils::WildcardMatch_straight( FileFullPath->Buffer, L"*metadatatest.txt*" ) == true )
        //{
        //    KdBreakPoint();
        //}

        if( IsOwnFile( IrpContext, FileFullPath->Buffer, MetaDataInfo ) == false )
            break;

        if( MetaDataInfo->MetaData.Type == METADATA_UNK_TYPE )
            break;

        if( MetaDataInfo->MetaData.Type != METADATA_STB_TYPE )
            ConcernedType |= CONCERNED_SIZE;

        if( MetaDataInfo->MetaData.Type == METADATA_STB_TYPE )
        {
            ConcernedType |= CONCERNED_SIZE;
            ConcernedType |= CONCERNED_NAME;
        }
        
    } while( false );

    return ConcernedType;
}

NTSTATUS CorrectFileSize( IRP_CONTEXT* IrpContext, METADATA_DRIVER* MetaDataInfo, LARGE_INTEGER* AllocatoinSize, LARGE_INTEGER* EndOfFile )
{
    NTSTATUS Status = STATUS_SUCCESS;

    do
    {
        ASSERT( IrpContext != NULLPTR );
        ASSERT( MetaDataInfo != NULLPTR );
        ASSERT( AllocatoinSize != NULLPTR && EndOfFile != NULLPTR );
        if( IrpContext == NULLPTR || MetaDataInfo == NULLPTR || AllocatoinSize == NULLPTR || EndOfFile == NULLPTR )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        EndOfFile->QuadPart = MetaDataInfo->MetaData.ContentSize;
        AllocatoinSize->QuadPart = ROUND_TO_SIZE( EndOfFile->QuadPart, IrpContext->InstanceContext->ClusterSize );

    } while( false );

    return Status;
}
