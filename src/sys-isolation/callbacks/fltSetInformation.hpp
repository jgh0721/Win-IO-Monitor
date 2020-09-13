#ifndef HDR_ISOLATION_SET_INFORMATION
#define HDR_ISOLATION_SET_INFORMATION

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreSetInformation( __inout PFLT_CALLBACK_DATA Data,
                         __in PCFLT_RELATED_OBJECTS FltObjects,
                         __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostSetInformation( __inout PFLT_CALLBACK_DATA    Data,
                          __in PCFLT_RELATED_OBJECTS    FltObjects,
                          __in_opt PVOID                CompletionContext,
                          __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END

#endif // HDR_ISOLATION_SET_INFORMATION
