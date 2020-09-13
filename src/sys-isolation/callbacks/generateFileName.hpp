#ifndef HDR_ISOLATION_GENERATE_FILENAME
#define HDR_ISOLATION_GENERATE_FILENAME

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*!
 * 격리 필터는 마치 상위 필터 드라이버의 입장에서 보면 파일시스템과 같이 동작해야하므로,
 * 격리된 파일에 대해 파일이름을 제공해야한다. 만약, 그렇지 않으면 격리 필터가 소유한 SFO( Shadow File Object ) 가
 * 실제 파일시스템까지 전달되고, BSOD 가 발생하게된다
 */

EXTERN_C_BEGIN

NTSTATUS FLTAPI FilterGenerateFileName( __in PFLT_INSTANCE Instance,
                                        __in PFILE_OBJECT FileObject,
                                        __in_opt PFLT_CALLBACK_DATA CallbackData,
                                        __in FLT_FILE_NAME_OPTIONS NameOptions,
                                        __out PBOOLEAN CacheFileNameInformation,
                                        __out PFLT_NAME_CONTROL FileName
);

NTSTATUS FLTAPI FilterNormalizeNameComponent( __in PFLT_INSTANCE Instance,
                                              __in PCUNICODE_STRING ParentDirectory,
                                              __in USHORT VolumeNameLength,
                                              __in PCUNICODE_STRING Component,
                                              __out_bcount( ExpandComponentNameLength ) PFILE_NAMES_INFORMATION ExpandComponentName,
                                              __in ULONG ExpandComponentNameLength,
                                              __in FLT_NORMALIZE_NAME_FLAGS Flags,
                                              __deref_inout_opt PVOID* NormalizationContext
);

NTSTATUS FLTAPI FilterNormalizeNameComponentEx( __in PFLT_INSTANCE Instance,
                                                __in PFILE_OBJECT FileObject,
                                                __in PCUNICODE_STRING ParentDirectory,
                                                __in USHORT VolumeNameLength,
                                                __in PCUNICODE_STRING Component,
                                                __out_bcount( ExpandComponentNameLength ) PFILE_NAMES_INFORMATION ExpandComponentName,
                                                __in ULONG ExpandComponentNameLength,
                                                __in FLT_NORMALIZE_NAME_FLAGS Flags,
                                                __deref_inout_opt PVOID* NormalizationContext
);

VOID FLTAPI FilterNormalizeContextCleanup( __in_opt PVOID* NormalizationContext );

EXTERN_C_END

#endif // HDR_ISOLATION_GENERATE_FILENAME