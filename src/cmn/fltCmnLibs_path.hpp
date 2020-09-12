#ifndef HDR_FLT_CMNLIBS_PATH
#define HDR_FLT_CMNLIBS_PATH

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsUtils
{
    /**
     * @brief 장치이름을 이용하여 대응하는 드라이브 문자를 반환
     * @param uniDeviceName
     * @param wchDriveLetter, C, D 등의 문자를 반환, 찾을 수 없다면 0 반환
     * @return TRUE 또는 FALSE
        IRQL = PASSIVE_LEVEL
    */
    BOOLEAN                                 FindDriveLetterByDeviceName( __in UNICODE_STRING* uniDeviceName, __out WCHAR* wchDriveLetter );
    
} // nsUtils

#endif // HDR_FLT_CMNLIBS_PATH