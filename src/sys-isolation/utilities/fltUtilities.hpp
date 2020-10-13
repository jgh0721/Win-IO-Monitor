#ifndef HDR_DRIVER_UTILITIES
#define HDR_DRIVER_UTILITIES

#include "fltBase.hpp"

#include "irpContext_Defs.hpp"
#include "privateFCBMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FAST_IO_POSSIBLE CheckIsFastIOPossible( __in FCB* Fcb );
PVOID FltMapUserBuffer( __in PFLT_CALLBACK_DATA Data );

/**
 * @brief FltCreateFileEx 를 이용하여 주어진 파일 또는 디렉토리를 열고 결과를 반환한다 
 * @param IrpContext 
 * @param FileFullPath
    1. 디바이스 이름 또는 드라이브 문자가 포함되지 않은 형태
    2. 디바이스 이름으로 시작되는 완전 경로
    3. 드라이브 문자로 시작되는 완전 경로 
 * @param FileHandle 
 * @param FileObject 
 * @param IoStatus 
 * @param DesiredAccess 
 * @param FileAttributes 
 * @param ShareAccess 
 * @param CreateDisposition 
 * @param CreateOptions 
 * @param Flags 
 * @return 
*/
NTSTATUS FltCreateFileOwn( __in IRP_CONTEXT* IrpContext, __in_z const WCHAR* FileFullPath,
                           __out HANDLE* FileHandle, __out FILE_OBJECT** FileObject,
                           __out IO_STATUS_BLOCK* IoStatus,
                           __in ACCESS_MASK DesiredAccess = FILE_SPECIAL_ACCESS,
                           __in ULONG FileAttributes = FILE_ATTRIBUTE_NORMAL,
                           __in ULONG ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           __in ULONG CreateDisposition = FILE_OPEN,
                           __in ULONG CreateOptions = FILE_NON_DIRECTORY_FILE,
                           __in ULONG Flags = IO_IGNORE_SHARE_ACCESS_CHECK );

#endif // HDR_DRIVER_UTILITIES
