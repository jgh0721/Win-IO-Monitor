#ifndef HDR_WINIOMONITOR_PROCNAMEMGR
#define HDR_WINIOMONITOR_PROCNAMEMGR

#include "bufferMgr.hpp"
#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

typedef struct _PROCESS_INFO
{
    PEPROCESS               Process;

    ULONG                   ParentProcessId;
    ULONG                   ProcessId;

    TyGenericBuffer<WCHAR>  ProcessFileFullPath;
    UNICODE_STRING*         ProcessFileFullPathUni;
    WCHAR*                  ProcessName;            // just point to processname from ProcessFileFullPath or ProcessFileFullPathUni, dont free this pointer.

    LIST_ENTRY              ListEntry;
} PROCESS_INFO, *PPROCESS_INFO;

NTSTATUS StartProcessNotify();
NTSTATUS StopProcessNotify();

/**
 * @brief PsSetCreateProcessNotifyRoutine 의 콜백함수
 * @param ParentId
 * @param ProcessId
 * @param Create
*/
void CreateProcessNotifyRoutine( __in HANDLE ParentId, __in HANDLE ProcessId, __in BOOLEAN Create );

/**
 * @brief EPROCESS 를 통해 해당 프로세스 핸들을 반환
 * @param Process
 * @return 프로세스의 핸들 반환, 실패하면 NULL 반환

    IRQL <= APC_LEVEL, 호출자는 반드시 핸들을 닫아야함( ZwClose )
*/
HANDLE GetProcessHandleFromEPROCESS( __in PEPROCESS Process );
/**
 * @brief 프로세스 핸들로부터 프로세스 경로 및 이름을 반환
 * @param ProcessHandle
 * @param ProcessName
 * @return

    IRQL <= PASSVIE_LEVEL, 호출자는 반드시 메모리를 해제해야한다( ExFreePool )
*/
NTSTATUS GetProcessNameByHandle( __in HANDLE ProcessHandle, __out PUNICODE_STRING* ProcessName );

/**
 * @brief 지정한 ProcessId 를 통해 프로세스 정보를 가져옵니다
 * @param ProcessId 
 * @param ProcessFileFullPath 해당 프로세스에 대한 전체경로
 * @param wszProcessName 경로를 제외한 프로세스 이름, 해당 포인터는 단순히 ProcessFileFullPath 를 가르키고 있으므로, 별도로 해제해선 안됨
 * @return STATUS_SUCCESS, STATUS_NOT_FOUND
*/
NTSTATUS SearchProcessInfo( __in ULONG ProcessId, __out TyGenericBuffer<WCHAR>* ProcessFileFullPath, __out_opt PWCH* wszProcessName );

#endif // HDR_WINIOMONITOR_PROCNAMEMGR