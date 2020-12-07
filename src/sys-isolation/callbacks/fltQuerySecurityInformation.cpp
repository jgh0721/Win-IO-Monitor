#include "fltQuerySecurityInformation.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#include "utilities/fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreQuerySecurityInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                                    PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;
    IRP_CONTEXT*                                IrpContext = NULLPTR;

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        if( FLT_IS_FASTIO_OPERATION( Data ) )
            return FLT_PREOP_DISALLOW_FASTIO;

        IrpContext = CreateIrpContext( Data, FltObjects );
        PrintIrpContext( IrpContext );

        AcquireCmnResource( IrpContext, FCB_MAIN_SHARED );

        PVOID KernelBuffer = NULLPTR;
        FileObject = ( ( CCB* )FileObject->FsContext2 )->LowerFileObject;

        const auto& Parameters = Data->Iopb->Parameters.QuerySecurity;
        if( Parameters.Length == 0 )
            KernelBuffer = Parameters.SecurityBuffer;
        else
        {
            KernelBuffer = ExAllocatePool( PagedPool, Parameters.Length );
            if( KernelBuffer == NULLPTR )
            {
                AssignCmnResult( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
                __leave;
            }

            RtlCopyMemory( KernelBuffer, Parameters.SecurityBuffer, Parameters.Length );
        }

        ULONG LengthNeeded = 0;
        NTSTATUS Status = FltQuerySecurityObject( FltObjects->Instance,
                                                  FileObject,
                                                  Data->Iopb->Parameters.QuerySecurity.SecurityInformation,
                                                  KernelBuffer,
                                                  Data->Iopb->Parameters.QuerySecurity.Length,
                                                  &LengthNeeded );

        // NOTE: https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/irp-mj-query-security-and-irp-mj-set-security
        // CACLS 유틸리티
        if( Status == STATUS_BUFFER_TOO_SMALL )
            Status = STATUS_BUFFER_OVERFLOW;

        if( KernelBuffer != NULLPTR )
        {
            RtlCopyMemory( Parameters.SecurityBuffer, KernelBuffer, Parameters.Length );
            ExFreePool( KernelBuffer );
        }

        AssignCmnResult( IrpContext, Status );
        AssignCmnResultInfo( IrpContext, LengthNeeded );
        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }
    __finally
    {
        if( IrpContext != NULLPTR )
        {
            if( FlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
                FltStatus = IrpContext->PreFltStatus;

            PrintIrpContext( IrpContext, true );
        }

        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostQuerySecurityInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                                      PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}
