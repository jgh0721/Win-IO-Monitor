#include "fltRead.hpp"

#include "privateFCBMgr.hpp"
#include "irpContext.hpp"
#include "utilities/bufferMgr.hpp"

#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreRead( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;

    UNREFERENCED_PARAMETER( CompletionContext );

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
        auto ReadBuffer = nsUtils::MakeUserBuffer( Data );

        if( ReadBuffer == NULLPTR )
        {
            AssignCmnResult( IrpContext, STATUS_INVALID_USER_BUFFER );
            __leave;
        }

        if( PagingIo != FALSE )
        {
            ReadPagingIO( IrpContext, ReadBuffer );
        }
        else
        {
            if( NonCachedIo != FALSE )
                ReadNonCachedIO( IrpContext, ReadBuffer );
            else
                ReadCachedIO( IrpContext, ReadBuffer );
        }

        auto FileObject = FltObjects->FileObject;
        auto Fcb = ( FCB* )FileObject->FsContext;
        auto& Params = Data->Iopb->Parameters.Read;
        ULONG BytesReturned = 0;

        Data->IoStatus.Status = FltReadFile( FltObjects->Instance,
                                             Fcb->LowerFileObject,
                                             &Params.ByteOffset, Params.Length,
                                             Params.ReadBuffer, FLTFL_IO_OPERATION_NON_CACHED
                                             , &BytesReturned, NULLPTR,NULLPTR
        );

        Data->IoStatus.Information = BytesReturned;
        FileObject->CurrentByteOffset.QuadPart = Params.ByteOffset.QuadPart + BytesReturned;
        if( FileObject->CurrentByteOffset.QuadPart > Fcb->AdvFcbHeader.FileSize.QuadPart )
            FileObject->CurrentByteOffset.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;

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

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostRead( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                  PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS ReadPagingIO( IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Read.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Read.ByteOffset;

    ULONG BytesRead = 0;                // FltReadFile 을 통해 읽은 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;

    AcquireCmnResource( IrpContext, FCB_PGIO_SHARED );

    __try
    {
        if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart == Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
            {

            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.FileSize.QuadPart )
            {

            }
            else
            {
                ASSERT( false );
            }
        }
        else if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart == Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {

            }
            else if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {

            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                     ByteOffset.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
            {

            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.FileSize.QuadPart )
            {

            }
            else
            {
                ASSERT( false );
            }
        }
        else
        {
            ASSERT( false );
        }
    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return IrpContext->Status;
}

NTSTATUS ReadCachedIO( IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Read.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Read.ByteOffset;

    ULONG BytesRead = 0;                // FltReadFile 을 통해 읽은 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;

    AcquireCmnResource( IrpContext, FCB_MAIN_SHARED );

    __try
    {
        if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart == Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {

            }
            else if( ByteOffset.QuadPart == Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {

            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {

            }
        }
        else if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart == Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {

            }
            else if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {

            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                     ByteOffset.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
            {

            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.FileSize.QuadPart )
            {

            }
            else
            {
                ASSERT( false );
            }
        }
        else
        {
            ASSERT( false );
        }
    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return IrpContext->Status;
}

NTSTATUS ReadNonCachedIO( IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Read.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Read.ByteOffset;

    ULONG BytesRead = 0;                // FltReadFile 을 통해 읽은 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;

    AcquireCmnResource( IrpContext, FCB_MAIN_SHARED );

    __try
    {
        if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart == Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToRead = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );
                    auto BytesToZero = 0;

                    Status = ReadNonCachedIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy + BytesToZero );
                        IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.ValidDataLength.QuadPart - ByteOffset.QuadPart );;
                    auto BytesToRead = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );
                    auto BytesToZero = 0;

                    Status = ReadNonCachedIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy + BytesToZero );

                        if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                        {
                            IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                        }
                        else
                        {
                            IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                        }
                    }
                }
                else
                {
                    ASSERT( false );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                AssignCmnResult( IrpContext, STATUS_SUCCESS );
                SafeZeroMemory( ReadBuffer, Length );
                AssignCmnResultInfo( IrpContext, 0 );
            }
        }
        else if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToRead = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );
                    auto BytesToZero = 0;

                    Status = ReadNonCachedIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy + BytesToZero );
                        IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                         ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart
                         )
                {
                    // 예) ValidDataLength = 500 , ByteOffset = 300, Length = 300
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.ValidDataLength.QuadPart - ByteOffset.QuadPart );
                    auto BytesToRead = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );
                    auto BytesToZero = Length - BytesToCopy;

                    Status = ReadNonCachedIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy + BytesToZero );
                        IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    // 예) ValidDataLength = 500, FileSize = 550, ByteOffset = 300, Length = 300
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.ValidDataLength.QuadPart - ByteOffset.QuadPart );
                    auto BytesToRead = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );
                    auto BytesToZero = Length - BytesToCopy;

                    Status = ReadNonCachedIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                        IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                    }
                }
                else
                {
                    ASSERT( false );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                     ByteOffset.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    AssignCmnResult( IrpContext, STATUS_SUCCESS );
                    SafeZeroMemory( ReadBuffer, Length );
                    AssignCmnResultInfo( IrpContext, Length );

                    IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    AssignCmnResult( IrpContext, STATUS_SUCCESS );
                    SafeZeroMemory( ReadBuffer, Length );
                    AssignCmnResultInfo( IrpContext, 0 );

                    IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                }
                else
                {
                    ASSERT( false );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                AssignCmnResult( IrpContext, STATUS_SUCCESS );
                SafeZeroMemory( ReadBuffer, Length );
                AssignCmnResultInfo( IrpContext, 0 );
            }
            else
            {
                ASSERT( false );
            }
        }
        else
        {
            ASSERT( false );
        }
    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return IrpContext->Status;
}

NTSTATUS ReadNonCachedIO( IRP_CONTEXT* IrpContext, PVOID ReadBuffer, ULONG BytesToCopy, ULONG BytesToRead,
                          ULONG BytesToZero )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Read.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Read.ByteOffset;

    ULONG BytesRead = 0;                // FltReadFile 을 통해 읽은 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;

    __try
    {
        TySwapBuffer = AllocateSwapBuffer( BUFFER_SWAP_READ, BytesToRead );

        if( TySwapBuffer.Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            AssignCmnResult( IrpContext, Status );
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d\n",
                       IrpContext->EvtID, __FUNCTION__, "Allocate Swap Buffer FAILED", __LINE__ ) );
            __leave;
        }

        Status = FltReadFile( IrpContext->FltObjects->Instance, Fcb->LowerFileObject,
                              &ByteOffset, BytesToRead,
                              TySwapBuffer.Buffer,
                              FLTFL_IO_OPERATION_NON_CACHED,
                              &BytesRead, NULLPTR, NULLPTR
        );

        AssignCmnResult( IrpContext, Status );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s\n"
                       , IrpContext->EvtID, __FUNCTION__, "FltReadFile FAILED", __LINE__
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       ) );
            __leave;
        }

        RtlCopyMemory( ReadBuffer, TySwapBuffer.Buffer, BytesToCopy );
        if( BytesToZero > 0 )
            SafeZeroMemory( Add2Ptr( ReadBuffer, BytesToCopy ), BytesToZero );
    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return Status;
}
