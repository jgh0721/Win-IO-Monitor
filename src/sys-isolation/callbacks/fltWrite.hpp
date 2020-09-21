#ifndef HDR_ISOLATION_WRITE
#define HDR_ISOLATION_WRITE

#include "fltBase.hpp"
#include "irpContext_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreWrite( __inout PFLT_CALLBACK_DATA Data,
                __in PCFLT_RELATED_OBJECTS FltObjects,
                __deref_out_opt PVOID* CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostWrite( __inout PFLT_CALLBACK_DATA    Data,
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
 *  만약 기록하려는 데이터가 ValidDataLength 를 넘어서면 기록 전에 파일의 끝(EndOfFile) 을 재설정해야한다
 *  만약 기록하려는 데이터가 FileSize 를 넘어서면 기록 전에 파일의 끝을 재설정해야한다.
        또한, 캐시관리자를 호출하여 ValidDataLength 와 새로 설정된 파일의 끝 사이를 0 으로 채우고, 기록이 완료되면 캐시관리자에게 변경된 파일 크기를 알려야한다. 
 * Cached I/O
 *  User Buffer <-> System Cache
 */

NTSTATUS WritePagingIO( __in IRP_CONTEXT* IrpContext, __out PVOID WriteBuffer );
NTSTATUS WriteCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID WriteBuffer );
NTSTATUS WriteNonCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID WriteBuffer );


NTSTATUS WriteNonCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID WriteBuffer, __in ULONG BytesToCopy, __in ULONG BytesToWrite );

NTSTATUS SetEndOfFile( __in IRP_CONTEXT* IrpContext, __in LONGLONG llEndOfFile );
NTSTATUS SafeCcZeroData( __in IRP_CONTEXT* IrpContext, __in LONGLONG llStartOffset, __in LONGLONG llEndOffset );

#endif // HDR_ISOLATION_WRITE
