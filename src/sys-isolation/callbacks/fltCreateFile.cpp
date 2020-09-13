#include "fltCreateFile.hpp"

#include "irpContext.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreCreate( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                  PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;

    __try
    {
        ///////////////////////////////////////////////////////////////////////
        /// Validate Input

        if( BooleanFlagOn( Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE ) )
            __leave;

        auto CreateOptions = Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;
        auto CreateDisposition = ( Data->Iopb->Parameters.Create.Options >> 24 ) & 0x000000ff;

        IrpContext = CreateIrpContext( Data, FltObjects );
        if( IrpContext == NULLPTR )
            __leave;

    }
    __finally
    {

    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostCreate( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                    PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    return FltStatus;
}
