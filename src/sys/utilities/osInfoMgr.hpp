#ifndef HDR_UTIL_OSINFOMGR
#define HDR_UTIL_OSINFOMGR

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsUtils
{
    namespace nsDetail
    {
        typedef struct _OS_INFO
        {
            ULONG           MajorVersion;
            ULONG           MinorVersion;
            ULONG           BuildNumber;

            ULONG           CpuBits;

            BOOLEAN         IsCheckedBuild;
        } OS_INFO, * POS_INFO;

        extern OS_INFO GlobalOSInfo;
    }

    VOID InitializeOSInfo();

    /*!
        현재 OS 의 버전을 비교하여 지정한 조건에 부합하는지 판단합니다.

        함수가 작동하는 시스템이 6.0, 조건으로 5.3 >= 를 지정했다면 true 가 반환된다.
        함수가 작동하는 시스템이 5.2, 조건으로 5.1 > 를 지정했다면 true 가 반환된다.
        함수가 작동하는 시스템이 6.1, 조건으로 5.2 >= 를 지정했다면 true 가 반환된다.
        함수가 작동하는 시스템이 6.0, 조건으로 5.3 <= 를 지정했다면 false 가 반환된다.
        함수가 작동하는 시스템이 5.1, 조건으로 5.2 <= 를 지정했다면 true 가 반환된다.
        함수가 작동하는 시스템이 5.2, 조건으로 6.1 <= 를 지정했다면 true 가 반환된다.

        ※ iMinorVersion 이 -1일 경우 메이저 버전만 비교
        ※ 항상 NTDLL 을 동적 로딩하여 비교함

        @return true = 함수가 작동하는 시스템이 지정한 조건에 부합함
        @retrun false = 함수가 작동하는 시스템이 지정한 조건에 부합하지 않거나 함수 작동 실패
    */
    bool VerifyVersionInfoEx( __in DWORD dwMajorVersion, const char* szCondition = "=" );
    bool VerifyVersionInfoEx( __in DWORD dwMajorVersion, __in int iMinorVersion, const char* szCondition = "=" );

} // nsUtils

#endif // HDR_UTIL_OSINFOMGR