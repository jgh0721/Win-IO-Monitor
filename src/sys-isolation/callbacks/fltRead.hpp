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
 *  ���� ���������͸� �������� �ʴ´�
 * Non Cached I/O( Direct I/O )
 *  User Buffer <-> Device
 * Cached I/O
 *  User Buffer <-> System Cache
 *
 *  ValidDataLength <= FileSize <= AllocationSize
 *
 *  ������ ���� ��, ���� �� ���� ���� ũ�⿡ �����Ͽ� ByteOffset �� Length �� ���� ��ġ�� ����ؾ��Ѵ�
 *  ����, �б� ������ �����ϱ� ���� Byte-Range Lock( FileLock) �� Oplock �� �����ؾ��Ѵ�.
 *  ����, ���� ValidDataLength �� �Ѿ �����͸� ������ �õ��ϸ� �б�� ���������� ValidDataLength �� �Ѿ �κ��� �����ʹ� '\0' ���� ä������ 
 */

NTSTATUS ReadPagingIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer );
NTSTATUS ReadCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer );
NTSTATUS ReadNonCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer );

NTSTATUS ReadPagingIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer, __in ULONG BytesToCopy, __in ULONG BytesToRead, __in ULONG BytesToZero );
NTSTATUS ReadCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer, __in ULONG BytesToCopy, __in ULONG BytesToRead, __in ULONG BytesToZero );
NTSTATUS ReadNonCachedIO( __in IRP_CONTEXT* IrpContext, __out PVOID ReadBuffer, __in ULONG BytesToCopy, __in ULONG BytesToRead, __in ULONG BytesToZero );

#endif // HDR_ISOLATION_READ
