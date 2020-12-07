#include "fltWrite.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"
#include "utilities/bufferMgr.hpp"
#include "metadata/Metadata.hpp"

#include "fltCmnLibs.hpp"
#include "Cipher.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreWrite( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                 PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT* IrpContext = NULLPTR;

    __try
    {
        if( IsOwnFileObject( FltObjects->FileObject ) == false )
            __leave;

        if( FLT_IS_FASTIO_OPERATION( Data ) )
        {
            FltStatus = FLT_PREOP_DISALLOW_FASTIO;
            __leave;
        }

        if( FLT_IS_IRP_OPERATION( Data ) == false )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );

        if( IrpContext != NULLPTR )
            PrintIrpContext( IrpContext );

        auto PagingIo = BooleanFlagOn( Data->Iopb->IrpFlags, IRP_PAGING_IO );
        auto NonCachedIo = BooleanFlagOn( Data->Iopb->IrpFlags, IRP_NOCACHE );
        auto WriteBuffer = nsUtils::MakeUserBuffer( Data );

        if( Data->Iopb->Parameters.Write.Length == 0 )
        {
            AssignCmnResult( IrpContext, STATUS_SUCCESS );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            __leave;
        }

        if( WriteBuffer == NULLPTR )
        {
            AssignCmnResult( IrpContext, STATUS_INVALID_USER_BUFFER );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            __leave;
        }

        if( IrpContext->Fcb->LowerFileObject != NULLPTR && IrpContext->Fcb->LowerFileHandle == NULLPTR )
        {
            IrpContext->Fcb->LowerFileObject = IrpContext->Ccb->LowerFileObject;
            IrpContext->Fcb->LowerFileHandle = IrpContext->Ccb->LowerFileHandle;
        }
        
        if( PagingIo != FALSE )
        {
            WritePagingIO( IrpContext, WriteBuffer );
        }
        else
        {
            if( NonCachedIo != FALSE )
                WriteNonCachedIO( IrpContext, WriteBuffer );
            else
                WriteCachedIO( IrpContext, WriteBuffer );
        }

        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }
    __finally
    {
        if( IrpContext != NULLPTR )
        {
            if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
                FltStatus = IrpContext->PreFltStatus;
        }

        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostWrite( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                   PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}

NTSTATUS WritePagingIO( IRP_CONTEXT* IrpContext, PVOID WriteBuffer )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Write.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Write.ByteOffset;
    auto FileObject = IrpContext->FltObjects->FileObject;

    ULONG BytesWritten = 0;                // FltWriteFile 을 통해 기록한 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;

    if( IsEndOfFile( ByteOffset ) )
    {
        ByteOffset.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
    }

    if( Length > Fcb->AdvFcbHeader.FileSize.QuadPart - ByteOffset.QuadPart )
        Length = Fcb->AdvFcbHeader.FileSize.QuadPart - ByteOffset.QuadPart;

    __try
    {
        if( !BooleanFlagOn( Fcb->Flags, FCB_STATE_PGIO_SHARED ) && !BooleanFlagOn( Fcb->Flags, FCB_STATE_PGIO_EXCLUSIVE ) )
            AcquireCmnResource( IrpContext, FCB_PGIO_EXCLUSIVE );

        SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );

        if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart == Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WritePagingIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite, Length );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = Length;
                    if( BooleanFlagOn( Fcb->Flags, FCB_STATE_PGIO_SHARED ) || BooleanFlagOn( Fcb->Flags, FCB_STATE_PGIO_EXCLUSIVE ) )
                        BytesToCopy = Fcb->AdvFcbHeader.ValidDataLength.QuadPart - ByteOffset.QuadPart;

                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WritePagingIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite, Length );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                AssignCmnResult( IrpContext, STATUS_SUCCESS );
                AssignCmnResultInfo( IrpContext, 0 );
            }
            else
            {
                ASSERT( FALSE );
            }
        }
        else if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WritePagingIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite, Length );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                         ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.ValidDataLength.QuadPart - ByteOffset.QuadPart );
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WritePagingIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite, Length );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &( Fcb->AdvFcbHeader.AllocationSize ) ) );
                            SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
                        }
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.FileSize.QuadPart - ByteOffset.QuadPart );
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WritePagingIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite, Length );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;

                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &( Fcb->AdvFcbHeader.AllocationSize ) ) );
                            SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
                        }
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }
            }
            else if( ByteOffset.QuadPart == Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WritePagingIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite, Length );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &( Fcb->AdvFcbHeader.AllocationSize ) ) );
                            SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
                        }
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.FileSize.QuadPart - ByteOffset.QuadPart );
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WritePagingIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite, Length );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }
            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                     ByteOffset.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart
                     )
            {
                if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                    ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WritePagingIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite, Length );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.FileSize.QuadPart - ByteOffset.QuadPart );
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WritePagingIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite, Length );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                AssignCmnResult( IrpContext, STATUS_SUCCESS );
                AssignCmnResultInfo( IrpContext, 0 );
            }
            else
            {
                ASSERT( FALSE );
            }
        }
        else
        {
            ASSERT( FALSE );
        }

    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return Status;
}

