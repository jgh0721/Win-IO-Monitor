#include "fltInstance.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS InstanceSetup( PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_SETUP_FLAGS Flags, ULONG VolumeDeviceType,
                        FLT_FILESYSTEM_TYPE VolumeFilesystemType )
{
    return STATUS_SUCCESS;
}

NTSTATUS InstanceQueryTeardown( PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags )
{
    return STATUS_SUCCESS;
}

void InstanceTeardownStart( PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_TEARDOWN_FLAGS Flags )
{
}

void InstanceTeardownComplete( PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_TEARDOWN_FLAGS Flags )
{
}
