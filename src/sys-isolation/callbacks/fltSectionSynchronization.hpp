#ifndef HDR_ISOLATION_SECTION_SYNCHRONIZATION
#define HDR_ISOLATION_SECTION_SYNCHRONIZATION

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*!
    Win32 AcquireFileForNtCreateSection(CreateFileMapping) 함수를 호출하여 파일을 메모리에 매핑/해제할 때 호출된다
*/

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreAcquireSectionSynchronization( __inout PFLT_CALLBACK_DATA Data,
                                        __in PCFLT_RELATED_OBJECTS FltObjects,
                                        __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostAcquireSectionSynchronization( __inout PFLT_CALLBACK_DATA    Data,
                                         __in PCFLT_RELATED_OBJECTS    FltObjects,
                                         __in_opt PVOID                CompletionContext,
                                         __in FLT_POST_OPERATION_FLAGS Flags );

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreReleaseSectionSynchronization( __inout PFLT_CALLBACK_DATA Data,
                                        __in PCFLT_RELATED_OBJECTS FltObjects,
                                        __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostReleaseSectionSynchronization( __inout PFLT_CALLBACK_DATA    Data,
                                         __in PCFLT_RELATED_OBJECTS    FltObjects,
                                         __in_opt PVOID                CompletionContext,
                                         __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END


#endif // HDR_ISOLATION_SECTION_SYNCHRONIZATION