NTSTATUS WriteCachedIO( IRP_CONTEXT* IrpContext, PVOID WriteBuffer )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Write.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Write.ByteOffset;
    auto FileObject = IrpContext->FltObjects->FileObject;

    if( IsEndOfFile( ByteOffset ) )
    {
        ByteOffset.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
    }

    ULONG BytesWritten = 0;                // FltWriteFile 을 통해 기록한 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;

    __try
    {
        if( !CcCanIWrite( FileObject, Length, TRUE, FALSE ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s\n"
                       , IrpContext->EvtID, __FUNCTION__, "CcCanIWrite FAILED" ) );

            Status = STATUS_PENDING;
            AssignCmnResult( IrpContext, Status );
            __leave;
        }

        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

        if( FltCheckLockForWriteAccess( &Fcb->FileLock, IrpContext->Data ) == FALSE )
        {
            Status = STATUS_FILE_LOCK_CONFLICT;
            __leave;
        }

        SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );

        if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart == Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    if( FileObject->PrivateCacheMap == NULL )
                    {
                        CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                              FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );

                        CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                    }

                    Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );

                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                    Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                    Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );;

                    if( FileObject->PrivateCacheMap == NULLPTR )
                    {
                        CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                              FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );
                    }

                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );

                    auto BytesToCopy = Length;
                    auto BytesToWrite = Length;

                    Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, Length );
                        UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }

                        SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                        SetFlag( FileObject->Flags, FCB_STATE_FILE_SIZE_CHANGED );

                        FileObject->CurrentByteOffset = Fcb->AdvFcbHeader.FileSize;
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }

            }
            else if( ByteOffset.QuadPart == Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );

                if( !NT_SUCCESS( Status ) )
                {
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );;

                if( FileObject->PrivateCacheMap == NULLPTR )
                {
                    CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                          FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );
                }

                CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );

                auto BytesToCopy = Length;
                auto BytesToWrite = Length;

                Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                if( NT_SUCCESS( Status ) )
                {
                    AssignCmnResultInfo( IrpContext, Length );

                    SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                    SetFlag( FileObject->Flags, FCB_STATE_FILE_SIZE_CHANGED );
                    FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;

                    UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                    Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                    if( CcIsFileCached( FileObject ) )
                    {
                        CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                    }
                }
            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );

                if( !NT_SUCCESS( Status ) )
                {
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );;

                if( FileObject->PrivateCacheMap == NULLPTR )
                {
                    CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                          FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );
                }

                CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );

                Status = SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, ByteOffset.QuadPart - 1 );
                if( !NT_SUCCESS( Status ) )
                {
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                auto BytesToCopy = Length;
                auto BytesToWrite = Length;

                Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                if( NT_SUCCESS( Status ) )
                {
                    AssignCmnResultInfo( IrpContext, Length );
                    UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                    if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                    {
                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;

                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }
                    }

                    FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;

                    SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                    SetFlag( FileObject->Flags, FCB_STATE_FILE_SIZE_CHANGED );
                }
            }
            else
            {
                ASSERT( FALSE );
            }
        }
        else if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    if( FileObject->PrivateCacheMap == NULL )
                    {
                        CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                              FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );

                        CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                    }

                    auto BytesToCopy = Length;
                    auto BytesToWrite = Length;

                    Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, Length );

                        if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                        {
                            Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }

                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                        SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                         ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart
                         )
                {
                    if( FileObject->PrivateCacheMap == NULL )
                    {
                        CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                              FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );

                        CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                    }

                    auto BytesToCopy = Length;
                    auto BytesToWrite = Length;

                    Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, Length );

                        if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                        {
                            Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }

                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                        SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    Status = SetEndOfFile( IrpContext, ALIGNED( LONGLONG, ByteOffset.QuadPart + Length, Fcb->InstanceContext->ClusterSize ) );

                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                    Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );;

                    if( FileObject->PrivateCacheMap == NULLPTR )
                    {
                        CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                              FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );
                    }

                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );

                    auto BytesToCopy = Length;
                    auto BytesToWrite = Length;

                    Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, Length );
                        UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                        if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                        {
                            Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }

                        SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                        SetFlag( FileObject->Flags, FCB_STATE_FILE_SIZE_CHANGED );

                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }
            }
            else if( ByteOffset.QuadPart == Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    if( FileObject->PrivateCacheMap == NULL )
                    {
                        CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                              FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );

                        CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                    }

                    auto BytesToCopy = Length;
                    auto BytesToWrite = Length;

                    Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, Length );
                        UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                        if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                        {
                            Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }

                        SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                        SetFlag( FileObject->Flags, FCB_STATE_FILE_SIZE_CHANGED );

                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );

                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                    Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );;

                    if( FileObject->PrivateCacheMap == NULL )
                    {
                        CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                              FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );
                    }
                    else
                    {
                        CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                    }

                    auto BytesToCopy = Length;
                    auto BytesToWrite = Length;

                    Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, Length );
                        UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                        if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                        {
                            Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }

                        SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                        SetFlag( FileObject->Flags, FCB_STATE_FILE_SIZE_CHANGED );

                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }
            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                     ByteOffset.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart
                     )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    if( FileObject->PrivateCacheMap == NULLPTR )
                    {
                        CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                              FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );
                    }

                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );

                    Status = SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, ByteOffset.QuadPart - 1 );
                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    auto BytesToCopy = Length;
                    auto BytesToWrite = Length;

                    Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, Length );
                        UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                        if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                        {
                            Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }

                        SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                        SetFlag( FileObject->Flags, FCB_STATE_FILE_SIZE_CHANGED );

                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );

                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                    Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );

                    if( FileObject->PrivateCacheMap == NULLPTR )
                    {
                        CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                              FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );
                    }

                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                    Status = SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, ByteOffset.QuadPart - 1 );
                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    auto BytesToCopy = Length;
                    auto BytesToWrite = Length;

                    Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, Length );
                        UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                        if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                        {
                            Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                        }
                        SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                        SetFlag( FileObject->Flags, FCB_STATE_FILE_SIZE_CHANGED );

                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }
            }
            else if( ByteOffset.QuadPart == Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );

                if( !NT_SUCCESS( Status ) )
                {
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );;

                if( FileObject->PrivateCacheMap == NULL )
                {
                    CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                          FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );

                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );

                    Status = SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, ByteOffset.QuadPart - 1 );
                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }
                }
                else
                {
                    Status = SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, ByteOffset.QuadPart - 1 );
                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                }

                auto BytesToCopy = Length;
                auto BytesToWrite = Length;

                Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                if( NT_SUCCESS( Status ) )
                {
                    AssignCmnResultInfo( IrpContext, Length );
                    UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                    Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );

                    SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                    SetFlag( FileObject->Flags, FCB_STATE_FILE_SIZE_CHANGED );

                    FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                }
            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );

                if( !NT_SUCCESS( Status ) )
                {
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );;

                if( FileObject->PrivateCacheMap == NULLPTR )
                {
                    CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                          FALSE, &GlobalContext.CacheMgrCallbacks, Fcb );
                }

                CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );

                Status = SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, ByteOffset.QuadPart - 1 );
                if( !NT_SUCCESS( Status ) )
                {
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                auto BytesToCopy = Length;
                auto BytesToWrite = Length;

                Status = WriteCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );
                if( NT_SUCCESS( Status ) )
                {
                    AssignCmnResultInfo( IrpContext, Length );
                    UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                    Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );

                    SetFlag( FileObject->Flags, FCB_STATE_FILE_MODIFIED );
                    SetFlag( FileObject->Flags, FCB_STATE_FILE_SIZE_CHANGED );

                    FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                }
            }
            else
            {
                ASSERT( FALSE );
            }
        }
        else
        {
            ASSERT( FALSE );
        }
    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return Status;
}

