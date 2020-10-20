#include "Metadata.hpp"

#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

typedef struct _METADATA_CONTEXT
{
    NPAGED_LOOKASIDE_LIST                   MetaDataLookASideList;

    PVOID                                   StubCodeX86;
    ULONG                                   StubCodeX86Size;
    PVOID                                   StubCodeX64;
    ULONG                                   StubCodeX64Size;

    ERESOURCE                               MetaDataLock;
    LIST_ENTRY                              ListHead;

} METADATA_CONTEXT, *PMETADATA_CONTEXT;

METADATA_CONTEXT MetaDataContext;

NTSTATUS InitializeMetaDataMgr()
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        RtlZeroMemory( &MetaDataContext, sizeof( METADATA_CONTEXT ) );

        Status = ExInitializeResourceLite( &MetaDataContext.MetaDataLock );
        if( !NT_SUCCESS( Status ) )
            break;

        ExInitializeNPagedLookasideList( &MetaDataContext.MetaDataLookASideList, 
                                         NULL, NULL, 0, 
                                         METADATA_DRIVER_SIZE, 0, 0 );

        InitializeListHead( &MetaDataContext.ListHead );

        Status = STATUS_SUCCESS;
        
    } while( false );

    return Status;
}

NTSTATUS UninitializeMetaDataMgr()
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite( &MetaDataContext.MetaDataLock, TRUE );
        // TODO: Free every METADATA_DRIVER struct on ListHead
        ExReleaseResourceLite( &MetaDataContext.MetaDataLock );
        KeLeaveCriticalRegion();

        ExDeleteResourceLite( &MetaDataContext.MetaDataLock );
        ExDeleteNPagedLookasideList( &MetaDataContext.MetaDataLookASideList );

        Status = STATUS_SUCCESS;

    } while( false );

    return Status;
}

