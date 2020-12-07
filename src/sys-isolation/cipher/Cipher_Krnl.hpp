#ifndef HDR_ISOLATION_CIPHER_KERNEL
#define HDR_ISOLATION_CIPHER_KERNEL

#include "fltBase.hpp"
#include "Cipher.hpp"

#include "WinIOIsolation_Event.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS CipherFile( __in USER_FILE_ENCRYPT* opt );
NTSTATUS DecipherFile( __in USER_FILE_ENCRYPT* opt, __out_opt PVOID SolutionMetaData = NULLPTR, __out_opt ULONG* SolutionMetaDataSize = NULLPTR );

///////////////////////////////////////////////////////////////////////////////

NTSTATUS RetrieveStubCode( __in USER_FILE_ENCRYPT* opt, __out PVOID* StubCode, __out ULONG* StubCodeSize );

#endif // HDR_ISOLATION_CIPHER_KERNEL