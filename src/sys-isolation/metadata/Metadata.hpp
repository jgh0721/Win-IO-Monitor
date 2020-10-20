#ifndef HDR_ISOLATION_METADATA
#define HDR_ISOLATION_METADATA

#include "fltBase.hpp"
#include "Metadata_Defs.hpp"

#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS InitializeMetaDataMgr();
NTSTATUS UninitializeMetaDataMgr();

METADATA_TYPE GetFileMetaDataInfo( __in IRP_CONTEXT* IrpContext, __in PFILE_OBJECT FileObject, __out_opt METADATA_DRIVER* MetaDataInfo );
LONGLONG GetFileSizeFromMetaData( __in METADATA_DRIVER* MetaDataInfo );
LONGLONG GetHDRSizeFromMetaData( __in METADATA_DRIVER* MetaDataInfo );

METADATA_DRIVER* AllocateMetaDataInfo();
void InitializeMetaDataInfo( __inout METADATA_DRIVER* MetaDataInfo );
void UninitializeMetaDataInfo( __in METADATA_DRIVER*& MetaDataInfo );

NTSTATUS WriteMetaData( __in IRP_CONTEXT* IrpContext, __in PFILE_OBJECT FileObject, __in METADATA_DRIVER* MetaDataInfo );
NTSTATUS UpdateFileSizeOnMetaData( __in IRP_CONTEXT* IrpContext, __in PFILE_OBJECT FileObject, __in const LARGE_INTEGER& FileSize );
NTSTATUS UpdateFileSizeOnMetaData( __in IRP_CONTEXT* IrpContext, __in PFILE_OBJECT FileObject, __in LONGLONG FileSize );

bool IsMetaDataDriverInfo( __in PVOID Buffer );

///////////////////////////////////////////////////////////////////////////////
/// StubCode Management

void SetMetaDataStubCode( __in_opt PVOID StubCodeX86, __in ULONG StubCodeX86Size,
                          __in_opt PVOID StubCodeX64, __in ULONG StubCodeX64Size );

PVOID GetStubCodeX86();
ULONG GetStubCodeX86Size();
PVOID GetStubCodeX64();
ULONG GetStubCodeX64Size();

#endif // HDR_ISOLATION_METADATA