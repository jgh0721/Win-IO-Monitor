#ifndef HDR_WINIOISOLATION_VOLUMENAME_MGR
#define HDR_WINIOISOLATION_VOLUMENAME_MGR

#include "fltBase.hpp"
#include "volumeNameMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*!
 *
 */

NTSTATUS InitializeVolumeNameMgr();
NTSTATUS UninitializeVolumeNameMgr();

NTSTATUS VolumeMgr_Add( __in const WCHAR* DeviceVolumeName, __in WCHAR DriveLetter );
NTSTATUS VolumeMgr_Remove( __in const WCHAR* DeviceVolumeName );

VOID VolumeMgr_Replace( __inout_bcount_z( BufferSize ) WCHAR* Path, __in ULONG BufferSize );

#endif // HDR_WINIOISOLATION_VOLUMENAME_MGR