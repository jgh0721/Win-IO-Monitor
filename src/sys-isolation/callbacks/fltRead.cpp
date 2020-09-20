#include "fltRead.hpp"

#include "privateFCBMgr.hpp"
#include "irpContext.hpp"
#include "utilities/bufferMgr.hpp"

#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define READ_AHEAD_GRANULARITY          (0x10000)

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

        if( Data->Iopb->Parameters.Read.Length == 0 )
        {
            AssignCmnResult( IrpContext, STATUS_SUCCESS );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            __leave;
        }

        // Non-CachedIO 에서는 반드시 섹터단위로 정렬되어야한다 
        if( NonCachedIo != FALSE )
        {
            if( ( Data->Iopb->Parameters.Read.ByteOffset.LowPart & (IrpContext->InstanceContext->BytesPerSector - 1) ) ||
                ( Data->Iopb->Parameters.Read.Length & (IrpContext->InstanceContext->BytesPerSector - 1)) )
            {
                AssignCmnResult( IrpContext, STATUS_INVALID_PARAMETER );
                AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
                __leave;
            }
        }

        if( ReadBuffer == NULLPTR )
        {
            AssignCmnResult( IrpContext, STATUS_INVALID_USER_BUFFER );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
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
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToRead = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );
                    auto BytesToZero = 0;

                    Status = ReadPagingIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto ValidDataLengthOffset = ( ULONG )( Fcb->AdvFcbHeader.FileSize.QuadPart - ByteOffset.QuadPart );
                    auto BytesToRead = ALIGNED( ULONG, Length, Fcb->InstanceContext->BytesPerSector );
                    auto BytesToZero = Length - ValidDataLengthOffset;
                    auto BytesToCopy = Length - BytesToZero;

                    Status = ReadPagingIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        SafeZeroMemory( Add2Ptr( ReadBuffer, ValidDataLengthOffset ), BytesToZero );
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                    }
                }
                else
                {
                    ASSERT( false );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                SafeZeroMemory( ReadBuffer, Length );

                AssignCmnResult( IrpContext, STATUS_SUCCESS );
                AssignCmnResultInfo( IrpContext, 0 );
            }
            else
            {
                ASSERT( false );
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

                    Status = ReadPagingIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                         ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart
                         )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.ValidDataLength.QuadPart - ByteOffset.QuadPart );
                    auto BytesToRead = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );
                    auto BytesToZero = Length - BytesToCopy;

                    Status = ReadPagingIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        SafeZeroMemory( Add2Ptr( ReadBuffer, BytesToCopy ), BytesToZero );
                        AssignCmnResultInfo( IrpContext, Length );
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.ValidDataLength.QuadPart - ByteOffset.QuadPart );
                    auto BytesToRead = ALIGNED( ULONG, BytesToCopy, Fcb->InstanceContext->BytesPerSector );
                    auto BytesToZero = ( ULONG )( ByteOffset.QuadPart + Length - Fcb->AdvFcbHeader.ValidDataLength.QuadPart );

                    Status = ReadPagingIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        SafeZeroMemory( Add2Ptr( ReadBuffer, BytesToCopy ), BytesToZero );
                        AssignCmnResultInfo( IrpContext, ( ( ULONG )( Fcb->AdvFcbHeader.FileSize.QuadPart - ByteOffset.QuadPart ) ) );
                    }
                }
                else
                {
                    ASSERT( false );
                }
            }
            else if( ByteOffset.QuadPart == Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    SafeZeroMemory( ReadBuffer, Length );

                    AssignCmnResult( IrpContext, STATUS_SUCCESS );
                    AssignCmnResultInfo( IrpContext, Length );
                }
                else
                {
                    auto BytesToZero = ( ULONG )( Fcb->AdvFcbHeader.FileSize.QuadPart - Fcb->AdvFcbHeader.ValidDataLength.QuadPart );
                    SafeZeroMemory( ReadBuffer, BytesToZero );

                    AssignCmnResult( IrpContext, STATUS_SUCCESS );
                    AssignCmnResultInfo( IrpContext, BytesToZero );
                }
            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                     ByteOffset.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    SafeZeroMemory( ReadBuffer, Length );

                    AssignCmnResult( IrpContext, STATUS_SUCCESS );
                    AssignCmnResultInfo( IrpContext, Length );
                }
                else
                {
                    auto BytesToZero = ( ULONG )( Fcb->AdvFcbHeader.FileSize.QuadPart - ByteOffset.QuadPart );

                    SafeZeroMemory( ReadBuffer, BytesToZero );

                    AssignCmnResult( IrpContext, STATUS_SUCCESS );
                    AssignCmnResultInfo( IrpContext, BytesToZero );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                SafeZeroMemory( ReadBuffer, Length );

                AssignCmnResult( IrpContext, STATUS_SUCCESS );
                AssignCmnResultInfo( IrpContext, 0 );
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
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToRead = Length;
                    auto BytesToZero = 0;

                    Status = ReadCachedIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                        IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.ValidDataLength.QuadPart - ByteOffset.QuadPart );;
                    auto BytesToRead = BytesToCopy;
                    auto BytesToZero = Length - BytesToCopy;

                    Status = ReadCachedIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                        IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = Fcb->AdvFcbHeader.ValidDataLength.QuadPart;
                    }
                }
                else
                {
                    ASSERT( false );
                }
            }
            else if( ByteOffset.QuadPart == Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                AssignCmnResult( IrpContext, STATUS_SUCCESS );
                SafeZeroMemory( ReadBuffer, Length );
                AssignCmnResultInfo( IrpContext, 0 );
            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
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
                    auto BytesToRead = BytesToCopy;
                    auto BytesToZero = 0;

                    Status = ReadCachedIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                        IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.ValidDataLength.QuadPart - ByteOffset.QuadPart );
                    auto BytesToRead = BytesToCopy;
                    auto BytesToZero = 0;

                    Status = ReadCachedIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );

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
            else if( ByteOffset.QuadPart == Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                    ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart
                    )
                {
                    AssignCmnResult( IrpContext, STATUS_SUCCESS );
                    SafeZeroMemory( ReadBuffer, Length );
                    AssignCmnResultInfo( IrpContext, Length );

                    IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.FileSize.QuadPart - ByteOffset.QuadPart );
                    auto BytesToRead = 0;
                    auto BytesToZero = Length - BytesToCopy;

                    SafeZeroMemory( ReadBuffer, BytesToCopy );
                    SafeZeroMemory( Add2Ptr( ReadBuffer, BytesToCopy ), BytesToZero );

                    AssignCmnResult( IrpContext, STATUS_SUCCESS );
                    AssignCmnResultInfo( IrpContext, BytesToCopy );
                }
                else
                {
                    ASSERT( false );
                }

            }
            else if( ByteOffset.QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                     ByteOffset.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                if( ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = Length;
                    auto BytesToRead = BytesToCopy;
                    auto BytesToZero = 0;

                    Status = ReadCachedIO( IrpContext, ReadBuffer, BytesToCopy, BytesToRead, BytesToZero );

                    if( NT_SUCCESS( Status ) )
                    {
                        AssignCmnResultInfo( IrpContext, BytesToCopy );
                        IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Length;
                    }
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {
                    auto BytesToCopy = ( ULONG )( Fcb->AdvFcbHeader.FileSize.QuadPart - ByteOffset.QuadPart );;
                    auto BytesToRead = BytesToCopy;
                    auto BytesToZero = Length - BytesToCopy;

                    SafeZeroMemory( ReadBuffer, BytesToCopy );
                    SafeZeroMemory( Add2Ptr( ReadBuffer, BytesToCopy ), BytesToZero );

                    AssignCmnResult( IrpContext, STATUS_SUCCESS );
                    AssignCmnResultInfo( IrpContext, BytesToCopy );

                    IrpContext->FltObjects->FileObject->CurrentByteOffset.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                }
                else
                {
                    ASSERT( false );
                }

            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                SafeZeroMemory( ReadBuffer, Length );

                AssignCmnResult( IrpContext, STATUS_END_OF_FILE );
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

NTSTATUS ReadPagingIO( IRP_CONTEXT* IrpContext, PVOID ReadBuffer, ULONG BytesToCopy, ULONG BytesToRead,
                       ULONG BytesToZero )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Read.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Read.ByteOffset;
    auto FileObject = IrpContext->FltObjects->FileObject;

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
    }
    __finally
    {
        DeallocateBuffer( &TySwapBuffer );
    }

    return Status;
}

