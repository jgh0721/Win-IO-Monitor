#include "fltSetInformation.hpp"


#include "irpContext.hpp"
#include "WinIOMonitor_W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI WinIOPreSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                         PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    const auto& Parameters = Data->Iopb->Parameters.SetFileInformation;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS ) Parameters.FileInformationClass;

    auto IrpContext = CreateIrpContext( Data, FltObjects );

    if( IrpContext != NULLPTR )
    {
        PrintIrpContext( IrpContext );
        *CompletionContext = IrpContext;
        FltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }

    switch( FileInformationClass )
    {
        case FileBasicInformation: {} break;
        case FileRenameInformation: {} break;
        case FileLinkInformation: {} break;
        case FileDispositionInformation: {} break;
        case FilePositionInformation: {} break;
        case FileAllocationInformation: {} break;
        case FileEndOfFileInformation: {} break;
        case FileValidDataLengthInformation: {} break;
        case nsW32API::FileDispositionInformationEx: {} break;
        case nsW32API::FileRenameInformationEx: {} break;
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI WinIOPostSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                           PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    auto IrpContext = ( PIRP_CONTEXT )CompletionContext;

    const auto& Parameters = Data->Iopb->Parameters.SetFileInformation;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS ) Parameters.FileInformationClass;

    switch( FileInformationClass )
    {
        case FileBasicInformation: {} break;
        case FileRenameInformation: {} break;
        case FileLinkInformation: {} break;
        case FileDispositionInformation: {} break;
        case FilePositionInformation: {} break;
        case FileAllocationInformation: {} break;
        case FileEndOfFileInformation: {} break;
        case FileValidDataLengthInformation: {} break;
        case nsW32API::FileDispositionInformationEx: {} break;
        case nsW32API::FileRenameInformationEx: {} break;
    }

    CloseIrpContext( IrpContext );

    return FLT_POSTOP_FINISHED_PROCESSING;
}