NTSTATUS WriteNonCachedIO( IRP_CONTEXT* IrpContext, PVOID WriteBuffer )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Write.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Write.ByteOffset;
    auto FileObject = IrpContext->FltObjects->FileObject;

    if( IsEndOfFile( ByteOffset ) )
    {
        ByteOffset.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
    }

    ULONG BytesWritten = 0;                // FltWriteFile 을 통해 기록한 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;

    __try
    {
        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

        if( FltCheckLockForWriteAccess( &Fcb->FileLock, IrpContext->Data ) == FALSE )
        {
            Status = STATUS_FILE_LOCK_CONFLICT;
            __leave;
        }

        if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart == Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WriteNonCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );
                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                    Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );

                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WriteNonCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                        UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &( Fcb->AdvFcbHeader.AllocationSize ) ) );
                        }

                        SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );
                if( !NT_SUCCESS( Status ) )
                {
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );

                Status = SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, ByteOffset.QuadPart - 1 );
                if( !NT_SUCCESS( Status ) )
                {
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                auto BytesToCopy = Length;
                auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                Status = WriteNonCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );

                if( NT_SUCCESS( Status ) )
                {
                    AssignCmnResultInfo( IrpContext, BytesToCopy );

                    Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                    FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                    if( CcIsFileCached( FileObject ) )
                    {
                        CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &( Fcb->AdvFcbHeader.AllocationSize ) ) );
                    }

                    SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
                }
            }
            else
            {
                ASSERT( FALSE );
            }
        }
        else
        {
            ASSERT( Fcb->AdvFcbHeader.ValidDataLength.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart );

            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WriteNonCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                         ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart
                         )
                {
                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WriteNonCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;

                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &( Fcb->AdvFcbHeader.AllocationSize ) ) );
                        }

                        SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );
                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                    Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );

                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WriteNonCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                        UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &( Fcb->AdvFcbHeader.AllocationSize ) ) );
                        }

                        SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                     ByteOffset.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart
                     )
            {
                if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                    ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WriteNonCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Status = SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, ByteOffset.QuadPart - 1 );
                        if( !NT_SUCCESS( Status ) )
                        {
                            AssignCmnResult( IrpContext, Status );
                            __leave;
                        }

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;

                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &( Fcb->AdvFcbHeader.AllocationSize ) ) );
                        }

                        SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );
                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                    Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );

                    Status = SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, ByteOffset.QuadPart - 1 );
                    if( !NT_SUCCESS( Status ) )
                    {
                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    auto BytesToCopy = Length;
                    auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                    Status = WriteNonCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

                        Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                        UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                        if( CcIsFileCached( FileObject ) )
                        {
                            CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &( Fcb->AdvFcbHeader.AllocationSize ) ) );
                        }

                        SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
                    }
                }
                else
                {
                    ASSERT( FALSE );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                Status = SetEndOfFile( IrpContext, ByteOffset.QuadPart + Length );
                if( !NT_SUCCESS( Status ) )
                {
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                Fcb->AdvFcbHeader.FileSize.QuadPart = ByteOffset.QuadPart + Length;
                Fcb->AdvFcbHeader.AllocationSize.QuadPart = ALIGNED( LONGLONG, Fcb->AdvFcbHeader.FileSize.QuadPart, Fcb->InstanceContext->ClusterSize );

                Status = SafeCcZeroData( IrpContext, Fcb->AdvFcbHeader.ValidDataLength.QuadPart, ByteOffset.QuadPart - 1 );
                if( !NT_SUCCESS( Status ) )
                {
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                auto BytesToCopy = Length;
                auto BytesToWrite = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );

                Status = WriteNonCachedIO( IrpContext, WriteBuffer, BytesToCopy, BytesToWrite );

                if( NT_SUCCESS( Status ) )
                {
                    AssignCmnResultInfo( IrpContext, BytesToCopy );

                    Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                    UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

                    if( CcIsFileCached( FileObject ) )
                    {
                        CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &( Fcb->AdvFcbHeader.AllocationSize ) ) );
                    }

                    SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
                }
            }
            else
            {
                ASSERT( FALSE );
            }
        }
    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return Status;
}

