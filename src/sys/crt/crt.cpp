#include "crt.h"

#include <wdm.h>
#include <exception>

#define _CRTALLOC(x) __declspec(allocate(x))

// C initializers:
#pragma section(".CRT$XIA", long, read)
_CRTALLOC( ".CRT$XIA" ) _PIFV __xi_a[] = { 0 };
#pragma section(".CRT$XIZ", long, read)
_CRTALLOC( ".CRT$XIZ" ) _PIFV __xi_z[] = { 0 };

// C++ initializers:
#pragma section(".CRT$XCA", long, read)
__declspec( allocate( ".CRT$XCA" ) ) _PVFV __xc_a[] = { 0 };
#pragma section(".CRT$XCZ", long, read)
__declspec( allocate( ".CRT$XCZ" ) ) _PVFV __xc_z[] = { 0 };

// C pre-terminators:
#pragma section(".CRT$XPA", long, read)
__declspec( allocate( ".CRT$XPA" ) ) _PVFV __xp_a[] = { 0 };
#pragma section(".CRT$XPZ", long, read)
__declspec( allocate( ".CRT$XPZ" ) ) _PVFV __xp_z[] = { 0 };

// C terminators:
#pragma section(".CRT$XTA", long, read)
__declspec( allocate( ".CRT$XTA" ) ) _PVFV __xt_a[] = { 0 };
#pragma section(".CRT$XTZ", long, read)
__declspec( allocate( ".CRT$XTZ" ) ) _PVFV __xt_z[] = { 0 };

#pragma data_seg()

#pragma comment(linker, "/merge:.CRT=.rdata")

typedef struct _ON_EXIT_ENTRY
{
    LIST_ENTRY Entry;
    _PVFV func;
}ON_EXIT_ENTRY, * PON_EXIT_ENTRY;

LIST_ENTRY __onexithead;

int __cdecl _purecall()
{
    // It's abnormal execution, so we should detect it:
    __debugbreak();
    return 0;
}

static void execute_pvfv_array( _PVFV* begin, _PVFV* end )
{
    /*
    * walk the table of function pointers from the bottom up, until
    * the end is encountered.  Do not skip the first entry.  The initial
    * value of pfbegin points to the first valid entry.  Do not try to
    * execute what pfend points to.  Only entries before pfend are valid.
    */
    _PVFV* fn = begin;
    while( fn != end )
    {
        if( *fn ) ( **fn )( );
        ++fn;
    }
}

static int execute_pifv_array( _PIFV* begin, _PIFV* end )
{
    _PIFV* fn = begin;
    while( fn != end )
    {
        if( *fn )
        {
            int result = ( **begin )( );
            if( result ) return result;
        }
        ++fn;
    }
    return 0;
}

extern "C" int __cdecl __crt_init()
{
    InitializeListHead( &__onexithead );

    int result = execute_pifv_array( __xi_a, __xi_z );
    if( result ) return result;

    // C++ initialization
    execute_pvfv_array( __xc_a, __xc_z );
    return 0;
}

_PVFV _onexit( _PVFV lpfn )
{
	PON_EXIT_ENTRY _Entry = ( PON_EXIT_ENTRY )malloc( sizeof( ON_EXIT_ENTRY ) );

	if( !_Entry )
		return NULL;

	_Entry->func = lpfn;
	InsertHeadList( &__onexithead, &_Entry->Entry );
	return lpfn;
}

int __cdecl atexit( _PVFV func )
{
	return ( _onexit( func ) == NULL ) ? -1 : 0;
}

void doexit( int code, int quick, int retcaller )
{
	UNREFERENCED_PARAMETER( code );
	UNREFERENCED_PARAMETER( retcaller );

	if( !quick )
	{
		while( !IsListEmpty( &__onexithead ) )
		{
			PLIST_ENTRY _Entry = RemoveHeadList( &__onexithead );
			PON_EXIT_ENTRY Entry = ( PON_EXIT_ENTRY )_Entry;

			Entry->func();
			free( Entry );
		}
	}
}

extern "C" void __cdecl __crt_deinit()
{
    doexit( 0, 0, 1 );    /* full term, return to caller */

    execute_pvfv_array( __xp_a, __xp_z );
    execute_pvfv_array( __xt_a, __xt_z );
}

[[noreturn]]
static void RaiseException( ULONG BugCheckCode )
{
    KdBreakPoint();
    KeBugCheckEx( BugCheckCode, 0, 0, 0, 0 );
}

[[noreturn]]
void __cdecl _invalid_parameter_noinfo_noreturn()
{
    RaiseException( DRIVER_INVALID_CRUNTIME_PARAMETER );
}

namespace std
{
    [[noreturn]]
    void __cdecl _Xbad_alloc()
    {
        RaiseException( INSTALL_MORE_MEMORY );
    }

    [[noreturn]]
    void __cdecl _Xinvalid_argument( __in_z const char* )
    {
        RaiseException( DRIVER_INVALID_CRUNTIME_PARAMETER );
    }

