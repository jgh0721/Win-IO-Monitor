#ifndef HDR_FLT_UTILITIES
#define HDR_FLT_UTILITIES

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

} // nsUtils

#endif // HDR_FLT_UTILITIES