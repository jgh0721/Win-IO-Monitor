#include "fltNetworkQueryOpen.hpp"

#include "policies/ProcessFilter.hpp"
#include "utilities/procNameMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreNetworkQueryOpen( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                            PVOID* CompletionContext )
{
    return FLT_PREOP_DISALLOW_FASTIO;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostNetworkQueryOpen( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                              PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    return FltStatus;
}