NTSTATUS WritePagingIO( IRP_CONTEXT* IrpContext, PVOID WriteBuffer, ULONG BytesToCopy, ULONG BytesToWrite, __in ULONG Length )
{
    auto Fcb = IrpContext->Fcb;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Write.ByteOffset;

    if( IsEndOfFile( ByteOffset ) )
    {
        ByteOffset.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
    }

    ULONG BytesWritten = 0;                // FltWriteFile 을 통해 기록한 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;

    __try
    {
        TySwapBuffer = AllocateSwapBuffer( BUFFER_SWAP_WRITE, BytesToWrite );

        if( TySwapBuffer.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            AssignCmnResult( IrpContext, Status );
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d\n",
                       IrpContext->EvtID, __FUNCTION__, "Allocate Swap Buffer FAILED", __LINE__ ) );
            __leave;
        }

        RtlCopyMemory( TySwapBuffer.Buffer, WriteBuffer, BytesToCopy );

        if( BooleanFlagOn( Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
        {
            ByteOffset.QuadPart += GetHDRSizeFromMetaData( Fcb->MetaDataInfo );
        }

        EncryptBuffer( Fcb->EncryptConfig.CipherID, ByteOffset.QuadPart,
                       TySwapBuffer.Buffer, BytesToWrite,
                       Fcb->EncryptConfig.EncryptionKey, Fcb->EncryptConfig.KeySize,
                       NULLPTR, 0 );

        Status = FltWriteFile( IrpContext->FltObjects->Instance,
                               Fcb->LowerFileObject,
                               &ByteOffset, Length,
                               TySwapBuffer.Buffer, FLTFL_IO_OPERATION_NON_CACHED,
                               &BytesWritten, NULLPTR, NULL );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltWriteFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       ) );
            __leave;
        }

        AssignCmnResult( IrpContext, Status );
    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return Status;
}