METADATA_TYPE GetFileMetaDataInfo( __in IRP_CONTEXT* IrpContext, __in PFILE_OBJECT FileObject, __out_opt METADATA_DRIVER* MetaDataInfo )
{
    NTSTATUS Status = STATUS_SUCCESS;
    METADATA_TYPE MetadataType = METADATA_UNK_TYPE;

    do
    {
        ASSERT( IrpContext != NULLPTR && IrpContext->InstanceContext != NULLPTR );
        if( IrpContext == NULLPTR || IrpContext->InstanceContext == NULLPTR )
            break;

        ASSERT( FileObject != NULLPTR );
        if( FileObject == NULLPTR )
            break;

        BYTE CONTAINOR_MAGIC_NUMBER[ 2 ] = { 0, };
        LARGE_INTEGER READ_OFFSET = { 0, 0 };
        ULONG BytesRead = 0;

        METADATA_DRIVER MetaDataBuffer;
        RtlZeroMemory( &MetaDataBuffer, METADATA_DRIVER_SIZE );

        Status = FltReadFile( IrpContext->FltObjects->Instance, FileObject,
                              &READ_OFFSET, _countof( CONTAINOR_MAGIC_NUMBER ), ( PVOID )&CONTAINOR_MAGIC_NUMBER[ 0 ],
                              FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED,
                              &BytesRead, NULL, NULL );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] %s EvtID=%09d %s %s Status=0x%08x,%s Src=%ws\n",
                       ">>", IrpContext->EvtID, __FUNCTION__, "FltReadFile FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );
            break;
        }

        if( RtlCompareMemory( CONTAINOR_MAGIC_NUMBER, "MZ", 2 ) == 2 )
        {
            FILE_STANDARD_INFORMATION fsi;
            Status = FltQueryInformationFile( IrpContext->FltObjects->Instance, FileObject, &fsi, sizeof( fsi ), FileStandardInformation, &BytesRead );

            ULONG BufferSize = fsi.EndOfFile.QuadPart > METADATA_MAXIMUM_CONTAINOR_SIZE + METADATA_DRIVER_SIZE ? METADATA_MAXIMUM_CONTAINOR_SIZE + METADATA_DRIVER_SIZE : fsi.EndOfFile.QuadPart;
            PBYTE Buffer = (PBYTE)ExAllocatePool( NonPagedPool, BufferSize );

            Status = FltReadFile( IrpContext->FltObjects->Instance, FileObject, &READ_OFFSET, BufferSize, Buffer,
                                  FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED,
                                  &BytesRead, NULL, NULL );

            SIZE_T MatchCount = 0;
            int idx = BufferSize - METADATA_MAGIC_TEXT_SIZE + 1;    // 바로 밑에서 -- 를 하면서 반복문을 시작하기 때문에 1 을 더한다
            while( ( MatchCount != METADATA_MAGIC_TEXT_SIZE ) && ( --idx >= 0 ) )
            {
                MatchCount = RtlCompareMemory( &Buffer[ idx ], METADATA_MAGIC_TEXT, METADATA_MAGIC_TEXT_SIZE );
            }

            if( Buffer != NULLPTR )
                ExFreePool( Buffer );

            if( MatchCount != METADATA_MAGIC_TEXT_SIZE )
                break;

            READ_OFFSET.QuadPart = idx;

            Status = FltReadFile( IrpContext->FltObjects->Instance, FileObject, &READ_OFFSET, METADATA_DRIVER_SIZE,
                                  ( PVOID )&MetaDataBuffer,
                                  FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED,
                                  &BytesRead, NULL, NULL );

            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] %s EvtID=%09d %s %s Status=0x%08x,%s Src=%ws\n",
                           ">>", IrpContext->EvtID, __FUNCTION__, "FltReadFile FAILED"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , IrpContext->SrcFileFullPath.Buffer
                           ) );
                break;
            }
        }
        else
        {
            READ_OFFSET = { 0,0 };

            Status = FltReadFile( IrpContext->FltObjects->Instance, FileObject, &READ_OFFSET, METADATA_DRIVER_SIZE,
                                  ( PVOID )&MetaDataBuffer,
                                  FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED,
                                  &BytesRead, NULL, NULL );

            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] %s EvtID=%09d %s %s Status=0x%08x,%s Src=%ws\n",
                           ">>", IrpContext->EvtID, __FUNCTION__, "FltReadFile FAILED"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , IrpContext->SrcFileFullPath.Buffer
                           ) );
                break;
            }
        }

        if( IsMetaDataDriverInfo( &MetaDataBuffer ) == false )
            break;

        if( ARGUMENT_PRESENT( MetaDataInfo ) )
            RtlCopyMemory( MetaDataInfo, &MetaDataBuffer, METADATA_DRIVER_SIZE );

        MetadataType = MetaDataBuffer.MetaData.Type;

    } while( false );

    return MetadataType;
}

LONGLONG GetFileSizeFromMetaData( METADATA_DRIVER* MetaDataInfo )
{
    ASSERT( MetaDataInfo != NULLPTR );
    if( MetaDataInfo == NULLPTR )
        return 0;

    return GetHDRSizeFromMetaData( MetaDataInfo ) + MetaDataInfo->MetaData.ContentSize;
}

LONGLONG GetHDRSizeFromMetaData( METADATA_DRIVER* MetaDataInfo )
{
    ASSERT( MetaDataInfo != NULLPTR );
    if( MetaDataInfo == NULLPTR )
        return 0;

    return METADATA_DRIVER_SIZE + MetaDataInfo->MetaData.SolutionMetaDataSize + MetaDataInfo->MetaData.ContainorSize;
}

METADATA_DRIVER* AllocateMetaDataInfo()
{
    auto MetaDataInfo = ( METADATA_DRIVER* )ExAllocateFromNPagedLookasideList( &MetaDataContext.MetaDataLookASideList );
    if( MetaDataInfo != NULLPTR )
        RtlZeroMemory( MetaDataInfo, METADATA_DRIVER_SIZE );

    return MetaDataInfo;
}

