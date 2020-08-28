#ifndef HDR_MINIFILTER_CLOSE
#define HDR_MINIFILTER_CLOSE

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS
FLTAPI WinIOPreClose( __inout PFLT_CALLBACK_DATA Data,
                      __in PCFLT_RELATED_OBJECTS FltObjects,
                      __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS
FLTAPI WinIOPostClose( __inout PFLT_CALLBACK_DATA Data,
                       __in PCFLT_RELATED_OBJECTS FltObjects,
                       __in_opt PVOID CompletionContext,
                       __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END

#endif // HDR_MINIFILTER_CLOSE