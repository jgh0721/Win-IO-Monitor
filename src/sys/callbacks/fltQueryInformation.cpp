#include "fltQueryInformation.hpp"

#include "WinIOMonitor_W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI WinIOPreQueryInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    __try
    {
        auto FileInformationClass = (nsW32API::FILE_INFORMATION_CLASS)Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;

        switch( FileInformationClass )
        {
            case nsW32API::FileAllInformation: {} break;
            case nsW32API::FileAttributeTagInformation: {} break;
            case nsW32API::FileBasicInformation: {} break;
            case nsW32API::FileCompressionInformation: {} break;
            case nsW32API::FileEaInformation: {} break;
            case nsW32API::FileInternalInformation: {} break;
            case nsW32API::FileMoveClusterInformation: {} break;
            case nsW32API::FileNameInformation: {} break;
            case nsW32API::FileNetworkOpenInformation: {} break;
            case nsW32API::FilePositionInformation: {} break;
            case nsW32API::FileStandardInformation: {} break;
            case nsW32API::FileStreamInformation: {} break;
        }
    }
    __finally
    {
        
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI WinIOPostQueryInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{

    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;

    __try
    {
        switch( FileInformationClass )
        {
            case nsW32API::FileAllInformation: {} break;
            case nsW32API::FileAttributeTagInformation: {} break;
            case nsW32API::FileBasicInformation: {} break;
            case nsW32API::FileCompressionInformation: {} break;
            case nsW32API::FileEaInformation: {} break;
            case nsW32API::FileInternalInformation: {} break;
            case nsW32API::FileMoveClusterInformation: {} break;
            case nsW32API::FileNameInformation: {} break;
            case nsW32API::FileNetworkOpenInformation: {} break;
            case nsW32API::FilePositionInformation: {} break;
            case nsW32API::FileStandardInformation: {} break;
            case nsW32API::FileStreamInformation: {} break;
        }
    }
    __finally
    {
        
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}
