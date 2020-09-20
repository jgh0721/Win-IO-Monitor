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
 * Non Cached I/O( Direct I/O )
 *  User Buffer <-> Device
 * Cached I/O
 *  User Buffer <-> System Cache
 *
 *  ValidDataLength <= FileSize <= AllocationSize
 *
 *  파일을 읽을 때, 위의 세 가지 파일 크기에 유의하여 ByteOffset 과 Length 의 현재 위치를 고려해야한다
 *  또한, 읽기 동작을 수행하기 전에 Byte-Range Lock( FileLock) 과 Oplock 을 점검해야한다. 
 */

NTSTATUS ReadPagingIO( __in IRP_CONTEXT* IrpContext );
NTSTATUS ReadCachedIO( __in IRP_CONTEXT* IrpContext );
NTSTATUS ReadNonCachedIO( __in IRP_CONTEXT* IrpContext );

#endif // HDR_ISOLATION_READ