    [[noreturn]]
    void __cdecl _Xlength_error( __in_z const char* )
    {
        RaiseException( KMODE_EXCEPTION_NOT_HANDLED );
    }

    [[noreturn]]
    void __cdecl _Xout_of_range( __in_z const char* )
    {
        RaiseException( DRIVER_OVERRAN_STACK_BUFFER );
    }

    [[noreturn]]
    void __cdecl _Xoverflow_error( __in_z const char* )
    {
        RaiseException( DRIVER_OVERRAN_STACK_BUFFER );
    }

    [[noreturn]]
    void __cdecl _Xruntime_error( __in_z const char* )
    {
        RaiseException( KMODE_EXCEPTION_NOT_HANDLED );
    }

    [[noreturn]]
    void __cdecl RaiseHandler( const std::exception& )
    {
        RaiseException( KMODE_EXCEPTION_NOT_HANDLED );
    }

    using _Prhand = void( __cdecl* )( const exception& );

#if defined(_MSC_VER) || _MSC_VER >= 1900 
    _Prhand _Raise_handler = &RaiseHandler;
#endif

}

[[noreturn]]
void __cdecl _invoke_watson(
    wchar_t const* const expression,
    wchar_t const* const function_name,
    wchar_t const* const file_name,
    unsigned int   const line_number,
    uintptr_t      const reserved )
{
    UNREFERENCED_PARAMETER( expression );
    UNREFERENCED_PARAMETER( function_name );
    UNREFERENCED_PARAMETER( file_name );
    UNREFERENCED_PARAMETER( line_number );
    UNREFERENCED_PARAMETER( reserved );

    KdBreakPoint();
    RaiseException( KMODE_EXCEPTION_NOT_HANDLED );
}

///////////////////////////////////////////////////////////////////////////////
/// Memory

#define KCRT_POOL_DEFAULT_TAG	'trck' // kcrt

typedef struct _MALLOC_HEADER
{
    ULONG32 Tags;
    ULONG32 _Resv0;
    ULONG_PTR Size;
    POOL_TYPE Type;
}MALLOC_HEADER, * PMALLOC_HEADER;

C_ASSERT( sizeof( MALLOC_HEADER ) % sizeof( void* ) == 0 );

PMALLOC_HEADER GET_MALLOC_HEADER( PVOID ptr )
{
    return ( MALLOC_HEADER* )( ( PUCHAR )ptr - sizeof( MALLOC_HEADER ) );
}

PVOID GET_MALLOC_ADDRESS( PMALLOC_HEADER header )
{
    return ( PVOID )( ( PUCHAR )header + sizeof( MALLOC_HEADER ) );
}

ULONG_PTR GET_MALLOC_SIZE( PVOID ptr )
{
    PMALLOC_HEADER header = GET_MALLOC_HEADER( ptr );

    if( header->Tags != KCRT_POOL_DEFAULT_TAG )
        KeBugCheckEx( BAD_POOL_HEADER, 0, 0, 0, 0 );

    return header->Size;
}

void __cdecl free( void* ptr )
{
    if( ptr )
    {
        MALLOC_HEADER* mhdr = GET_MALLOC_HEADER( ptr );

        if( mhdr->Tags != KCRT_POOL_DEFAULT_TAG )
            KeBugCheckEx( BAD_POOL_HEADER, 0, 0, 0, 0 );

        ExFreePool( mhdr );
    }
}

void* __cdecl malloc( size_t size, POOL_TYPE type )
{
    PMALLOC_HEADER mhdr = NULL;
    const size_t new_size = size + sizeof( MALLOC_HEADER );

    mhdr = ( PMALLOC_HEADER )ExAllocatePoolWithTag( type, new_size, KCRT_POOL_DEFAULT_TAG );
    if( mhdr )
    {
        RtlZeroMemory( mhdr, new_size );

        mhdr->Tags = KCRT_POOL_DEFAULT_TAG;
        mhdr->Size = size;
        mhdr->Type = type;
        return GET_MALLOC_ADDRESS( mhdr );
    }

    return NULL;
}

void* __cdecl realloc( void* ptr, size_t new_size, POOL_TYPE type )
{
    if( !ptr )
    {
        return malloc( new_size, type );
    }
    else if( new_size == 0 )
    {
        free( ptr );
        return NULL;
    }
    else
    {
        size_t old_size = GET_MALLOC_SIZE( ptr );

        if( new_size <= old_size )
        {
            return ptr;
        }
        else
        {
            
            void* new_ptr = malloc( new_size, GET_MALLOC_HEADER( ptr )->Type );

            if( new_ptr )
            {
                memcpy( new_ptr, ptr, old_size );
                free( ptr );
                return new_ptr;
            }
        }
    }

    return NULL;
}

constexpr unsigned long CrtPoolTag = 'TRC_';

