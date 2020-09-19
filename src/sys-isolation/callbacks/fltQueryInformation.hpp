#ifndef HDR_ISOLATION_QUERY_INFORMATION
#define HDR_ISOLATION_QUERY_INFORMATION

#include "fltBase.hpp"

#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreQueryInformation( __inout PFLT_CALLBACK_DATA Data,
                           __in PCFLT_RELATED_OBJECTS FltObjects,
                           __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostQueryInformation( __inout PFLT_CALLBACK_DATA    Data,
                            __in PCFLT_RELATED_OBJECTS    FltObjects,
                            __in_opt PVOID                CompletionContext,
                            __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END

NTSTATUS ProcessFileAllInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileAttributeTagInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileBasicInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileCompressionInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileEaInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileInternalInformation( __in IRP_CONTEXT* IrpContext );
//NTSTATUS ProcessFileMoveClusterInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileNameInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileNetworkOpenInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFilePositionInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileStandardInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileStreamInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileHardLinkInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileNormalizedNameInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileStandardLinkInformation( __in IRP_CONTEXT* IrpContext );

#endif // HDR_ISOLATION_QUERY_INFORMATION
