#include "driverMgmt.hpp"

#include "metadata/Metadata.hpp"
#include "communication/Communication.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FEATURE_CONTEXT FeatureContext;

NTSTATUS InitializeFeatureMgr()
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        RtlZeroMemory( &FeatureContext, sizeof( FEATURE_CONTEXT ) );

        IF_FAILED_BREAK( Status, InitializeMetaDataMgr() );
        IF_FAILED_BREAK( Status, InitializeNotifyEventWorker() );
        
    } while( false );

    return Status;
}

NTSTATUS UninitializeFeatureMgr()
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        IF_FAILED_BREAK( Status, UninitializeNotifyEventWorker() );
        IF_FAILED_BREAK( Status, UninitializeMetaDataMgr() );
        
    } while( false );

    return Status;
}
