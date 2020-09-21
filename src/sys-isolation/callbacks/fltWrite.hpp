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
 *  ���� ����Ϸ��� �����Ͱ� ValidDataLength �� �Ѿ�� ��� ���� ������ ��(EndOfFile) �� �缳���ؾ��Ѵ�
 *  ���� ����Ϸ��� �����Ͱ� FileSize �� �Ѿ�� ��� ���� ������ ���� �缳���ؾ��Ѵ�.
        ����, ĳ�ð����ڸ� ȣ���Ͽ� ValidDataLength �� ���� ������ ������ �� ���̸� 0 ���� ä���, ����� �Ϸ�Ǹ� ĳ�ð����ڿ��� ����� ���� ũ�⸦ �˷����Ѵ�. 
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
