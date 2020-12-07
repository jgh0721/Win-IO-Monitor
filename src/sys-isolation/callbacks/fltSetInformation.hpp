#ifndef HDR_ISOLATION_SET_INFORMATION
#define HDR_ISOLATION_SET_INFORMATION

#include "fltBase.hpp"
#include "irpContext_Defs.hpp"

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

///////////////////////////////////////////////////////////////////////////////

// ULONG = TyEnCompleteState 조합
ULONG CommonPreSetInformation( __in PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __inout_opt PVOID* CompletionContext,
                               __out PIRP_CONTEXT* IrpContext );

NTSTATUS ProcessSetFileAllocationInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessSetFileEndOfFileInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessSetFileValidDataLengthInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessSetFilePositionInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessSetFileUnifiedRenameInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessSetFileDispositionInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessSetFileDispositionInformationEx( __in IRP_CONTEXT* IrpContext );

NTSTATUS ProcessSetFileInformation( __in IRP_CONTEXT* IrpContext );

// @param DstFileFullPath 에는 사용자가 전달한 변경하려는 이름을 전달한다( 드라이버에 의해 실제 변경된 이름이 아님 )
void PostProcessFileRename( __in IRP_CONTEXT* IrpContext, __in TyGenericBuffer<WCHAR>* DstFileFullPath );

#endif // HDR_ISOLATION_SET_INFORMATION
