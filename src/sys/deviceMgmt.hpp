#ifndef HDR_DEVICE_MGMT
#define HDR_DEVICE_MGMT

#include "fltBase.hpp"
#include "utilities/contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS CreateControlDevice( __in CTX_GLOBAL_DATA& GlobalContext );
NTSTATUS RemoveControlDevice( __in CTX_GLOBAL_DATA& GlobalContext );

///////////////////////////////////////////////////////////////////////////////

/*!
    User-Mode 어플리케이션에서 CreateFile/CloseHandle 를 통해 장치 핸들을 획득 및 해제할 때 호출된다
*/

NTSTATUS DeviceCreate( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS DeviceCleanup( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS DeviceClose( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );

#endif // HDR_DEVICE_MGMT