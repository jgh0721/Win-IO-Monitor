#ifndef HDR_ISOLATION_READ
#define HDR_ISOLATION_READ

#include "fltBase.hpp"
#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreRead( __inout PFLT_CALLBACK_DATA Data,
               __in PCFLT_RELATED_OBJECTS FltObjects,
               __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostRead( __inout PFLT_CALLBACK_DATA    Data,
                __in PCFLT_RELATED_OBJECTS    FltObjects,
                __in_opt PVOID                CompletionContext,
                __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END

///////////////////////////////////////////////////////////////////////////////

/*!
 * Paging I/O
 *  System Cache <-> Device
 *  현재 파일포인터를 변경하지 않는다
 * Non Cached I/O( Direct I/O )
 *  User Buffer <-> Device
 * Cached I/O
 *  User Buffer <-> System Cache
 *
 *  ValidDataLength <= FileSize <= AllocationSize
 *
 *  파일을 읽을 때, 위의 세 가지 파일 크기에 유의하여 ByteOffset 과 Length 의 현재 위치를 고려해야한다
 *  또한, 읽기 동작을 수행하기 전에 Byte-Range Lock( FileLock) 과 Oplock 을 점검해야한다.
 *  또한, 현재 ValidDataLength 를 넘어선 데이터를 읽으려 시도하면 읽기는 성공하지만 ValidDataLength 를 넘어선 부분의 데이터는 '\0' 으로 채워진다 
 */

NTSTATUS ReadPagingIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer );
NTSTATUS ReadCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer );
NTSTATUS ReadNonCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer );

NTSTATUS ReadPagingIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer, __in ULONG BytesToCopy, __in ULONG BytesToRead, __in ULONG BytesToZero );
NTSTATUS ReadCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer, __in ULONG BytesToCopy, __in ULONG BytesToRead, __in ULONG BytesToZero );
NTSTATUS ReadNonCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer, __in ULONG BytesToCopy, __in ULONG BytesToRead, __in ULONG BytesToZero );

#endif // HDR_ISOLATION_READ
