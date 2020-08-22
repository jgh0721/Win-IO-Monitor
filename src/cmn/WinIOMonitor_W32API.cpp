#include "WinIOMonitor_W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

nsW32API::NtOsKrnlAPI nsW32API::NtOsKrnlAPIMgr;
nsW32API::FltMgrAPI nsW32API::FltMgrAPIMgr;

void nsW32API::InitializeFltMgrAPI( FltMgrAPI* fltMgrAPI )
{
    fltMgrAPI->pfnFltGetFileContext = ( FltMgrAPI::FltGetFileContext ) FltGetRoutineAddress( "FltGetFileContext" );
    fltMgrAPI->pfnFltSetFileContext = ( FltMgrAPI::FltSetFileContext ) FltGetRoutineAddress( "FltSetFileContext" );
}