void InitializeMetaDataInfo( METADATA_DRIVER* MetaDataInfo )
{
    ASSERT( MetaDataInfo != NULLPTR );
    if( MetaDataInfo == NULLPTR )
        return;

    RtlZeroMemory( MetaDataInfo, METADATA_DRIVER_SIZE );
    RtlCopyMemory( MetaDataInfo->MetaData.Magic, METADATA_MAGIC_TEXT, METADATA_MAGIC_TEXT_SIZE );

    MetaDataInfo->MetaData.Version = 1;
    MetaDataInfo->MetaData.Type = METADATA_NOR_TYPE;
}

void UninitializeMetaDataInfo( METADATA_DRIVER*& MetaDataInfo )
{
    if( MetaDataInfo == NULLPTR )
        return;
    
    ExFreeToNPagedLookasideList( &MetaDataContext.MetaDataLookASideList, MetaDataInfo );
    MetaDataInfo = NULLPTR;
}

LARGE_INTEGER GetMetaDataOffset( METADATA_DRIVER* MetaDataInfo )
{
    ASSERT( MetaDataInfo != NULLPTR );
    if( MetaDataInfo == NULLPTR )
        return LARGE_INTEGER{ 0,0 };

    if( MetaDataInfo->MetaData.Type != METADATA_STB_TYPE )
        return LARGE_INTEGER{ 0,0 };

    return LARGE_INTEGER{ MetaDataInfo->MetaData.ContainorSize, 0 };
}

NTSTATUS WriteMetaData( IRP_CONTEXT* IrpContext, PFILE_OBJECT FileObject, METADATA_DRIVER* MetaDataInfo )
{
    NTSTATUS Status = STATUS_SUCCESS;

    do
    {
        FILE_BASIC_INFORMATION fbi;
        ULONG LengthReturned = 0;

        Status = FltQueryInformationFile( IrpContext->FltObjects->Instance,
                                          FileObject, &fbi, sizeof( FILE_BASIC_INFORMATION ), FileBasicInformation,
                                          &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] %s EvtID=%09d %s %s Status=0x%08x,%s Src=%ws\n"
                       , ">>", IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );
            break;
        }

        LARGE_INTEGER FILE_BEGIN_OFFSET = GetMetaDataOffset( MetaDataInfo );
        ULONG BytesWritten = 0;

        Status = FltWriteFile( IrpContext->FltObjects->Instance,
                               FileObject,
                               &FILE_BEGIN_OFFSET, METADATA_DRIVER_SIZE, MetaDataInfo,
                               FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                               &BytesWritten, NULL, NULL );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] %s EvtID=%09d %s %s Status=0x%08x,%s Src=%ws\n"
                       , ">>", IrpContext->EvtID, __FUNCTION__, "FltWriteFile FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );
            break;
        }

        Status = FltSetInformationFile( IrpContext->FltObjects->Instance,
                                        FileObject,
                                        &fbi, sizeof( FILE_BASIC_INFORMATION ),
                                        FileBasicInformation );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] %s EvtID=%09d %s %s Status=0x%08x,%s Src=%ws\n"
                       , ">>", IrpContext->EvtID, __FUNCTION__, "FltSetInformationFile FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer
                       ) );
            break;
        }

    } while( false );

    return Status;
}

NTSTATUS UpdateFileSizeOnMetaData( __in IRP_CONTEXT* IrpContext, PFILE_OBJECT FileObject, const LARGE_INTEGER& FileSize )
{
    return UpdateFileSizeOnMetaData( IrpContext, FileObject, FileSize.QuadPart );
}

