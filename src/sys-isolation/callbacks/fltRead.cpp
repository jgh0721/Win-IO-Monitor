#include "fltRead.hpp"

#include "privateFCBMgr.hpp"
#include "irpContext.hpp"
#include "utilities/bufferMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreRead( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;

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

        if( PagingIo != FALSE )
        {
            ReadPagingIO( IrpContext );
        }
        else
        {
            if( NonCachedIo != FALSE )
                ReadNonCachedIO( IrpContext );
            else
                ReadCachedIO( IrpContext );
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

NTSTATUS ReadPagingIO( IRP_CONTEXT* IrpContext )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Read.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Read.ByteOffset;

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

NTSTATUS ReadCachedIO( IRP_CONTEXT* IrpContext )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Read.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Read.ByteOffset;

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

NTSTATUS ReadNonCachedIO( IRP_CONTEXT* IrpContext )
{
    auto Fcb = IrpContext->Fcb;
    auto Length = IrpContext->Data->Iopb->Parameters.Read.Length;
    auto ByteOffset = IrpContext->Data->Iopb->Parameters.Read.ByteOffset;

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
                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                         ByteOffset.QuadPart + Length <= Fcb->AdvFcbHeader.FileSize.QuadPart
                         )
                {

                }
                else if( ByteOffset.QuadPart + Length > Fcb->AdvFcbHeader.FileSize.QuadPart )
                {

                }
                else
                {
                    ASSERT( false );
                }
            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {

            }
        }
        else if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
        {
            if( ByteOffset.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {

            }
            else if( ByteOffset.QuadPart >= Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                     ByteOffset.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
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
