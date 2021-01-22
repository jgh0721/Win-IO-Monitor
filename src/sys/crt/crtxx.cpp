#include "crt.h"
#include <wdm.h>

///////////////////////////////////////////////////////////////////////////////
/// Main

NTSTATUS Main( DRIVER_OBJECT* DriverObject, UNICODE_STRING* RegistryPath );

//////////////////////////////////////////////////////////////////////////
// Startup
PDRIVER_UNLOAD __user_unload;

extern "C" VOID _crt_unload( PDRIVER_OBJECT DrvObject )
{
	if( __user_unload )
	{
		__user_unload( DrvObject );
	}

	__crt_deinit();
}

extern "C" NTSTATUS _crt_load( PDRIVER_OBJECT DrvObject, PUNICODE_STRING RegPath )
{
	NTSTATUS st;

	if( __crt_init() != 0 )
		return STATUS_APP_INIT_FAILURE;

	__user_unload = NULL;
	st = Main( DrvObject, RegPath );
	if( NT_SUCCESS( st ) )
	{
		PDRIVER_UNLOAD du = DrvObject->DriverUnload;

		if( du )
		{
			__user_unload = du;
			DrvObject->DriverUnload = _crt_unload;
		}
	}
	else
	{
		__crt_deinit();
	}

	return st;
}

extern "C" NTSTATUS DriverEntry( PDRIVER_OBJECT DrvObject, PUNICODE_STRING RegPath )
{
	return _crt_load( DrvObject, RegPath );
}

////////////////////////////////////////////////////////////////////////////
//// new & delete

//// EASTL
//void* operator new[]( size_t size, const char*, int, unsigned, const char*, int ) {
//	return malloc( size );
//}
//void* operator new[]( size_t size, size_t, size_t, const char*, int, unsigned, const char*, int ) {
//	return malloc( size );
//}
//
//
//// For <unordered_set> and <unordered_map> support:
//#ifdef _AMD64_
//#pragma function(ceilf)
//_Check_return_ float __cdecl ceilf( _In_ float _X )
//{
//	int v = static_cast< int >( _X );
//	return static_cast< float >( _X > static_cast< float >( v ) ? v + 1 : v );
//}
//#else
//#pragma function(ceil)
//_Check_return_ double __cdecl ceil( _In_ double _X )
//{
//	int v = static_cast< int >( _X );
//	return static_cast< double >( _X > static_cast< double >( v ) ? v + 1 : v );
//}
//#endif