NTSTATUS ReadCachedIO( IRP_CONTEXT* IrpContext, PVOID ReadBuffer, ULONG BytesToCopy, ULONG BytesToRead,
                       ULONG BytesToZero )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Read.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Read.ByteOffset;
    auto FileObject = IrpContext->FltObjects->FileObject;

    ULONG BytesRead = 0;                // FltReadFile 을 통해 읽은 바이트 수
    NTSTATUS Status = STATUS_SUCCESS;
    TyGenericBuffer<BYTE> TySwapBuffer;
    IO_STATUS_BLOCK IoStatus;

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

        // NOTE: 캐시관리자 함수를 호출하기 전에 해당 파일객체에 대해 캐시를 초기화해야한다
        if( FileObject->PrivateCacheMap == NULLPTR )
        {
            CcInitializeCacheMap( FileObject,
                                  ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ),
                                  TRUE,
                                  &GlobalContext.CacheMgrCallbacks,
                                  Fcb );

            CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );
            // TODO: 향후에 Lazy-Write 기능을 구현해야한다
            CcSetAdditionalCacheAttributes( FileObject, FALSE, TRUE );
        }

        // NOTE: CcCopyRead 함수는 I/O 오류가 발생하면 예외를 발생시킨다
        __try
        {
            BOOLEAN IsSuccess = CcCopyRead( FileObject, ( PLARGE_INTEGER )&ByteOffset,
                                            BytesToRead, TRUE,
                                            TySwapBuffer.Buffer, &IoStatus );

            Status = IoStatus.Status;
            AssignCmnResult( IrpContext, Status );

            if( IsSuccess == FALSE )
            {
                KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d Status=0x%08x,%s\n"
                           , IrpContext->EvtID, __FUNCTION__, "CcCopyRead FAILED", __LINE__
                           , Status, ntkernel_error_category::find_ntstatus( Status )->message
                           ) );
                __leave;
            }

            RtlCopyMemory( ReadBuffer, TySwapBuffer.Buffer, BytesToCopy );
            if( BytesToZero > 0 )
            {
                SafeZeroMemory( Add2Ptr( ReadBuffer, BytesToCopy ), BytesToZero );
            }
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            AssignCmnResult( IrpContext, STATUS_UNSUCCESSFUL );

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
