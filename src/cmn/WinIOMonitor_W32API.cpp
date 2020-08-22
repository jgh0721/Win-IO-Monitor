#include "WinIOMonitor_W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

nsW32API::NtOsKrnlAPI nsW32API::NtOsKrnlAPIMgr;
nsW32API::FltMgrAPI nsW32API::FltMgrAPIMgr;

///////////////////////////////////////////////////////////////////////////////

void nsW32API::InitializeNtOsKrnlAPI( NtOsKrnlAPI* ntOsKrnlAPI )
{
    ASSERT( ntOsKrnlAPI != NULLPTR );

    UNICODE_STRING routineName;

    RtlInitUnicodeString( &routineName, L"PsSetLoadImageNotifyRoutine" );
    ntOsKrnlAPI->pfnPsSetLoadImageNotifyRoutine = ( NtOsKrnlAPI::PsSetLoadImageNotifyRoutine )MmGetSystemRoutineAddress( &routineName );

    RtlInitUnicodeString( &routineName, L"PsSetCreateProcessNotifyRoutine" );
    ntOsKrnlAPI->pfnPsSetCreateProcessNotifyRoutine = ( NtOsKrnlAPI::PsSetCreateProcessNotifyRoutine )MmGetSystemRoutineAddress( &routineName );

    RtlInitUnicodeString( &routineName, L"PsSetCreateProcessNotifyRoutineEx" );
    ntOsKrnlAPI->pfnPsSetCreateProcessNotifyRoutineEx = ( NtOsKrnlAPI::PsSetCreateProcessNotifyRoutineEx )MmGetSystemRoutineAddress( &routineName );

    RtlInitUnicodeString( &routineName, L"IoGetTransactionParameterBlock" );
    ntOsKrnlAPI->pfnIoGetTransactionParameterBlock = ( NtOsKrnlAPI::IoGetTransactionParameterBlock )MmGetSystemRoutineAddress( &routineName );

    RtlInitUnicodeString( &routineName, L"SeLocateProcessImageName" );
    ntOsKrnlAPI->pfnSeLocateProcessImageName = ( NtOsKrnlAPI::SeLocateProcessImageName )MmGetSystemRoutineAddress( &routineName );

    RtlInitUnicodeString( &routineName, L"ZwQueryInformationProcess" );
    ntOsKrnlAPI->pfnZwQueryInformationProcess = ( NtOsKrnlAPI::ZwQueryInformationProcess )MmGetSystemRoutineAddress( &routineName );
}

void nsW32API::InitializeFltMgrAPI( FltMgrAPI* fltMgrAPI )
{
    ASSERT( fltMgrAPI != NULLPTR );

    fltMgrAPI->pfnFltGetFileContext = ( FltMgrAPI::FltGetFileContext ) FltGetRoutineAddress( "FltGetFileContext" );
    fltMgrAPI->pfnFltSetFileContext = ( FltMgrAPI::FltSetFileContext ) FltGetRoutineAddress( "FltSetFileContext" );
}
