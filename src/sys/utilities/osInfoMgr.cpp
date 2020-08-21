#include "osInfoMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif


nsUtils::nsDetail::OS_INFO GlobalOSInfo;

///////////////////////////////////////////////////////////////////////////////

namespace nsUtils
{
    void InitializeOSInfo( )
    {
        RtlZeroMemory( &GlobalOSInfo, sizeof( GlobalOSInfo ) );

        GlobalOSInfo.IsCheckedBuild = PsGetVersion( &GlobalOSInfo.MajorVersion, &GlobalOSInfo.MinorVersion, &GlobalOSInfo.BuildNumber, NULL );

#ifdef _AMD64_
        GlobalOSInfo.CpuBits = 64;
#else
        GlobalOSInfo.CpuBits = 32;
#endif
    }

} // nsUtils
