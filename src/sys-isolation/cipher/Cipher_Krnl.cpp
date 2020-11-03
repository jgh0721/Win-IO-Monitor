#include "Cipher_Krnl.hpp"

#include "irpContext.hpp"
#include "metadata/Metadata.hpp"
#include "utilities/bufferMgr.hpp"
#include "utilities/contextMgr.hpp"
#include "utilities/fltUtilities.hpp"
#include "utilities/volumeMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

const unsigned int UNIT_SIZE = 1048576;

NTSTATUS CipherFile( USER_FILE_ENCRYPT* opt )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    WCHAR* SrcFileFullPath = ( WCHAR* )Add2Ptr( opt, opt->OffsetOfSrcFileFullPath );
    WCHAR* DstFileFullPath = ( WCHAR* )Add2Ptr( opt, opt->OffsetOfDstFileFullPath );
    IRP_CONTEXT* IrpContext = NULLPTR;

    CTX_INSTANCE_CONTEXT* SrcInstanceContext = NULLPTR;
    CTX_INSTANCE_CONTEXT* DstInstanceContext = NULLPTR;
    HANDLE SrcFileHandle = NULL; FILE_OBJECT* SrcFileObject = NULLPTR;
    HANDLE DstFileHandle = NULL; FILE_OBJECT* DstFileObject = NULLPTR;
    FILE_STANDARD_INFORMATION SrcFileStdInfo;

    IO_STATUS_BLOCK iobs;
    TyGenericBuffer<BYTE> Buffer;
    ULONG LengthReturned = 0;
    METADATA_DRIVER* MetaDataInfo = NULLPTR;

    __try
    {
        IrpContext = ( PIRP_CONTEXT )ExAllocateFromNPagedLookasideList( &GlobalContext.IrpContextLookasideList );
        if( IrpContext == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory( IrpContext, sizeof( IRP_CONTEXT ) );

        IrpContext->EvtID = CreateEvtID();

        SrcInstanceContext = VolumeMgr_SearchContext( SrcFileFullPath );
        DstInstanceContext = VolumeMgr_SearchContext( DstFileFullPath );

        if( IrpContext->InstanceContext == NULLPTR || DstInstanceContext == NULLPTR )
            __leave;

        IrpContext->InstanceContext = SrcInstanceContext;
        Status = FltCreateFileOwn( IrpContext, SrcFileFullPath, &SrcFileHandle, &SrcFileObject, &iobs );
        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltCreateFile Failed"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , SrcFileFullPath
                       ) );
            __leave;
        }

        IrpContext->InstanceContext = DstInstanceContext;
        Status = FltCreateFileOwn( IrpContext, DstFileFullPath, &DstFileHandle, &DstFileObject, &iobs, 
                                   GENERIC_READ | GENERIC_WRITE, 
                                   FILE_ATTRIBUTE_NORMAL, 
                                   0, 
                                   FILE_CREATE );
        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltCreateFile Failed"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , DstFileFullPath
                       ) );
            __leave;
        }

        Status = FltQueryInformationFile( SrcInstanceContext->Instance, SrcFileObject,
                                          &SrcFileStdInfo, sizeof( SrcFileStdInfo ),
                                          FileStandardInformation,
                                          &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile Failed"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , DstFileFullPath
                       ) );
            __leave;
        }

        /*!
            1. StubCode 를 사용한다면 해당 코드 작성
            2. METADATA_INFO 기록
            3. 솔루션 메타데이터 기록( 있다면 )
            4. 파일 읽기
            5. 암호화를 사용한다면 데이터 암호화
            6. 파일 기록
            7. SetEndOfFile 호출
        */

        LARGE_INTEGER FileSize = { 0,0 };
        LARGE_INTEGER ReadOffset = { 0,0 };
        LARGE_INTEGER WriteOffset = { 0, 0 };
        ULONG BytesRead = 0, BytesWritten = 0;

        MetaDataInfo = AllocateMetaDataInfo();
        if( MetaDataInfo == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        InitializeMetaDataInfo( MetaDataInfo );

        MetaDataInfo->MetaData.Type = METADATA_NOR_TYPE;
        MetaDataInfo->MetaData.EncryptMethod = (METADATA_ENCRYPT_METHOD)opt->EncryptConfig.Method;

        MetaDataInfo->MetaData.SolutionMetaDataSize = opt->LengthOfSolutionData;
        MetaDataInfo->MetaData.ContentSize = SrcFileStdInfo.EndOfFile.QuadPart;

        Status = FltWriteFile( DstInstanceContext->Instance, DstFileObject,
                               &WriteOffset, METADATA_DRIVER_SIZE, MetaDataInfo,
                               FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, 
                               &BytesWritten, NULLPTR, NULLPTR );
        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltWriteFile Failed"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , DstFileFullPath
                       ) );
            __leave;
        }

        FileSize.QuadPart += METADATA_DRIVER_SIZE;
        WriteOffset.QuadPart += METADATA_DRIVER_SIZE;
        if( opt->LengthOfSolutionData > 0 && opt->OffsetOfSolutionData > 0 )
        {
            Status = FltWriteFile( DstInstanceContext->Instance, DstFileObject,
                                   &WriteOffset, opt->LengthOfSolutionData, Add2Ptr( opt, opt->OffsetOfSolutionData ),
                                   FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                                   &BytesWritten, NULLPTR, NULLPTR );
            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltWriteFile Failed"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , DstFileFullPath
                           ) );
                __leave;
            }

            FileSize.QuadPart += opt->LengthOfSolutionData;
            WriteOffset.QuadPart += opt->LengthOfSolutionData;
        }

        ULONG ChunkCount = (ULONG)( SrcFileStdInfo.EndOfFile.QuadPart / UNIT_SIZE );
        ULONG RemainBytes = (ULONG)( SrcFileStdInfo.EndOfFile.QuadPart % UNIT_SIZE );
        LARGE_INTEGER AlignedRemainBytes;
        AlignedRemainBytes.QuadPart = ALIGNED( LONGLONG, RemainBytes, DstInstanceContext->BytesPerSector );

        Buffer = AllocateSwapBuffer( BUFFER_SWAP_WRITE, ChunkCount > 0 ? UNIT_SIZE : AlignedRemainBytes.QuadPart );

        for( auto idx = 0; idx < ChunkCount; ++idx )
        {
            Status = FltReadFile( SrcInstanceContext->Instance, SrcFileObject,
                                  &ReadOffset, UNIT_SIZE, Buffer.Buffer,
                                  FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                                  &BytesRead, NULLPTR, NULLPTR );
            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltReadFile Failed"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , DstFileFullPath
                           ) );
                __leave;
            }

            if( MetaDataInfo->MetaData.EncryptMethod != METADATA_ENC_NONE )
            {
                EncryptBuffer( ( CIPHER_ID )opt->EncryptConfig.Method,
                               ReadOffset.QuadPart,
                               Buffer.Buffer, UNIT_SIZE,
                               opt->EncryptConfig.EncryptionKey, opt->EncryptConfig.KeySize,
                               opt->EncryptConfig.IVKey, opt->EncryptConfig.IVSize );
            }

            Status = FltWriteFile( DstInstanceContext->Instance, DstFileObject, 
                                   &WriteOffset, UNIT_SIZE, Buffer.Buffer, 
                                   FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, 
                                   &BytesWritten, NULLPTR, NULLPTR );
            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltWriteFile Failed"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , DstFileFullPath
                           ) );
                __leave;
            }

            ReadOffset.QuadPart += UNIT_SIZE;
            WriteOffset.QuadPart += UNIT_SIZE;
        }

        if( RemainBytes > 0 )
        {
            RtlZeroMemory( Buffer.Buffer, Buffer.BufferSize );

            Status = FltReadFile( SrcInstanceContext->Instance, SrcFileObject,
                                  &ReadOffset, AlignedRemainBytes.QuadPart, Buffer.Buffer,
                                  FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                                  &BytesRead, NULLPTR, NULLPTR );
            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltReadFile Failed"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , DstFileFullPath
                           ) );
                __leave;
            }

            if( MetaDataInfo->MetaData.EncryptMethod != METADATA_ENC_NONE )
            {
                EncryptBuffer( ( CIPHER_ID )opt->EncryptConfig.Method,
                               ReadOffset.QuadPart,
                               Buffer.Buffer, AlignedRemainBytes.QuadPart,
                               opt->EncryptConfig.EncryptionKey, opt->EncryptConfig.KeySize,
                               opt->EncryptConfig.IVKey, opt->EncryptConfig.IVSize );
            }

            Status = FltWriteFile( DstInstanceContext->Instance, DstFileObject,
                                   &WriteOffset, AlignedRemainBytes.QuadPart, Buffer.Buffer,
                                   FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                                   &BytesWritten, NULLPTR, NULLPTR );
            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltWriteFile Failed"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , DstFileFullPath
                           ) );
                __leave;
            }
        }

        FileSize.QuadPart += SrcFileStdInfo.EndOfFile.QuadPart;

        Status = FltSetInformationFile( DstInstanceContext->Instance, DstFileObject,
                                        &FileSize, sizeof( FILE_END_OF_FILE_INFORMATION ), FileEndOfFileInformation );
        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltSetInformationFile Failed"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , DstFileFullPath
                       ) );
            __leave;
        }

    }
    __finally
    {
        if( IrpContext != NULLPTR )
            IrpContext->InstanceContext = NULLPTR;

        UninitializeMetaDataInfo( MetaDataInfo );
        DeallocateBuffer( &Buffer );

        if( SrcFileHandle != NULL )
            FltClose( SrcFileHandle );
        if( SrcFileObject != NULLPTR )
            ObDereferenceObject( SrcFileObject );

        if( DstFileHandle != NULL )
            FltClose( DstFileHandle );
        if( DstFileObject != NULLPTR )
            ObDereferenceObject( DstFileObject );

        CtxReleaseContext( SrcInstanceContext );
        CtxReleaseContext( DstInstanceContext );
        CloseIrpContext( IrpContext );
    }

    return Status;
}