NTSTATUS WriteCachedIO( IRP_CONTEXT* IrpContext, PVOID WriteBuffer, ULONG BytesToCopy, ULONG BytesToWrite )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Write.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Write.ByteOffset;

    ULONG BytesWritten = 0;                // FltWriteFile 을 통해 기록한 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;

    __try
    {
        TySwapBuffer = AllocateSwapBuffer( BUFFER_SWAP_WRITE, BytesToWrite );

        if( TySwapBuffer.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            AssignCmnResult( IrpContext, Status );
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d\n",
                       IrpContext->EvtID, __FUNCTION__, "Allocate Swap Buffer FAILED", __LINE__ ) );
            __leave;
        }

        RtlCopyMemory( TySwapBuffer.Buffer, WriteBuffer, BytesToCopy );

        // NOTE: CcCopyWrite 는 메모리 할당에 실패하면 예외를 발생시킨다
        __try
        {
            BOOLEAN IsSuccess = CcCopyWrite( IrpContext->FltObjects->FileObject, 
                                             &ByteOffset, BytesToCopy, 
                                             TRUE, TySwapBuffer.Buffer );

            if( IsSuccess == FALSE )
            {
                KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s\n"
                           , IrpContext->EvtID, __FUNCTION__, "CcCopyWrite FAILED", __LINE__
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           ) );
                __leave;
            }

            Status = STATUS_SUCCESS;
            AssignCmnResult( IrpContext, Status );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            AssignCmnResult( IrpContext, Status );

            KdPrint( ( "[WinIOSol] EvtID=%09d %s ExceptionCode=%d\n"
                       , IrpContext->EvtID, __FUNCTION__, GetExceptionCode()
                       ) );
        }

    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return Status;
}

