#ifndef HDR_W32API
#define HDR_W32API

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#include "W32API_Base.hpp"
#include "W32API_NtOsKrnl.hpp"
#include "W32API_FltMgr.hpp"

namespace nsW32API
{
    /**
     * @brief The FltIsVolumeWritable routine determines whether the disk device that corresponds to a volume or minifilter driver instance is writable. 
     * @param FltObject  Volume or Instance Object
     * @param IsWritable 
     * @return
     *
     * Support WinXP
    */
    NTSTATUS IsVolumeWritable( __in PVOID FltObject, __out PBOOLEAN IsWritable );

} // nsW32API

#endif // HDR_W32API