NTSTATUS DecipherFile( USER_FILE_ENCRYPT* opt, PVOID SolutionMetaData, ULONG* SolutionMetaDataSize )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    WCHAR* SrcFileFullPath = ( WCHAR* )Add2Ptr( opt, opt->OffsetOfSrcFileFullPath );
    WCHAR* DstFileFullPath = ( WCHAR* )Add2Ptr( opt, opt->OffsetOfDstFileFullPath );

    IRP_CONTEXT* IrpContext = NULLPTR;

    CTX_INSTANCE_CONTEXT* SrcInstanceContext = NULLPTR;
    CTX_INSTANCE_CONTEXT* DstInstanceContext = NULLPTR;
    HANDLE SrcFileHandle = NULL; FILE_OBJECT* SrcFileObject = NULLPTR;
    HANDLE DstFileHandle = NULL; FILE_OBJECT* DstFileObject = NULLPTR;
    FILE_STANDARD_INFORMATION SrcFileStdInfo;

    IO_STATUS_BLOCK iobs;
    TyGenericBuffer<BYTE> Buffer;
    ULONG LengthReturned = 0;
    METADATA_DRIVER* MetaDataInfo = NULLPTR;

    __try
    {
        IrpContext = ( PIRP_CONTEXT )ExAllocateFromNPagedLookasideList( &GlobalContext.IrpContextLookasideList );
        if( IrpContext == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory( IrpContext, sizeof( IRP_CONTEXT ) );

        IrpContext->EvtID = CreateEvtID();

        SrcInstanceContext = VolumeMgr_SearchContext( SrcFileFullPath );
        DstInstanceContext = VolumeMgr_SearchContext( DstFileFullPath );

        if( IrpContext->InstanceContext == NULLPTR || DstInstanceContext == NULLPTR )
            __leave;

        IrpContext->InstanceContext = SrcInstanceContext;
        Status = FltCreateFileOwn( IrpContext, SrcFileFullPath, &SrcFileHandle, &SrcFileObject, &iobs );
        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltCreateFile Failed"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , SrcFileFullPath
                       ) );
            __leave;
        }

        IrpContext->InstanceContext = DstInstanceContext;
        Status = FltCreateFileOwn( IrpContext, DstFileFullPath, &DstFileHandle, &DstFileObject, &iobs,
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_ATTRIBUTE_NORMAL,
                                   0,
                                   FILE_CREATE );
        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltCreateFile Failed"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , DstFileFullPath
                       ) );
            __leave;
        }

        Status = FltQueryInformationFile( SrcInstanceContext->Instance, SrcFileObject,
                                          &SrcFileStdInfo, sizeof( SrcFileStdInfo ),
                                          FileStandardInformation,
                                          &LengthReturned );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltQueryInformationFile Failed"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , DstFileFullPath
                       ) );
            __leave;
        }

        /*!
            4. 파일 읽기
            5. 암호화를 사용한다면 데이터 복호화
            6. 파일 기록
            7. SetEndOfFile 호출
        */

        MetaDataInfo = AllocateMetaDataInfo();
        auto Type = GetFileMetaDataInfo( SrcFileFullPath, MetaDataInfo );
        if( Type == METADATA_UNK_TYPE )
        {
            Status = STATUS_NOT_SUPPORTED;
            __leave;
        }

        LARGE_INTEGER FileSize = { 0,0 };
        LARGE_INTEGER ReadOffset = { 0,0 };
        LARGE_INTEGER WriteOffset = { 0, 0 };
        ULONG BytesRead = 0, BytesWritten = 0;

        FileSize.QuadPart = MetaDataInfo->MetaData.ContentSize;
        ReadOffset = GetMetaDataOffset( MetaDataInfo );
        if( MetaDataInfo->MetaData.SolutionMetaDataSize > 0 )
            ReadOffset.QuadPart += MetaDataInfo->MetaData.SolutionMetaDataSize;

        ULONG ChunkCount = FileSize.QuadPart / UNIT_SIZE;
        ULONG RemainBytes = FileSize.QuadPart % UNIT_SIZE;
        LARGE_INTEGER AlignedRemainBytes;
        AlignedRemainBytes.QuadPart = ALIGNED( LONGLONG, RemainBytes, DstInstanceContext->BytesPerSector );

        if( SolutionMetaData != NULLPTR && SolutionMetaDataSize != NULLPTR && *SolutionMetaDataSize > 0 )
        {
            LARGE_INTEGER Offset;
            Offset.QuadPart = GetMetaDataOffset( MetaDataInfo ).QuadPart + METADATA_DRIVER_SIZE;
            FltReadFile( SrcInstanceContext->Instance, SrcFileObject, &Offset, 
                         *SolutionMetaDataSize, SolutionMetaData, 
                         FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, 
                         &BytesRead, NULLPTR, NULLPTR );
        }

        Buffer = AllocateSwapBuffer( BUFFER_SWAP_READ, ChunkCount > 0 ? UNIT_SIZE : AlignedRemainBytes.QuadPart );

        for( auto idx = 0; idx < ChunkCount; ++idx )
        {
            Status = FltReadFile( SrcInstanceContext->Instance, SrcFileObject,
                                  &ReadOffset, UNIT_SIZE, Buffer.Buffer,
                                  FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                                  &BytesRead, NULLPTR, NULLPTR );
            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltReadFile Failed"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , DstFileFullPath
                           ) );
                __leave;
            }

            if( MetaDataInfo->MetaData.EncryptMethod != METADATA_ENC_NONE )
            {
                DecryptBuffer( ( CIPHER_ID )opt->EncryptConfig.Method,
                               ReadOffset.QuadPart,
                               Buffer.Buffer, UNIT_SIZE,
                               opt->EncryptConfig.EncryptionKey, opt->EncryptConfig.KeySize,
                               opt->EncryptConfig.IVKey, opt->EncryptConfig.IVSize );
            }

            Status = FltWriteFile( DstInstanceContext->Instance, DstFileObject,
                                   &WriteOffset, UNIT_SIZE, Buffer.Buffer,
                                   FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                                   &BytesWritten, NULLPTR, NULLPTR );
            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltWriteFile Failed"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , DstFileFullPath
                           ) );
                __leave;
            }

            ReadOffset.QuadPart += UNIT_SIZE;
            WriteOffset.QuadPart += UNIT_SIZE;
        }

        if( RemainBytes > 0 )
        {
            RtlZeroMemory( Buffer.Buffer, Buffer.BufferSize );

            Status = FltReadFile( SrcInstanceContext->Instance, SrcFileObject,
                                  &ReadOffset, AlignedRemainBytes.QuadPart, Buffer.Buffer,
                                  FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                                  &BytesRead, NULLPTR, NULLPTR );
            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltReadFile Failed"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , DstFileFullPath
                           ) );
                __leave;
            }

            if( MetaDataInfo->MetaData.EncryptMethod != METADATA_ENC_NONE )
            {
                DecryptBuffer( ( CIPHER_ID )opt->EncryptConfig.Method,
                               ReadOffset.QuadPart,
                               Buffer.Buffer, AlignedRemainBytes.QuadPart,
                               opt->EncryptConfig.EncryptionKey, opt->EncryptConfig.KeySize,
                               opt->EncryptConfig.IVKey, opt->EncryptConfig.IVSize );
            }

            Status = FltWriteFile( DstInstanceContext->Instance, DstFileObject,
                                   &WriteOffset, AlignedRemainBytes.QuadPart, Buffer.Buffer,
                                   FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                                   &BytesWritten, NULLPTR, NULLPTR );
            if( !NT_SUCCESS( Status ) )
            {
                KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                           , IrpContext->EvtID, __FUNCTION__, "FltWriteFile Failed"
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           , DstFileFullPath
                           ) );
                __leave;
            }
        }

        Status = FltSetInformationFile( DstInstanceContext->Instance, DstFileObject,
                                        &FileSize, sizeof( FILE_END_OF_FILE_INFORMATION ), FileEndOfFileInformation );
        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s Name=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltSetInformationFile Failed"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , DstFileFullPath
                       ) );
            __leave;
        }

    }
    __finally
    {
        if( IrpContext != NULLPTR )
            IrpContext->InstanceContext = NULLPTR;

        UninitializeMetaDataInfo( MetaDataInfo );
        DeallocateBuffer( &Buffer );

        if( SrcFileHandle != NULL )
            FltClose( SrcFileHandle );
        if( SrcFileObject != NULLPTR )
            ObDereferenceObject( SrcFileObject );

        if( DstFileHandle != NULL )
            FltClose( DstFileHandle );
        if( DstFileObject != NULLPTR )
            ObDereferenceObject( DstFileObject );

        CtxReleaseContext( SrcInstanceContext );
        CtxReleaseContext( DstInstanceContext );
        CloseIrpContext( IrpContext );
    }

    return Status;
}
