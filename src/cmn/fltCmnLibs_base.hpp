#ifndef HDR_FLT_CMNLIBS_BASE
#define HDR_FLT_CMNLIBS_BASE

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsUtils
{
    /*!
     * FLT_CALLBACK_DATA 의 인자를 분석하여 사용할 수 있는 사용자 버퍼를 반환한다
     */
    PVOID MakeUserBuffer( __in PFLT_CALLBACK_DATA Data );

} // nsUtils

#endif // HDR_FLT_CMNLIBS_BASE