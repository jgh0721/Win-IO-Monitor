#ifndef HDR_MINIFILTER_DIRECTORY_CONTROL
#define HDR_MINIFILTER_DIRECTORY_CONTROL

#include "fltBase.hpp"
#include "irpContext.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
WinIOPreDirectoryControl( __inout PFLT_CALLBACK_DATA Data,
                               __in PCFLT_RELATED_OBJECTS FltObjects,
                               __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
WinIOPostDirectoryControl( __inout PFLT_CALLBACK_DATA Data,
                                __in PCFLT_RELATED_OBJECTS FltObjects,
                                __in_opt PVOID CompletionContext,
                                __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END

void TuneFileDirectoryInformation( __in IRP_CONTEXT* IrpContext, __in CTX_INSTANCE_CONTEXT* InstanceContext );
void TuneFileFullDirectoryInformation( __in IRP_CONTEXT* IrpContext, __in CTX_INSTANCE_CONTEXT* InstanceContext );
void TuneFileBothDirectoryInformation( __in IRP_CONTEXT* IrpContext, __in CTX_INSTANCE_CONTEXT* InstanceContext );
void TuneFileNameInformation( __in IRP_CONTEXT* IrpContext, __in CTX_INSTANCE_CONTEXT* InstanceContext );
void TuneFileIdBothDirectoryInformation( __in IRP_CONTEXT* IrpContext, __in CTX_INSTANCE_CONTEXT* InstanceContext );
void TuneFileIdFullDirectoryInformation( __in IRP_CONTEXT* IrpContext, __in CTX_INSTANCE_CONTEXT* InstanceContext );

NTSTATUS GetQueryDirectoryBuffer( __in PFLT_CALLBACK_DATA Data, __out PVOID* Buffer );

NTSTATUS CheckBufferSize( __in TyGenericBuffer<WCHAR>* Buffer, __in ULONG RequiredSize );


#endif // HDR_MINIFILTER_DIRECTORY_CONTROL