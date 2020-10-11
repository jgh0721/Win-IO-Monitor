#ifndef HDR_ISOLATION_METADATA
#define HDR_ISOLATION_METADATA

#include "fltBase.hpp"
#include "Metadata_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS InitializeMetaDataMgr();
NTSTATUS UninitializeMetaDataMgr();

METADATA_TYPE GetFileMetaDataInfo( __in PFILE_OBJECT FileObject, __out METADATA_DRIVER* MetaDataInfo );

void InitializeMetaDataInfo( __inout METADATA_DRIVER* MetaDataInfo );
void UninitializeMetaDataInfo( __in METADATA_DRIVER*& MetaDataInfo );

NTSTATUS WriteMetaData( __in PFILE_OBJECT FileObject, __in METADATA_DRIVER* MetaDataInfo );
NTSTATUS UpdateFileSizeOnMetaData( __in PFILE_OBJECT FileObject, __in LONGLONG FileSize );

#endif // HDR_ISOLATION_METADATA