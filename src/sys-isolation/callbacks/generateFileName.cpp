#include "generateFileName.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS FLTAPI FilterGenerateFileName( PFLT_INSTANCE Instance, PFILE_OBJECT FileObject, PFLT_CALLBACK_DATA CallbackData,
                                        FLT_FILE_NAME_OPTIONS NameOptions, PBOOLEAN CacheFileNameInformation, PFLT_NAME_CONTROL FileName )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    return Status;
}

NTSTATUS FLTAPI FilterNormalizeNameComponent( PFLT_INSTANCE Instance, PCUNICODE_STRING ParentDirectory,
                                              USHORT VolumeNameLength, PCUNICODE_STRING Component, PFILE_NAMES_INFORMATION ExpandComponentName,
                                              ULONG ExpandComponentNameLength, FLT_NORMALIZE_NAME_FLAGS Flags, PVOID* NormalizationContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    return Status;
}

NTSTATUS FLTAPI FilterNormalizeNameComponentEx( PFLT_INSTANCE Instance, PFILE_OBJECT FileObject,
                                                PCUNICODE_STRING ParentDirectory, USHORT VolumeNameLength, PCUNICODE_STRING Component,
                                                PFILE_NAMES_INFORMATION ExpandComponentName, ULONG ExpandComponentNameLength, FLT_NORMALIZE_NAME_FLAGS Flags,
                                                PVOID* NormalizationContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    return Status;
}

void FLTAPI FilterNormalizeContextCleanup( PVOID* NormalizationContext )
{
    UNREFERENCED_PARAMETER( NormalizationContext );
}
