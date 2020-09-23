#ifndef HDR_ISOLATION_CC_FLUSH
#define HDR_ISOLATION_CC_FLUSH

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreAcquireCcFlush( __inout PFLT_CALLBACK_DATA Data,
                         __in PCFLT_RELATED_OBJECTS FltObjects,
                         __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostAcquireCcFlush( __inout PFLT_CALLBACK_DATA    Data,
                          __in PCFLT_RELATED_OBJECTS    FltObjects,
                          __in_opt PVOID                CompletionContext,
                          __in FLT_POST_OPERATION_FLAGS Flags );

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreReleaseCcFlush( __inout PFLT_CALLBACK_DATA Data,
                         __in PCFLT_RELATED_OBJECTS FltObjects,
                         __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostReleaseCcFlush( __inout PFLT_CALLBACK_DATA    Data,
                          __in PCFLT_RELATED_OBJECTS    FltObjects,
                          __in_opt PVOID                CompletionContext,
                          __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END

#endif // HDR_ISOLATION_CC_FLUSH
