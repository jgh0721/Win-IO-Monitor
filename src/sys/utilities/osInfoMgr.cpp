#include "osInfoMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

nsUtils::nsDetail::OS_INFO nsUtils::nsDetail::GlobalOSInfo;

///////////////////////////////////////////////////////////////////////////////

namespace nsUtils
{
    void InitializeOSInfo()
    {
        RtlZeroMemory( &nsDetail::GlobalOSInfo, sizeof( nsDetail::GlobalOSInfo ) );

        nsDetail::GlobalOSInfo.IsCheckedBuild = PsGetVersion( &nsDetail::GlobalOSInfo.MajorVersion, &nsDetail::GlobalOSInfo.MinorVersion, &nsDetail::GlobalOSInfo.BuildNumber, NULL );

#ifdef _AMD64_
        nsDetail::GlobalOSInfo.CpuBits = 64;
#else
        nsDetail::GlobalOSInfo.CpuBits = 32;
#endif
    }

    bool VerifyVersionInfoEx( DWORD dwMajorVersion, const char* szCondition )
    {
        return VerifyVersionInfoEx( dwMajorVersion, -1, szCondition );
    }

    bool VerifyVersionInfoEx( DWORD dwMajorVersion, int iMinorVersion /*= -1 */, const char* strCondition /*= "=" */ )
    {
        bool nRet = false;

        do
        {
            if( ( strcmp( strCondition, "=" ) == 0 ) || ( strcmp( strCondition, "==" ) == 0 ) )
            {
                // dwMajorVersion 이 같음( iMinorVersion < 0 )
                // dwMajorVersion 과 iMinorVersion 이 같음
                if( nsDetail::GlobalOSInfo.MajorVersion == dwMajorVersion )
                    nRet = true;
                else
                    break;

                if( iMinorVersion >= 0 )
                {
                    // 현 시스템의 MinorVersion 이 조건과 같지 않다면 false
                    if( !( nsDetail::GlobalOSInfo.MinorVersion == ( unsigned int )iMinorVersion ) )
                        nRet = false;
                }
            }
            else if( strcmp( strCondition, ">=" ) == 0 )
            {
                // 현 시스템의 MajorVersion 이 조건보다 크거나 같은지 비교
                if( nsDetail::GlobalOSInfo.MajorVersion >= dwMajorVersion )
                    nRet = true;
                else
                    break;

                // 현 시스템의 MajorVersion 과 조건이 같은경우 MinorVersion 비교
                if( iMinorVersion >= 0 && nsDetail::GlobalOSInfo.MajorVersion == dwMajorVersion )
                {
                    // 현 시스템의 MinorVersion 이 조건보다 크거나 같지 않을경우 false
                    if( !( nsDetail::GlobalOSInfo.MinorVersion >= ( unsigned int )iMinorVersion ) )
                        nRet = false;
                }
            }
            else if( strcmp( strCondition, ">" ) == 0 )
            {
                // 현 시스템의 MajorVersion이 조건보다 크다면 바로 조건에 부합
                if( nsDetail::GlobalOSInfo.MajorVersion > dwMajorVersion )
                {
                    nRet = true;
                    break;
                }

                // 현 시스템의 MajorVersion 과 조건이 같은경우 MinorVersion 비교
                if( iMinorVersion >= 0 && nsDetail::GlobalOSInfo.MajorVersion == dwMajorVersion )
                {
                    // 현 시스템의 MinorVersion 이 조건보다 클 경우 true
                    if( nsDetail::GlobalOSInfo.MinorVersion > ( unsigned int )iMinorVersion )
                        nRet = true;
                }
            }
            else if( strcmp( strCondition, "<=" ) == 0 )
            {
                // 현 시스템의 MajorVersion 이 조건보다 크거나 같은지 비교
                if( nsDetail::GlobalOSInfo.MajorVersion <= dwMajorVersion )
                    nRet = true;
                else
                    break;

                // 현 시스템의 MajorVersion 과 조건이 같은경우 MinorVersion 비교
                if( iMinorVersion >= 0 && nsDetail::GlobalOSInfo.MajorVersion == dwMajorVersion )
                {
                    // 현 시스템의 MinorVersion 이 조건보다 작거나 같지 않을경우 false
                    if( !( nsDetail::GlobalOSInfo.MinorVersion <= ( unsigned int )iMinorVersion ) )
                        nRet = false;
                }
            }
            else if( strcmp( strCondition, "<" ) == 0 )
            {
                // 현 시스템의 MajorVersion이 조건보다 작다면 바로 조건에 부합
                if( nsDetail::GlobalOSInfo.MajorVersion < dwMajorVersion )
                {
                    nRet = true;
                    break;
                }

                // 현 시스템의 MajorVersion 과 조건이 같은경우 MinorVersion 비교
                if( iMinorVersion >= 0 && nsDetail::GlobalOSInfo.MajorVersion == dwMajorVersion )
                {
                    // 현 시스템의 MinorVersion 이 조건보다 작을 경우 true
                    if( nsDetail::GlobalOSInfo.MinorVersion < ( unsigned int )iMinorVersion )
                        nRet = true;
                }
            }

        } while( false );

        return nRet;
    }

} // nsUtils
