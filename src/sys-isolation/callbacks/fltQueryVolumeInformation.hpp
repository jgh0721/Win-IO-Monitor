#ifndef HDR_ISOLATION_QUERY_VOLUME_INFORMATION
#define HDR_ISOLATION_QUERY_VOLUME_INFORMATION

#include "fltBase.hpp"
#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreQueryVolumeInformation( __inout PFLT_CALLBACK_DATA Data,
                                 __in PCFLT_RELATED_OBJECTS FltObjects,
                                 __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostQueryVolumeInformation( __inout PFLT_CALLBACK_DATA    Data,
                                  __in PCFLT_RELATED_OBJECTS    FltObjects,
                                  __in_opt PVOID                CompletionContext,
                                  __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END

NTSTATUS ProcessFileFsVolumeInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsLabelInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsSizeInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsDeviceInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsAttributeInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsControlInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsFullSizeInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsObjectIdInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsDriverPathInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsVolumeFlagsInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsSectorSizeInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsDataCopyInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsMetadataSizeInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS ProcessFileFsFullSizeInformationEx( __in IRP_CONTEXT* IrpContext );

#endif // HDR_ISOLATION_QUERY_VOLUME_INFORMATION