void* __cdecl operator new( size_t Size )
{
    void* Pointer = ExAllocatePoolWithTag( NonPagedPool, Size, CrtPoolTag );
    if( Pointer ) RtlZeroMemory( Pointer, Size );
    return Pointer;
}

void* __cdecl operator new( size_t Size, POOL_TYPE PoolType )
{
    void* Pointer = ExAllocatePoolWithTag( PoolType, Size, CrtPoolTag );
    if( Pointer ) RtlZeroMemory( Pointer, Size );
    return Pointer;
}

void* __cdecl operator new[]( size_t Size ) {
    void* Pointer = ExAllocatePoolWithTag( NonPagedPool, Size, CrtPoolTag );
    if( Pointer ) RtlZeroMemory( Pointer, Size );
    return Pointer;
}

void* __cdecl operator new[]( size_t Size, POOL_TYPE PoolType ) {
    void* Pointer = ExAllocatePoolWithTag( PoolType, Size, CrtPoolTag );
    if( Pointer ) RtlZeroMemory( Pointer, Size );
    return Pointer;
}

void __cdecl operator delete( void* Pointer )
{
    if( !Pointer ) return;
    ExFreePoolWithTag( Pointer, CrtPoolTag );
}

void __cdecl operator delete( void* Pointer, size_t Size )
{
    UNREFERENCED_PARAMETER( Size );
    if( !Pointer ) return;
    ExFreePoolWithTag( Pointer, CrtPoolTag );
}

void __cdecl operator delete[]( void* Pointer )
{
    if( !Pointer ) return;
    ExFreePoolWithTag( Pointer, CrtPoolTag );
}

void __cdecl operator delete[]( void* Pointer, size_t Size )
{
    UNREFERENCED_PARAMETER( Size );
    if( !Pointer ) return;
    ExFreePoolWithTag( Pointer, CrtPoolTag );
}

//#include <wdm.h>
//#include <ntstrsafe.h>
//#include "crt.h"
//
//#ifndef STATUS_BAD_DATA
//#define STATUS_BAD_DATA ((NTSTATUS)0xC000090BL)
//#endif

////////////////////////////////////////////////////////////////////////////
//// dummy
//char* __cdecl getenv( char const* name )
//{
//	name;
//	return NULL;
//}
//
////////////////////////////////////////////////////////////////////////////
//// assert
//void __cdecl _wassert( wchar_t const* _Message, wchar_t const* _File, unsigned _Line )
//{
//	_Message;
//	_File;
//	_Line;
//
//	KdBreakPoint();
//	ExRaiseStatus( STATUS_BAD_DATA );
//}
//
//#include <sal.h>
//
////////////////////////////////////////////////////////////////////////////
//// ntoskrnl
//NTSYSAPI BOOLEAN NTAPI RtlTimeToSecondsSince1970( __in PLARGE_INTEGER Time, __out PULONG ElapsedSeconds );
//int __cdecl _vsnprintf_s( char* const _Buffer, size_t const _BufferCount, size_t const _MaxCount, char const* const _Format, va_list _ArgList );
//
////////////////////////////////////////////////////////////////////////////
//// time
//
//typedef long clock_t;
//typedef long __time32_t;
//typedef __int64 __time64_t;
//
//clock_t __cdecl clock( void )
//{
//	LARGE_INTEGER li;
//
//	KeQueryTickCount( &li );
//	return li.LowPart;
//}
//
//__time64_t __cdecl _time64( __time64_t* _Time )
//{
//	ULONG uTime;
//	LARGE_INTEGER li;
//	__time64_t uTime64;
//
//	KeQuerySystemTime( &li );
//	RtlTimeToSecondsSince1970( &li, &uTime );
//	uTime64 = uTime;
//
//    if( _Time )
//		*_Time = uTime64;
//
//	return uTime64;
//}
//
//__time32_t __cdecl _time32( __time32_t* _Time )
//{
//	ULONG uTime = 0;
//	LARGE_INTEGER li = {0,0};
//
//	KeQuerySystemTime( &li );
//	RtlTimeToSecondsSince1970( &li, &uTime );
//
//    if( _Time )
//		*_Time = uTime;
//
//	return uTime;
//}
//
//time_t __cdecl time( time_t* _Time )
//{
//#if defined(_WIN64)
//	return _time64( _Time );
//#else
//	return _time32( _Time );
//#endif
//}
//
////////////////////////////////////////////////////////////////////////////
//// sprintf
//
//#include <stdarg.h>
//
//int __cdecl ksprintf( char* const s, size_t const sz, char const* const f, ... )
//{
//	int n = 0;
//	va_list _ArgList;
//	size_t remain = 0;
//
//	va_start( _ArgList, f );
//	RtlStringCbVPrintfExA( s, sz, NULL, &remain, STRSAFE_IGNORE_NULLS, f, _ArgList );
//	va_end( _ArgList );
//	n = sz - remain;
//	return n;
//}