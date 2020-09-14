#include "fltCreateFile.hpp"

#include "irpContext.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

typedef struct _CREATE_ARGS
{
    LARGE_INTEGER           FileSize;
    ULONG                   FileStatus;


} CREATE_ARGS, * PCREATE_ARGS;

typedef struct _RESULT
{

} RESULT, * PRESULT;

///////////////////////////////////////////////////////////////////////////////

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

        if( BooleanFlagOn( CreateOptions, FILE_DIRECTORY_FILE ) )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );
        if( IrpContext == NULLPTR )
            __leave;

        if( IrpContext->SrcFileFullPath.Buffer == NULLPTR )
            __leave;

        switch( CreateDisposition )
        {
            case FILE_SUPERSEDE: {} break;
            case FILE_OPEN: {} break;
            case FILE_CREATE: {} break;
            case FILE_OPEN_IF: {} break;
            case FILE_OVERWRITE: {} break;
            case FILE_OVERWRITE_IF: {} break;
        } // switch CreateDisposition

    }
    __finally
    {
        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostCreate( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                    PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    return FltStatus;
}
