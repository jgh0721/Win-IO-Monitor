#ifndef HDR_FLT_UTILITIES
#define HDR_FLT_UTILITIES

#include "bufferMgr_Defs.hpp"
#include "contextMgr_Defs.hpp"
#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsUtils
{
    /**
     * @brief 문자열 길이를 반환하는 함수
     * @param str
     * @return 반환하는 길이에는 NULL 문자를 포함하지 않음

        IRQL = PASSIVE_LEVEL
    */
    size_t strlength( __in_z const wchar_t* str );
    size_t strlength( __in_z const char* str );

    WCHAR* ReverseFindW( __in_z WCHAR* wszString, WCHAR ch );
    WCHAR* ForwardFindW( __in_z WCHAR* wszString, WCHAR ch );
    WCHAR* UpperWString( __inout_z WCHAR* wszString );

    /**
     * @brief 지정한 문자열에서 와일드카드(*,?) 를 이용하여 일치하는지 검사
     * @param pszString
     * @param pszMatch
     * @param isCaseSensitive
     * @return

        IRQL <= APC_LEVEL
        http://www.codeproject.com/Articles/188256/A-Simple-Wildcard-Matching-Function
    */
    bool                                    WildcardMatch_straight( const char* pszString, const char* pszMatch, bool isCaseSensitive = false );
    bool                                    WildcardMatch_straight( const wchar_t* pszString, const wchar_t* pszMatch, bool isCaseSensitive = false );

    NTSTATUS                                DfAllocateUnicodeString( __inout PUNICODE_STRING String );
    VOID                                    DfFreeUnicodeString( __inout PUNICODE_STRING String );

    ///////////////////////////////////////////////////////////////////////////
    /// File, Path...

    /**
     * @brief 장치이름을 이용하여 대응하는 드라이브 문자를 반환
     * @param uniDeviceName
     * @param wchDriveLetter, C, D 등의 문자를 반환, 찾을 수 없다면 0 반환
     * @return TRUE 또는 FALSE
        IRQL = PASSIVE_LEVEL
    */
    BOOLEAN                                 FindDriveLetterByDeviceName( __in UNICODE_STRING* uniDeviceName, __out WCHAR* wchDriveLetter );


    TyGenericBuffer< WCHAR >                ExtractFileFullPath( __in PFILE_OBJECT FileObject, __in_opt CTX_INSTANCE_CONTEXT* InstanceContext, __in bool IsInCreate );
    NTSTATUS                                DfGetVolumeGuidName( __in PCFLT_RELATED_OBJECTS FltObjects, __inout PUNICODE_STRING VolumeGuidName );
    NTSTATUS                                DfBuildFileIdString( __in PFLT_CALLBACK_DATA Data,
                                                                 __in PCFLT_RELATED_OBJECTS FltObjects,
                                                                 __in PCTX_STREAM_CONTEXT StreamContext,
                                                                 __out PUNICODE_STRING String );

    NTSTATUS                                DfDetectDeleteByFileId( __in PFLT_CALLBACK_DATA Data,
                                                                    __in PCFLT_RELATED_OBJECTS FltObjects,
                                                                    __in PCTX_STREAM_CONTEXT StreamContext );

    /*++

    Routine Description:

        This routine gets and parses the file name information, obtains the File
        ID and saves them in the stream context.

    Arguments:

        Data  - Pointer to FLT_CALLBACK_DATA.

        StreamContext - Pointer to stream context that will receive the file
                        information.

    Return Value:

        Returns statuses forwarded from Flt(Get|Parse)FileNameInformation or
        FltQueryInformationFile.

    --*/
    NTSTATUS                                DfGetFileNameInformation( __in PFLT_CALLBACK_DATA Data, __inout PCTX_STREAM_CONTEXT StreamContext );
    NTSTATUS                                DfGetFileId( __in PFLT_CALLBACK_DATA Data, __inout PCTX_STREAM_CONTEXT StreamContext );

    NTSTATUS                                DfIsFileDeleted( __in PFLT_CALLBACK_DATA Data,
                                                             __in PCFLT_RELATED_OBJECTS FltObjects,
                                                             __in PCTX_STREAM_CONTEXT StreamContext,
                                                             __in BOOLEAN IsTransaction );

} // nsUtils

#endif // HDR_FLT_UTILITIES