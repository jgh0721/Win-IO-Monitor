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
    HANDLE FileHandleOwn = NULL;
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
            *FileHandle = NULL;

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
                auto InstanceContext = VolumeMgr_SearchContext( FileFullPath[ 1 ] );
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
                *FileHandle = NULL;
        }
        else
        {
            if( FileHandleOwn != NULL )
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
