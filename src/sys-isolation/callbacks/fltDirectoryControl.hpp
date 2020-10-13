#ifndef HDR_ISOLATION_DIRECTORY_CONTROL
#define HDR_ISOLATION_DIRECTORY_CONTROL

#include "fltBase.hpp"
#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreDirectoryControl( __inout PFLT_CALLBACK_DATA Data,
                           __in PCFLT_RELATED_OBJECTS FltObjects,
                           __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostDirectoryControl( __inout PFLT_CALLBACK_DATA    Data,
                            __in PCFLT_RELATED_OBJECTS    FltObjects,
                            __in_opt PVOID                CompletionContext,
                            __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostDirectoryControlWhenSafe( __inout PFLT_CALLBACK_DATA    Data,
                                    __in PCFLT_RELATED_OBJECTS    FltObjects,
                                    __in_opt PVOID                CompletionContext,
                                    __in FLT_POST_OPERATION_FLAGS Flags );

NTSTATUS TuneFileDirectoryInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileFullDirectoryInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileBothDirectoryInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileNamesInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileObjectIdInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileQuotaInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileReparsePointInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileIdBothDirectoryInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileIdFullDirectoryInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileIdGlobalTxDirectoryInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileIdExtdDirectoryInformation( __in IRP_CONTEXT* IrpContext );
NTSTATUS TuneFileIdExtdBothDirectoryInformation( __in IRP_CONTEXT* IrpContext );

enum TyEnConcernedType
{
    CONCERNED_NONE  = 0x0,
    CONCERNED_SIZE  = 0x1,
    CONCERNED_NAME  = 0x2
};

/**
 * @brief 
 * @param IrpContext 
 * @param FileFullPath 
 * @param MetaDataInfo 
 * @return TyEnConcernedType ÀÇ Á¶ÇÕ  
*/
ULONG IsConcernedFile( __in IRP_CONTEXT* IrpContext, __in TyGenericBuffer<WCHAR>* FileFullPath, __out METADATA_DRIVER* MetaDataInfo );

NTSTATUS CorrectFileSize( __in IRP_CONTEXT* IrpContext, __in METADATA_DRIVER* MetaDataInfo, __inout LARGE_INTEGER* AllocatoinSize, __inout LARGE_INTEGER* EndOfFile );

#endif // HDR_ISOLATION_DIRECTORY_CONTROL
