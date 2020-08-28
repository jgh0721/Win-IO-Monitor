#include "fltNetworkQueryOpen.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

FLT_PREOP_CALLBACK_STATUS FLTAPI WinIOPreNetworkQueryOpen( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                           PVOID* CompletionContext )
{
    if( FLT_IS_FASTIO_OPERATION( Data ) )
    {
        return FLT_PREOP_DISALLOW_FASTIO;
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI WinIOPostNetworkQueryOpen( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                             PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    return FLT_POSTOP_FINISHED_PROCESSING;
}
