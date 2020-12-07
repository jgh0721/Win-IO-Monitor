#include "fltUtilities.hpp"

#include "bufferMgr.hpp"
#include "volumeMgr.hpp"
#include "contextMgr.hpp"

#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FAST_IO_POSSIBLE CheckIsFastIOPossible( FCB* Fcb )
{
    FAST_IO_POSSIBLE FastIoPossible = FastIoIsNotPossible;

    ASSERT( Fcb != NULLPTR );
    if( Fcb == NULLPTR )
        return FastIoPossible;

    // based on MSDN CDFS sample
    //
    //  The following macro is used to set the fast i/o possible bits in the
    //  FsRtl header.
    //
    //      FastIoIsNotPossible - If the Fcb is bad or there are oplocks on the file.
    //
    //      FastIoIsQuestionable - If there are file locks.
    //
    //      FastIoIsPossible - In all other cases.
    //

    ExAcquireFastMutex( Fcb->AdvFcbHeader.FastMutex );

    do
    {
        if( FltOplockIsFastIoPossible( &Fcb->FileOplock ) )
        {
            if( FsRtlAreThereCurrentFileLocks( &Fcb->FileLock ) != FALSE )
                FastIoPossible = FastIoIsQuestionable;
            else
                FastIoPossible = FastIoIsPossible;
        }
        else
        {
            FastIoPossible = FastIoIsNotPossible;
        }

    } while( false );

    ExReleaseFastMutex( Fcb->AdvFcbHeader.FastMutex );

    return FastIoPossible;
}

PVOID FltMapUserBuffer( PFLT_CALLBACK_DATA Data )
{
    PVOID           UserBuffer = NULLPTR;
    NTSTATUS        Status = STATUS_SUCCESS;

    PMDL*           MdlAddressPointer = NULLPTR;
    PVOID*          BufferPointer = NULLPTR;
    PULONG          Length = NULLPTR;
    LOCK_OPERATION  DesiredAccess;

    do
    {
        Status = FltDecodeParameters( Data, &MdlAddressPointer, &BufferPointer, &Length, &DesiredAccess );
        if( !NT_SUCCESS( Status ) )
            break;

        if( MdlAddressPointer != NULLPTR && *MdlAddressPointer != NULLPTR )
            UserBuffer = MmGetSystemAddressForMdlSafe( *MdlAddressPointer, HighPagePriority );
        else
            UserBuffer = *BufferPointer;
        
    } while( false );

    return UserBuffer;
}

