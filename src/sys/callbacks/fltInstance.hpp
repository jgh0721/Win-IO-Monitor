#ifndef HDR_WINIOMONITOR_INSTANCE
#define HDR_WINIOMONITOR_INSTANCE

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

/**
 * @brief The filter manager calls this routine on the first operation after a new volume is mounted.
 * @param FltObjects
 * @param Flags
 * @param VolumeDeviceType
 * @param VolumeFilesystemType
 * @return

    IRQL = PASSIVE_LEVEL
*/

/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    New instances are only created and attached to a volume if it is a writable
    NTFS or ReFS volume.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

    VolumeFilesystemType - A FLT_FSTYPE_* value indicating which file system type
        the Filter Manager is offering to attach us to.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
NTSTATUS FLTAPI InstanceSetup( PCFLT_RELATED_OBJECTS FltObjects,
                               FLT_INSTANCE_SETUP_FLAGS Flags,
                               DEVICE_TYPE VolumeDeviceType,
                               FLT_FILESYSTEM_TYPE VolumeFilesystemType );

/*++
Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
NTSTATUS FLTAPI InstanceQueryTeardown( __in PCFLT_RELATED_OBJECTS FltObjects,
                                       __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags );
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is been deleted.

Return Value:

    None.

--*/
VOID FLTAPI InstanceTeardownStart( __in PCFLT_RELATED_OBJECTS FltObjects,
                                   __in FLT_INSTANCE_TEARDOWN_FLAGS Flags );
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is been deleted.

Return Value:

    None.

--*/
VOID FLTAPI InstanceTeardownComplete( __in PCFLT_RELATED_OBJECTS FltObjects,
                                      __in FLT_INSTANCE_TEARDOWN_FLAGS Flags );

EXTERN_C_END;

NTSTATUS CreateInstanceContext( PCFLT_RELATED_OBJECTS FltObjects,
                               FLT_INSTANCE_SETUP_FLAGS Flags,
                               DEVICE_TYPE VolumeDeviceType,
                               FLT_FILESYSTEM_TYPE VolumeFilesystemType );

NTSTATUS CloseInstanceContext();

#endif // HDR_WINIOMONITOR_INSTANCE