NTSTATUS WriteNonCachedIO( IRP_CONTEXT* IrpContext, PVOID WriteBuffer, ULONG BytesToCopy, ULONG BytesToWrite )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Write.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Write.ByteOffset;

    ULONG BytesWritten = 0;                // FltWriteFile 을 통해 기록한 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;

    if( IsEndOfFile( ByteOffset ) )
    {
        ByteOffset.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
    }

    __try
    {
        TySwapBuffer = AllocateSwapBuffer( BUFFER_SWAP_WRITE, BytesToWrite );

        if( TySwapBuffer.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            AssignCmnResult( IrpContext, Status );
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d\n",
                       IrpContext->EvtID, __FUNCTION__, "Allocate Swap Buffer FAILED", __LINE__ ) );
            __leave;
        }

        RtlCopyMemory( TySwapBuffer.Buffer, WriteBuffer, BytesToCopy );

        if( BooleanFlagOn( Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
            ByteOffset.QuadPart += GetHDRSizeFromMetaData( Fcb->MetaDataInfo );

        EncryptBuffer( Fcb->EncryptConfig.CipherID, ByteOffset.QuadPart,
                       TySwapBuffer.Buffer, BytesToWrite,
                       Fcb->EncryptConfig.EncryptionKey, Fcb->EncryptConfig.KeySize,
                       NULLPTR, 0 );

        Status = FltWriteFile( IrpContext->FltObjects->Instance,
                               Fcb->LowerFileObject,
                               &ByteOffset, BytesToWrite,
                               TySwapBuffer.Buffer, FLTFL_IO_OPERATION_NON_CACHED,
                               &BytesWritten, NULLPTR, NULL );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltWriteFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       ) );
            __leave;
        }

        AssignCmnResult( IrpContext, Status );
    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return Status;
}

NTSTATUS SetEndOfFile( IRP_CONTEXT* IrpContext, LONGLONG llEndOfFile )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = NULLPTR;

    do
    {
        if( IrpContext == NULLPTR )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if( IsOwnFileObject( IrpContext->FltObjects->FileObject ) == true )
            FileObject = ( ( FCB* )IrpContext->FltObjects->FileObject->FsContext )->LowerFileObject;
        else
            FileObject = IrpContext->FltObjects->FileObject;

        FILE_END_OF_FILE_INFORMATION FileEOF;
        FileEOF.EndOfFile.QuadPart = llEndOfFile;

        if( IrpContext->Fcb != NULLPTR && 
            BooleanFlagOn( IrpContext->Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
        {
            FileEOF.EndOfFile.QuadPart += GetHDRSizeFromMetaData( IrpContext->Fcb->MetaDataInfo );
        }

        Status = FltSetInformationFile( IrpContext->FltObjects->Instance,
                                        FileObject, &FileEOF, sizeof( FILE_END_OF_FILE_INFORMATION ),
                                        FileEndOfFileInformation );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s Src=%ws\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltSetInformationFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       , IrpContext->SrcFileFullPath.Buffer ) );
        }

        // UpdateFileSizeOnMetaData( IrpContext, FileObject, llEndOfFile );

        Status = STATUS_SUCCESS;
    } while( false );

    return Status;
}

NTSTATUS SafeCcZeroData( IRP_CONTEXT* IrpContext, LONGLONG llStartOffset, LONGLONG llEndOffset )
{
    NTSTATUS Status = STATUS_SUCCESS;

    __try
    {
        LARGE_INTEGER StartOffset;
        LARGE_INTEGER EndOffset;

        StartOffset.QuadPart = llStartOffset;
        EndOffset.QuadPart = llEndOffset;

        CcZeroData( IrpContext->FltObjects->FileObject, &StartOffset, &EndOffset, TRUE );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;

        KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Src=%ws\n"
                   , IrpContext->EvtID, __FUNCTION__, "CcZeroData Raise Exception!!!"
                   , IrpContext->SrcFileFullPath.Buffer ) );
    }

    return Status;
}