NTSTATUS FltCreateFileOwn( IRP_CONTEXT* IrpContext, const WCHAR* FileFullPath, HANDLE* FileHandle, FILE_OBJECT** FileObject, IO_STATUS_BLOCK* IoStatus, ACCESS_MASK DesiredAccess,  ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, ULONG Flags )
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE FileHandleOwn = NULLPTR;
    FILE_OBJECT* FileObjectOwn = NULLPTR;
    TyGenericBuffer<WCHAR> SrcFileFullPath;
    PFLT_INSTANCE Instance = NULLPTR;

    do
    {
        if( IrpContext == NULLPTR || FileFullPath == NULLPTR || IoStatus == NULLPTR )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if( ARGUMENT_PRESENT( FileHandle ) )
            *FileHandle = NULLPTR;

        if( ARGUMENT_PRESENT( FileObject ) )
            *FileObject = NULLPTR;

        ULONG RequiredSize = nsUtils::strlength( FileFullPath ) + nsUtils::strlength( IrpContext->InstanceContext->DeviceNameBuffer ) + 1;
        RequiredSize *= sizeof( WCHAR );
        SrcFileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
        if( SrcFileFullPath.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        OBJECT_ATTRIBUTES oa;
        UNICODE_STRING uni;

        Instance = IrpContext->InstanceContext->Instance;

        if( nsUtils::StartsWithW( FileFullPath, L"\\Device\\" ) != NULLPTR )
        {
            RtlStringCbCopyW( SrcFileFullPath.Buffer, SrcFileFullPath.BufferSize, FileFullPath );
        }
        else
        {
            if( FileFullPath[0] == L'\\' )
            {
                RtlStringCbCatW( SrcFileFullPath.Buffer, SrcFileFullPath.BufferSize, IrpContext->InstanceContext->DeviceNameBuffer );
                RtlStringCbCatW( SrcFileFullPath.Buffer, SrcFileFullPath.BufferSize, FileFullPath );
            }
            else if( FileFullPath[1] == L':' && FileFullPath[2] == L'\\' )
            {
                // 드라이브 문자에서 INSTANCE_CONTEXT 를 획득한 후 INSTANCE 를 가져온다
                auto InstanceContext = VolumeMgr_SearchContext( FileFullPath[ 0 ] );
                if( InstanceContext == NULLPTR )
                {
                    Status = STATUS_NO_SUCH_DEVICE;
                    break;
                }

                Instance = InstanceContext->Instance;
                RtlStringCbCatW( SrcFileFullPath.Buffer, SrcFileFullPath.BufferSize, IrpContext->InstanceContext->DeviceNameBuffer );
                RtlStringCbCatW( SrcFileFullPath.Buffer, SrcFileFullPath.BufferSize, &FileFullPath[2] );
                CtxReleaseContext( InstanceContext );
            }
        }

        RtlInitUnicodeString( &uni, SrcFileFullPath.Buffer );
        InitializeObjectAttributes( &oa, &uni,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );

        Status = GlobalFltMgr.FltCreateFileEx( GlobalContext.Filter, 
                                               Instance,
                                               &FileHandleOwn, &FileObjectOwn,
                                               DesiredAccess, &oa, IoStatus,
                                               NULLPTR, FileAttributes, ShareAccess, 
                                               CreateDisposition, CreateOptions, 
                                               NULL, 0, 
                                               Flags );

        if( ARGUMENT_PRESENT( FileHandle ) != FALSE )
        {
            if( NT_SUCCESS( Status ) )
                *FileHandle = FileHandleOwn;
            else
                *FileHandle = NULLPTR;
        }
        else
        {
            if( FileHandleOwn != NULLPTR )
                FltClose( FileHandleOwn );
            FileHandleOwn = NULLPTR;
        }

        if( ARGUMENT_PRESENT( FileObject ) != FALSE )
        {
            if( NT_SUCCESS( Status ) )
                *FileObject = FileObjectOwn;
            else
                *FileObject = NULLPTR;
        }
        else
        {
            if( FileObjectOwn != NULLPTR )
                ObDereferenceObject( FileObjectOwn );
            FileObjectOwn = NULLPTR;
        }

    } while( false );

    DeallocateBuffer( &SrcFileFullPath );
    return Status;
}

WCHAR* ExtractFileFullPathWOVolume( CTX_INSTANCE_CONTEXT* InstanceContext, TyGenericBuffer<WCHAR>* FileFullPath )
{
    ASSERT( FileFullPath != NULLPTR );
    ASSERT( FileFullPath->Buffer != NULLPTR );

    return ExtractFileFullPathWOVolume( InstanceContext, FileFullPath->Buffer );
}

WCHAR* ExtractFileFullPathWOVolume( CTX_INSTANCE_CONTEXT* InstanceContext, WCHAR* FileFullPath )
{
    ASSERT( FileFullPath != NULLPTR );

    WCHAR* FileFullPathWOVolume = NULLPTR;

    if( FileFullPath[ 1 ] == L':' )
        FileFullPathWOVolume = &FileFullPath[ 2 ];
    else
    {
        ASSERT( InstanceContext != NULLPTR );
        FileFullPathWOVolume = &FileFullPath[ InstanceContext->DeviceNameCch ];
    }

    if( FileFullPathWOVolume == NULLPTR )
        FileFullPathWOVolume = FileFullPath;

    return FileFullPathWOVolume;
}

TyGenericBuffer<WCHAR> ExtractFileFullPath( PFLT_CALLBACK_DATA Data, __in CTX_INSTANCE_CONTEXT* InstanceContext )
{
    TyGenericBuffer<WCHAR> FileFullPath;
    PFLT_FILE_NAME_INFORMATION fni = NULLPTR;

    do
    {
        // FltGetFileNameInformation return volume path not driver letter
        auto Ret = FltGetFileNameInformation( Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT, &fni );

        if( !NT_SUCCESS( Ret ) )
            break;

        FileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, fni->Name.MaximumLength );
        if( InstanceContext->DriveLetter != L'\0' )
        {
            FileFullPath.Buffer[ 0 ] = InstanceContext->DriveLetter;
            FileFullPath.Buffer[ 1 ] = L':';

            RtlStringCbCatNW( &FileFullPath.Buffer[ 2 ], FileFullPath.BufferSize - ( 2 * sizeof( WCHAR ) ),
                              &fni->Name.Buffer[ InstanceContext->DeviceNameCch ], 
                              fni->Name.Length - ( InstanceContext->DeviceNameCch * sizeof( WCHAR ) ) );
        }

    } while( false );

    if( fni != NULLPTR )
        FltReleaseFileNameInformation( fni );

    return FileFullPath;
}

