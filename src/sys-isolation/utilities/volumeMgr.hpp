#ifndef HDR_WINIOISOLATION_VOLUMENAME_MGR
#define HDR_WINIOISOLATION_VOLUMENAME_MGR

#include "fltBase.hpp"

#include "volumeMgr_Defs.hpp"
#include "contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*!
 *
 */

NTSTATUS InitializeVolumeNameMgr();
NTSTATUS UninitializeVolumeNameMgr();

NTSTATUS VolumeMgr_Add( __in const WCHAR* DeviceVolumeName, __in WCHAR DriveLetter, __in CTX_INSTANCE_CONTEXT* InstanceContext );
NTSTATUS VolumeMgr_Remove( __in const WCHAR* DeviceVolumeName );

VOID VolumeMgr_Replace( __inout_bcount_z( BufferSize ) WCHAR* Path, __in ULONG BufferSize );

/**
 * @brief 
 * @param DriveLetter 
 * @return InstanceContext
    호출자는 사용 후 반드시 CtxReleaseContext 를 호출해야한다
*/
CTX_INSTANCE_CONTEXT* VolumeMgr_SearchContext( __in WCHAR DriveLetter );
CTX_INSTANCE_CONTEXT* VolumeMgr_SearchContext( __in const WCHAR* Win32FilePath );

#endif // HDR_WINIOISOLATION_VOLUMENAME_MGR