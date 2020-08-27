#include "procNameMgr.hpp"


#include "fltUtilities.hpp"
#include "utilities/osInfoMgr.hpp"
#include "WinIOMonitor_W32API.hpp"

#if defined(_MSC_VER)
#	pragma warning( disable: 4311 )
#	pragma warning( disable: 4312 )
#	pragma warning( disable: 4302 )
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsDetail
{
	LIST_ENTRY	ProcInfoListHead;
	ERESOURCE	ProcInfoLock;

	//
	//BEGIN PROCESS STRUCTURE FOR WIN2K+X86
	//
	typedef struct _RTL_USER_PROCESS_PARAMETERS_V50_X86
	{
		char Pading[ 0x38 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V50_X86, * PRTL_USER_PROCESS_PARAMETERS_V50_X86;

	typedef struct _PEB_V50_X86
	{
		char  Pading[ 0x10 ];

		PRTL_USER_PROCESS_PARAMETERS_V50_X86 ProcessParameters;

	}PEB_V50_X86, * PPEB_V50_X86;

	typedef struct _EPROCESS_V50_X86
	{
		char  Pading[ 0x1B0 ];

		PPEB_V50_X86  Peb;

	}EPROCESS_V50_X86, * PEPROCESS_V50_X86;

	//
	//BEGIN PROCESS STRUCTURE FOR WINXP+X86
	//
	typedef struct _RTL_USER_PROCESS_PARAMETERS_V51_X86
	{
		char Pading[ 0x38 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V51_X86, * PRTL_USER_PROCESS_PARAMETERS_V51_X86;

	typedef struct _PEB_V51_X86
	{
		char  Pading[ 0x10 ];

		PRTL_USER_PROCESS_PARAMETERS_V51_X86 ProcessParameters;

	}PEB_V51_X86, * PPEB_V51_X86;

	typedef struct _EPROCESS_V51_X86
	{
		char  Pading[ 0x1B0 ];

		PPEB_V51_X86  Peb;

	}EPROCESS_V51_X86, * PEPROCESS_V51_X86;

	//
	//BEGIN PROCESS STRUCTURE FOR WIN2K3+X86
	//
	typedef struct _RTL_USER_PROCESS_PARAMETERS_V52_X86
	{
		char Pading[ 0x38 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V52_X86, * PRTL_USER_PROCESS_PARAMETERS_V52_X86;

	typedef struct _PEB_V52_X86
	{
		char  Pading[ 0x10 ];

		PRTL_USER_PROCESS_PARAMETERS_V52_X86 ProcessParameters;

	}PEB_V52_X86, * PPEB_V52_X86;

	typedef struct _EPROCESS_V52_X86
	{
		char  Pading[ 0x1A0 ];

		PPEB_V52_X86  Peb;

	}EPROCESS_V52_X86, * PEPROCESS_V52_X86;

	//
	//BEGIN PROCESS STRUCTURE FOR VISTA+X86
	//
	typedef struct _RTL_USER_PROCESS_PARAMETERS_V60_X86
	{
		char Pading[ 0x38 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V60_X86, * PRTL_USER_PROCESS_PARAMETERS_V60_X86;

	typedef struct _PEB_V60_X86
	{
		char  Pading[ 0x10 ];

		PRTL_USER_PROCESS_PARAMETERS_V60_X86 ProcessParameters;

	}PEB_V60_X86, * PPEB_V60_X86;

	typedef struct _EPROCESS_V60_X86
	{
		char  Pading[ 0x188 ];

		PPEB_V60_X86  Peb;

	}EPROCESS_V60_X86, * PEPROCESS_V60_X86;


	//
	//BEGIN PROCESS STRUCTURE FOR WIN7+X86
	//
	typedef struct _RTL_USER_PROCESS_PARAMETERS_V61_X86
	{
		char Pading[ 0x38 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V61_X86, * PRTL_USER_PROCESS_PARAMETERS_V61_X86;

	typedef struct _PEB_V61_X86
	{
		char  Pading[ 0x10 ];

		PRTL_USER_PROCESS_PARAMETERS_V61_X86 ProcessParameters;

	}PEB_V61_X86, * PPEB_V61_X86;

	typedef struct _EPROCESS_V61_X86
	{
		char  Pading[ 0x1A8 ];

		PPEB_V61_X86  Peb;

	}EPROCESS_V61_X86, * PEPROCESS_V61_X86;

	//
	//BEGIN PROCESS STRUCTURE FOR WIN8+X86
	//
	typedef struct _RTL_USER_PROCESS_PARAMETERS_V62_X86
	{
		char Pading[ 0x38 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V62_X86, * PRTL_USER_PROCESS_PARAMETERS_V62_X86;

	typedef struct _PEB_V62_X86
	{
		char  Pading[ 0x10 ];

		PRTL_USER_PROCESS_PARAMETERS_V62_X86 ProcessParameters;

	}PEB_V62_X86, * PPEB_V62_X86;

	typedef struct _EPROCESS_V62_X86
	{
		char  Pading[ 0x140 ];

		PPEB_V62_X86  Peb;

	}EPROCESS_V62_X86, * PEPROCESS_V62_X86;

	//
	//BEGIN PROCESS STRUCTURE FOR WIN8.1+X86
	//
	typedef struct _RTL_USER_PROCESS_PARAMETERS_V63_X86
	{
		char Pading[ 0x38 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V63_X86, * PRTL_USER_PROCESS_PARAMETERS_V63_X86;

	typedef struct _PEB_V63_X86
	{
		char  Pading[ 0x10 ];

		PRTL_USER_PROCESS_PARAMETERS_V63_X86 ProcessParameters;

	}PEB_V63_X86, * PPEB_V63_X86;

	typedef struct _EPROCESS_V63_X86
	{
		char  Pading[ 0x140 ];

		PPEB_V63_X86  Peb;

	}EPROCESS_V63_X86, * PEPROCESS_V63_X86;


	//
	//BEGIN PROCESS STRUCTURE FOR WIN10+X86
	//
	typedef struct _RTL_USER_PROCESS_PARAMETERS_V100_X86
	{
		char Pading[ 0x38 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V100_X86, * PRTL_USER_PROCESS_PARAMETERS_V100_X86;

	typedef struct _PEB_V100_X86
	{
		char  Pading[ 0x10 ];

		PRTL_USER_PROCESS_PARAMETERS_V100_X86 ProcessParameters;

	}PEB_V100_X86, * PPEB_V100_X86;

	typedef struct _EPROCESS_V100_X86
	{
		char  Pading[ 0x144 ];

		PPEB_V100_X86  Peb;

	}EPROCESS_V100_X86, * PEPROCESS_V100_X86;


	//
	//BEGIN PROCESS STRUCTURE FOR WIN7+X64
	//
	typedef struct _RTL_USER_PROCESS_PARAMETERS_V61_AMD64
	{
		char            Pad[ 0x60 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V61_AMD64, * PRTL_USER_PROCESS_PARAMETERS_V61_AMD64;

	typedef struct _PPEB_V61_AMD64
	{
		char  Pad[ 0x20 ];

		PRTL_USER_PROCESS_PARAMETERS_V61_AMD64 ProcessParameters;

	}PEB_V61_AMD64, * PPEB_V61_AMD64;

	typedef struct _EPROCESS_V61_AMD64
	{
		char Pad[ 0x338 ];

		PPEB_V61_AMD64  Peb;

	}EPROCESS_V61_AMD64, * PEPROCESS_V61_AMD64;

	//
	//BEGIN PROCESS STRUCTURE FOR WIN8+X64
	//
	typedef struct _RTL_USER_PROCESS_PARAMETERS_V62_AMD64
	{
		char            Pad[ 0x60 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V62_AMD64, * PRTL_USER_PROCESS_PARAMETERS_V62_AMD64;

	typedef struct _PPEB_V62_AMD64
	{
		char  Pad[ 0x20 ];

		PRTL_USER_PROCESS_PARAMETERS_V62_AMD64 ProcessParameters;

	}PEB_V62_AMD64, * PPEB_V62_AMD64;

	typedef struct _EPROCESS_V62_AMD64
	{
		char Pad[ 0x3e8 ];

		PPEB_V62_AMD64  Peb;

	}EPROCESS_V62_AMD64, * PEPROCESS_V62_AMD64;

	//
	//BEGIN PROCESS STRUCTURE FOR WIN8.1+X64
	//

	typedef struct _RTL_USER_PROCESS_PARAMETERS_V63_AMD64
	{
		char            Pad[ 0x60 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V63_AMD64, * PRTL_USER_PROCESS_PARAMETERS_V63_AMD64;

	typedef struct _PPEB_V63_AMD64
	{
		char  Pad[ 0x20 ];

		PRTL_USER_PROCESS_PARAMETERS_V63_AMD64 ProcessParameters;

	}PEB_V63_AMD64, * PPEB_V63_AMD64;


	typedef struct _EPROCESS_V63_AMD64
	{
		char Pad[ 0x3e8 ];

		PPEB_V63_AMD64  Peb;

	}EPROCESS_V63_AMD64, * PEPROCESS_V63_AMD64;

	//
	//BEGIN PROCESS STRUCTURE FOR WIN10+X64
	//

	typedef struct _RTL_USER_PROCESS_PARAMETERS_V100_AMD64
	{
		char            Pad[ 0x60 ];

		UNICODE_STRING  ImagePathName;

	}RTL_USER_PROCESS_PARAMETERS_V100_AMD64, * PRTL_USER_PROCESS_PARAMETERS_V100_AMD64;

	typedef struct _PPEB_V100_AMD64
	{
		char  Pad[ 0x20 ];

		PRTL_USER_PROCESS_PARAMETERS_V100_AMD64 ProcessParameters;

	}PEB_V100_AMD64, * PPEB_V100_AMD64;

	typedef struct _EPROCESS_V100_AMD64
	{
		char Pad[ 0x3F8 ];

		PPEB_V100_AMD64  Peb;

	}EPROCESS_V100_AMD64, * PEPROCESS_V100_AMD64;

}

/**
 * @brief 프로세스 정보를 관리하기 위한 셋을 생성
 * @return
*/
void CreateProcInfoSet();
void CloseProcInfoSet();

void InsertProcessInfo( __in ULONG uParentProcessId, __in ULONG uProcessId );
void DeleteProcessInfo( __in ULONG uProcessId );

BOOLEAN GetProcessNameFromEPROCESS( __in ULONG ProcessId, __in PEPROCESS Process, __out WCHAR* wszProcessName, __out ULONG bufferSize );

///////////////////////////////////////////////////////////////////////////////

NTSTATUS StartProcessNotify()
{
    CreateProcInfoSet();
    return PsSetCreateProcessNotifyRoutine( CreateProcessNotifyRoutine, FALSE );
}

NTSTATUS StopProcessNotify()
{
    NTSTATUS Status = PsSetCreateProcessNotifyRoutine( CreateProcessNotifyRoutine, TRUE );
    CloseProcInfoSet();
    return Status;
}

void CreateProcessNotifyRoutine( HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create )
{
	if( Create == TRUE )
	{
		/// Process Creation
		InsertProcessInfo( ( ULONG )ParentId, ( ULONG )ProcessId );
	}
	else
	{
		/// Process Termination
		DeleteProcessInfo( ( ULONG )ProcessId );
	}
}

void CreateProcInfoSet()
{
	RtlZeroMemory( &nsDetail::ProcInfoListHead, sizeof( nsDetail::ProcInfoListHead ) );
	RtlZeroMemory( &nsDetail::ProcInfoLock, sizeof( nsDetail::ProcInfoLock ) );

	InitializeListHead( &nsDetail::ProcInfoListHead );
	ExInitializeResourceLite( &nsDetail::ProcInfoLock );
}

void CloseProcInfoSet()
{
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite( &nsDetail::ProcInfoLock, TRUE );

	LIST_ENTRY* Head = &nsDetail::ProcInfoListHead;

	while( IsListEmpty( Head ) == FALSE )
	{
		LIST_ENTRY* current = RemoveHeadList( Head );
		PROCESS_INFO* procInfo = CONTAINING_RECORD( current, PROCESS_INFO, ListEntry );

	    if( procInfo != NULLPTR )
		{
			if( procInfo->Process != NULLPTR )
				ObDereferenceObject( procInfo->Process );

			if( procInfo->ProcessFileFullPathUni != NULL )
				ExFreePool( procInfo->ProcessFileFullPathUni );

			DeallocateBuffer( &procInfo->ProcessFileFullPath );
			ExFreePool( procInfo );
		}
	}

	ExReleaseResourceLite( &nsDetail::ProcInfoLock );
	KeLeaveCriticalRegion();

	ExDeleteResourceLite( &nsDetail::ProcInfoLock );
}

void InsertProcessInfo( __in ULONG uParentProcessId, ULONG uProcessId )
{
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite( &nsDetail::ProcInfoLock, TRUE );

	do
	{
		PROCESS_INFO* procInfo = ( PROCESS_INFO* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_INFO ), 'proc' );
		if( procInfo == NULL )
		{
			KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[WinIOMon] %s %s\n",
						 __FUNCTION__, "ExAllocatePoolWithTag FAILED" ) );
			break;
		}

		RtlZeroMemory( procInfo, sizeof( PROCESS_INFO ) );
		procInfo->ParentProcessId = ( ULONG )uParentProcessId;
		procInfo->ProcessId = ( ULONG )uProcessId;
		PsLookupProcessByProcessId( ( HANDLE )uProcessId, &procInfo->Process );

        do
        {
			if( nsW32API::NtOsKrnlAPIMgr.pfnSeLocateProcessImageName != NULLPTR && procInfo->Process != NULLPTR )
				nsW32API::NtOsKrnlAPIMgr.pfnSeLocateProcessImageName( procInfo->Process, &procInfo->ProcessFileFullPathUni );

			if( procInfo->ProcessFileFullPathUni != NULLPTR )
				break;

			HANDLE hProcess = GetProcessHandleFromEPROCESS( procInfo->Process );
			if( hProcess != NULL )
			{
				NTSTATUS Status = GetProcessNameByHandle( hProcess, &procInfo->ProcessFileFullPathUni );

				if( !NT_SUCCESS( Status ) || ( NT_SUCCESS( Status ) && wcslen( procInfo->ProcessFileFullPathUni->Buffer ) == 0 ) )
				{
					procInfo->ProcessFileFullPath = AllocateBuffer<WCHAR>( BUFFER_PROCNAME );
					if( procInfo->ProcessFileFullPath.Buffer != NULLPTR )
					{
						GetProcessNameFromEPROCESS( uProcessId, procInfo->Process, 
													procInfo->ProcessFileFullPath.Buffer, procInfo->ProcessFileFullPath.BufferSize );
					}
				}

				ZwClose( hProcess );
			}

        } while( false );

		InsertHeadList( &nsDetail::ProcInfoListHead, &procInfo->ListEntry );

	} while( false );

	ExReleaseResourceLite( &nsDetail::ProcInfoLock );
	KeLeaveCriticalRegion();
}

void DeleteProcessInfo( ULONG uProcessId )
{
	if( uProcessId <= 4 )
		return;

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite( &nsDetail::ProcInfoLock, TRUE );

	for( PLIST_ENTRY current = nsDetail::ProcInfoListHead.Flink; current != &nsDetail::ProcInfoListHead; )
	{
		PROCESS_INFO* entry = CONTAINING_RECORD( current, PROCESS_INFO, ListEntry );

		current = current->Flink;

		if( entry->ProcessId != uProcessId )
			continue;

		RemoveEntryList( &entry->ListEntry );

		if( entry->Process != NULLPTR )
			ObDereferenceObject( entry->Process );

		if( entry->ProcessFileFullPathUni != NULL )
			ExFreePool( entry->ProcessFileFullPathUni );

		DeallocateBuffer( &entry->ProcessFileFullPath );
		ExFreePool( entry );
		break;
	}

	ExReleaseResourceLite( &nsDetail::ProcInfoLock );
	KeLeaveCriticalRegion();
}

HANDLE GetProcessHandleFromEPROCESS( PEPROCESS Process )
{
	HANDLE hProcess = NULL;
	NTSTATUS Status = STATUS_SUCCESS;

	do
	{
		Status = ObOpenObjectByPointer( Process, 0, NULL, 0, 0, KernelMode, &hProcess );

		if( !NT_SUCCESS( Status ) )
		{
			KdPrintEx( ( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[WinIOMon] %s %s Status=0x%08x\n", 
						 __FUNCTION__, "ObOpenObjectByPointer FAILED", Status ) );

			return NULL;
		}

	} while( false );

	return hProcess;
}

NTSTATUS GetProcessNameByHandle( HANDLE ProcessHandle, PUNICODE_STRING* ProcessName )
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	ULONG ProcessImageFilePathBytes = 512;
	PUNICODE_STRING ProcessImageFilePath = NULL;

	ASSERT( ProcessName != NULLPTR );

    do
    {
		if( ProcessName == NULLPTR )
			break;

		if( nsW32API::NtOsKrnlAPIMgr.pfnZwQueryInformationProcess == NULLPTR )
			break;

		ProcessImageFilePath = ( PUNICODE_STRING )ExAllocatePoolWithTag( PagedPool, ProcessImageFilePathBytes, 'fdew' );

		if( ProcessImageFilePath != NULL )
		{
			ULONG ReturnLength = 0;

			Status = nsW32API::NtOsKrnlAPIMgr.pfnZwQueryInformationProcess( ProcessHandle, ( PROCESSINFOCLASS )ProcessImageFileName, 
																			ProcessImageFilePath, ProcessImageFilePathBytes, 
																			&ReturnLength );
			if( !NT_SUCCESS( Status ) )
			{
				ExFreePoolWithTag( ProcessImageFilePath, 'fdew' );
				ProcessImageFilePathBytes = ReturnLength;
			}
		}
		else
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
		}

    } while( Status == STATUS_INFO_LENGTH_MISMATCH );

	if( NT_SUCCESS( Status ) )
		*ProcessName = ProcessImageFilePath;

	return Status;
}

NTSTATUS SearchProcessInfo( ULONG ProcessId, TyGenericBuffer<WCHAR>* ProcessFileFullPath, PWCH* wszProcessName )
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if( ProcessFileFullPath == NULLPTR )
		return Status;

    KeEnterCriticalRegion();
	ExAcquireResourceSharedLite( &nsDetail::ProcInfoLock, TRUE );

    do
    {
		RtlZeroMemory( ProcessFileFullPath, sizeof( TyGenericBuffer<WCHAR> ) );

		if( ProcessId <= 4 )
		{
			Status = STATUS_SUCCESS;
			*ProcessFileFullPath = AllocateBuffer<WCHAR>( BUFFER_PROCNAME );
			wcscpy( ProcessFileFullPath->Buffer, L"\\SYSTEM" );
			break;
		}

		PLIST_ENTRY Current = nsDetail::ProcInfoListHead.Flink;
		while( Current != &nsDetail::ProcInfoListHead )
		{
			PROCESS_INFO* entry = CONTAINING_RECORD( Current, PROCESS_INFO, ListEntry );

			if( entry->ProcessId == ProcessId )
			{
				if( entry->ProcessFileFullPathUni != NULL && entry->ProcessFileFullPathUni->Buffer )
				{
					*ProcessFileFullPath = AllocateBuffer<WCHAR>( BUFFER_PROCNAME, entry->ProcessFileFullPathUni->Length );
					RtlStringCbCopyW( ProcessFileFullPath->Buffer, ProcessFileFullPath->BufferSize, entry->ProcessFileFullPathUni->Buffer );
				}
				else
				{
					*ProcessFileFullPath = AllocateBuffer<WCHAR>( BUFFER_PROCNAME, entry->ProcessFileFullPath.BufferSize );
					RtlStringCbCopyW( ProcessFileFullPath->Buffer, ProcessFileFullPath->BufferSize, entry->ProcessFileFullPath.Buffer );
				}

				if( wszProcessName != NULLPTR )
				{
					WCHAR* wszBuffer = nsUtils::ReverseFindW( ProcessFileFullPath->Buffer, L'\\' );
					if( wszBuffer != NULLPTR )
					{
						wszBuffer++;
						*wszProcessName = wszBuffer;
					}
				}

				break;
			}

			Current = Current->Flink;
		}

		if( ProcessFileFullPath->Buffer == NULLPTR )
		{
			ExReleaseResourceLite( &nsDetail::ProcInfoLock );
			KeLeaveCriticalRegion();

			InsertProcessInfo( 0, ProcessId );
			return SearchProcessInfo( ProcessId, ProcessFileFullPath, wszProcessName );
		}
		else
		{
			Status = STATUS_SUCCESS;
		}

    } while( false );

	ExReleaseResourceLite( &nsDetail::ProcInfoLock );
	KeLeaveCriticalRegion();

	return Status;
}

BOOLEAN GetProcessNameFromEPROCESS( __in ULONG ProcessId, __in PEPROCESS Process, __out WCHAR* wszProcessName, __out ULONG bufferSize )
{
	if( ProcessId == 0 || ProcessId == 4 || ProcessId == 8 )
	{
		RtlStringCbCopyW( wszProcessName, bufferSize, L"\\SYSTEM" );

		return TRUE;
	}

	using namespace nsDetail;

	if( nsUtils::nsDetail::GlobalOSInfo.CpuBits == 32 )
	{
		if( nsUtils::nsDetail::GlobalOSInfo.MajorVersion == 5 )
		{
			if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 0 )
			{
				PEPROCESS_V50_X86   process;
				PPEB_V50_X86         peb;
				PRTL_USER_PROCESS_PARAMETERS_V50_X86 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V50_X86 )Process;

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
			else if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 1 )
			{
				PEPROCESS_V51_X86   process;
				PPEB_V51_X86         peb;
				PRTL_USER_PROCESS_PARAMETERS_V51_X86 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V51_X86 )Process;

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
			else if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 2 )
			{
				PEPROCESS_V52_X86   process;
				PPEB_V52_X86         peb;
				PRTL_USER_PROCESS_PARAMETERS_V52_X86 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V52_X86 )Process;

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
		}
		else if( nsUtils::nsDetail::GlobalOSInfo.MajorVersion == 6 )
		{
			if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 0 )
			{
				PEPROCESS_V60_X86   process;
				PPEB_V60_X86         peb;
				PRTL_USER_PROCESS_PARAMETERS_V60_X86 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V60_X86 )Process;

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
			else if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 1 )
			{
				PEPROCESS_V61_X86   process;
				PPEB_V61_X86         peb;
				PRTL_USER_PROCESS_PARAMETERS_V61_X86 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V61_X86 )Process;

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
			else if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 2 )
			{
				PEPROCESS_V62_X86   process;
				PPEB_V62_X86         peb;
				PRTL_USER_PROCESS_PARAMETERS_V62_X86 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V62_X86 )Process;

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
			else if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 3 )
			{
				PEPROCESS_V63_X86   process;
				PPEB_V63_X86         peb;
				PRTL_USER_PROCESS_PARAMETERS_V63_X86 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V63_X86 )Process;

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
		}
		else if( nsUtils::nsDetail::GlobalOSInfo.MajorVersion == 10 )
		{
			if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 0 )
			{
				PEPROCESS_V100_X86   process;
				PPEB_V100_X86         peb;
				PRTL_USER_PROCESS_PARAMETERS_V100_X86 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V100_X86 )PsGetCurrentProcess();

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
		}
	}
	else if( nsUtils::nsDetail::GlobalOSInfo.CpuBits == 64 )
	{
		if( nsUtils::nsDetail::GlobalOSInfo.MajorVersion == 6 )
		{
			if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 0 )
			{

			}
			else if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 1 )
			{
				PEPROCESS_V61_AMD64    process;
				PPEB_V61_AMD64         peb;
				PRTL_USER_PROCESS_PARAMETERS_V61_AMD64 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V61_AMD64 )Process;

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
			else if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 2 )
			{
				PEPROCESS_V62_AMD64    process;
				PPEB_V62_AMD64         peb;
				PRTL_USER_PROCESS_PARAMETERS_V62_AMD64 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V62_AMD64 )Process;

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
			else if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 3 )
			{
				PEPROCESS_V63_AMD64    process;
				PPEB_V63_AMD64         peb;
				PRTL_USER_PROCESS_PARAMETERS_V63_AMD64 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V63_AMD64 )Process;

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

					RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
		}
		else if( nsUtils::nsDetail::GlobalOSInfo.MajorVersion == 10 )
		{
			if( nsUtils::nsDetail::GlobalOSInfo.MinorVersion == 0 )
			{
				PEPROCESS_V100_AMD64    process;
				PPEB_V100_AMD64         peb;
				PRTL_USER_PROCESS_PARAMETERS_V100_AMD64 ProcessParameters;
				BOOLEAN  bException = FALSE;

				process = ( PEPROCESS_V100_AMD64 )PsGetCurrentProcess();

				if( process == NULL || process->Peb == NULL ) return FALSE;

				__try
				{
					ULONG i;
					peb = process->Peb;
					ProcessParameters = peb->ProcessParameters;

                    RtlCopyMemory( wszProcessName, ProcessParameters->ImagePathName.Buffer, ProcessParameters->ImagePathName.Length );

					for( i = 0; i < ProcessParameters->ImagePathName.Length / sizeof( WCHAR ); i++ )
					{
						wszProcessName[ i ] = RtlUpcaseUnicodeChar( wszProcessName[ i ] );
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					bException = TRUE;
				}

				return !bException;
			}
		}
	}

	return FALSE;
}