TyGenericBuffer<WCHAR> ExtractDstFileFullPath( PFLT_INSTANCE Instance, PFILE_OBJECT FileObject, HANDLE RootDirectory,
                                               PWSTR FileName, ULONG FileNameLength )
{
    TyGenericBuffer<WCHAR> FileFullPath;
    PFLT_FILE_NAME_INFORMATION fni = NULLPTR;

    do
    {
        NTSTATUS Status = FltGetDestinationFileNameInformation( Instance, FileObject, RootDirectory,
                                                                FileName, FileNameLength,
                                                                FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                                                                &fni );

        if( NT_SUCCESS( Status ) )
        {
            FileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, fni->Name.Length + sizeof( WCHAR ) +
                                                  ( CONTAINOR_SUFFIX_MAX * sizeof( WCHAR ) ) );

            if( FileFullPath.Buffer == NULLPTR )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlStringCbCopyNW( FileFullPath.Buffer, FileFullPath.BufferSize, fni->Name.Buffer, fni->Name.Length );
        }
        else
        {
            if( FileName != NULLPTR && FileNameLength > 0 && RootDirectory == NULLPTR )
            {
                FileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, FileNameLength );

                if( FileFullPath.Buffer == NULLPTR )
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                int base = 0;
                if( FileName[ 0 ] == L'\\' && FileName[ 1 ] == L'?' && FileName[ 2 ] == L'?' && FileName[ 3 ] == L'\\' )
                    base = 4;

                RtlStringCbCopyNW( FileFullPath.Buffer, FileFullPath.BufferSize, &FileName[base], FileNameLength );
            }
        }

        VolumeMgr_Replace( FileFullPath.Buffer, &FileFullPath.BufferSize );

    } while( false );

    if( fni != NULLPTR )
        FltReleaseFileNameInformation( fni );

    return FileFullPath;
}

CTX_RENLINK_CONTEXT RetrieveRenameLinkContext( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects )
{
    CTX_RENLINK_CONTEXT CtxRenLinkContext;
    RtlZeroMemory( &CtxRenLinkContext, sizeof( CTX_RENLINK_CONTEXT ) );

    do
    {
        if( Data->Iopb->MajorFunction != IRP_MJ_SET_INFORMATION )
            break;

        CtxRenLinkContext.Length                    = Data->Iopb->Parameters.SetFileInformation.Length;
        CtxRenLinkContext.FileInformationClass      = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

        // NOTE: FILE_RENAME_INFORMATION 과 FILE_LINK_INFORMATION 은 구조체가 같다
        switch( CtxRenLinkContext.FileInformationClass )
        {
            case nsW32API::FileRenameInformation:
            case nsW32API::FileLinkInformation: {
                auto InfoBuffer = (FILE_RENAME_INFORMATION*)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

                CtxRenLinkContext.Flags             = InfoBuffer->ReplaceIfExists != FALSE ? FILE_RENAME_REPLACE_IF_EXISTS : 0;
                CtxRenLinkContext.RootDirectory     = InfoBuffer->RootDirectory;
                CtxRenLinkContext.FileNameLength    = InfoBuffer->FileNameLength;
                CtxRenLinkContext.FileName          = InfoBuffer->FileName;

            } break;

            case nsW32API::FileRenameInformationEx:
            case nsW32API::FileLinkInformationEx: {
                auto InfoBuffer = ( nsW32API::FILE_RENAME_INFORMATION_EX* )Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

                CtxRenLinkContext.Flags             = InfoBuffer->Flags;
                CtxRenLinkContext.RootDirectory     = InfoBuffer->RootDirectory;
                CtxRenLinkContext.FileNameLength    = InfoBuffer->FileNameLength;
                CtxRenLinkContext.FileName          = InfoBuffer->FileName;

            } break;
        }
        
    } while( false );

    return CtxRenLinkContext;
}