NTSTATUS UpdateFileSizeOnMetaData( __in IRP_CONTEXT* IrpContext, PFILE_OBJECT FileObject, LONGLONG FileSize )
{
    NTSTATUS Status = STATUS_SUCCESS;

    do
    {
        if( FileObject == NULLPTR || FileSize < 0 )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        auto Fcb = ( FCB* )FileObject->FsContext;
        if( Fcb->MetaDataInfo == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        if( !BooleanFlagOn( Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrint( ( "[WinIOSol] EvtID=%09d %s ContentSize: %I64d -> %I64d Src=%ws\n",
                   IrpContext->EvtID, __FUNCTION__
                   , Fcb->MetaDataInfo->MetaData.ContentSize
                   , FileSize
                   , Fcb->FileFullPath.Buffer
                   ) );

        if( Fcb->MetaDataInfo->MetaData.ContentSize != FileSize )
        {
            Fcb->MetaDataInfo->MetaData.ContentSize = FileSize;

            auto Ccb = ( CCB* )FileObject->FsContext2;
            SetFlag( Ccb->Flags, CCB_STATE_SIZE_CHAGNED );
        }

        Status = STATUS_SUCCESS;

    } while( false );

    return Status;
}

bool IsMetaDataDriverInfo( PVOID Buffer )
{
    ASSERT( Buffer != NULLPTR );
    if( Buffer == NULLPTR )
        return false;

    const auto MetaDataInfo = ( METADATA_DRIVER* )Buffer;
    if( RtlCompareMemory( MetaDataInfo->MetaData.Magic, METADATA_MAGIC_TEXT, METADATA_MAGIC_TEXT_SIZE ) != METADATA_MAGIC_TEXT_SIZE )
        return false;

    if( MetaDataInfo->MetaData.Version == 0 )
        return false;

    if( MetaDataInfo->MetaData.Type == METADATA_UNK_TYPE )
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// StubCode Management

void SetMetaDataStubCode( PVOID StubCodeX86, ULONG StubCodeX86Size, PVOID StubCodeX64, ULONG StubCodeX64Size )
{
    if( StubCodeX86 != NULLPTR && StubCodeX86 > 0 )
    {
        MetaDataContext.StubCodeX86Size = ROUND_TO_SIZE( StubCodeX86Size, 512 );
        MetaDataContext.StubCodeX86 = ExAllocatePoolWithTag( NonPagedPool, StubCodeX86Size, POOL_MAIN_TAG );

        RtlZeroMemory( MetaDataContext.StubCodeX86, MetaDataContext.StubCodeX86Size );
        RtlCopyMemory( MetaDataContext.StubCodeX86, StubCodeX86, StubCodeX86Size );
        KdPrint( ( "[WinIOSol] %s StubCodeX86 = %p|%d|%d\n"
                   , __FUNCTION__, MetaDataContext.StubCodeX86, MetaDataContext.StubCodeX86Size, StubCodeX86Size ) );
    }

    if( StubCodeX64 != NULLPTR && StubCodeX64Size > 0 )
    {
        MetaDataContext.StubCodeX64Size = ROUND_TO_SIZE( StubCodeX64Size, 512 );
        MetaDataContext.StubCodeX64 = ExAllocatePoolWithTag( NonPagedPool, StubCodeX64Size, POOL_MAIN_TAG );

        RtlZeroMemory( MetaDataContext.StubCodeX64, MetaDataContext.StubCodeX64Size );
        RtlCopyMemory( MetaDataContext.StubCodeX64, StubCodeX64, StubCodeX64Size );
        KdPrint( ( "[WinIOSol] %s StubCodeX64 = %p|%d|%d\n"
                   , __FUNCTION__, MetaDataContext.StubCodeX64, MetaDataContext.StubCodeX64Size, StubCodeX64Size ) );
    }
}

PVOID GetStubCodeX86()
{
    return MetaDataContext.StubCodeX86;
}

ULONG GetStubCodeX86Size()
{
    return MetaDataContext.StubCodeX86Size;
}

PVOID GetStubCodeX64()
{
    return MetaDataContext.StubCodeX64;
}

ULONG GetStubCodeX64Size()
{
    return MetaDataContext.StubCodeX64Size;
}
