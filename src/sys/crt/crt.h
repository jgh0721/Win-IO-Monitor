#pragma once

#include <wdm.h>

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

using _PVFV = void( __cdecl* )( void ); // PVFV = Pointer to Void Func(Void)
using _PIFV = int( __cdecl* )( void ); // PIFV = Pointer to Int Func(Void)

extern "C" int __cdecl __crt_init();
extern "C" void __cdecl __crt_deinit();
extern "C" int __cdecl atexit( _PVFV func );

///////////////////////////////////////////////////////////////////////////////
/// Memory

void  __cdecl free( void* ptr );
void* __cdecl malloc( size_t size, POOL_TYPE type = NonPagedPool );
void* __cdecl realloc( void* ptr, size_t new_size, POOL_TYPE type = NonPagedPool );
