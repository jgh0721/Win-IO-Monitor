#ifndef HDR_FLT_CMNLIBS_STRING
#define HDR_FLT_CMNLIBS_STRING

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
     * @brief Find wszPattern in wszString from end position to begin
     * @param wszString 
     * @param wszPattern 
     * @return if found begin pattern begin pointer in wszString or NULLPTR

     *  IRQL = PAASIVE_LEVEL
    */
    WCHAR* EndsWithW( __in_z WCHAR* wszString, __in_z const WCHAR* wszPattern );
    WCHAR* StartsWithW( __in_z WCHAR* wszString, __in_z const WCHAR* wszPattern );

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
    
} // nsUtils

#endif // HDR_FLT_CMNLIBS_STRING