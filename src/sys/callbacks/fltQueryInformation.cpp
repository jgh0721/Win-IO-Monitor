#include "fltQueryInformation.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI WinIOPreQueryInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    __try
    {
        
    }
    __finally
    {
        
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI WinIOPostQueryInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    return FLT_POSTOP_FINISHED_PROCESSING;
}
