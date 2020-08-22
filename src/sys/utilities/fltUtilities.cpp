#include "fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsUtils
{
	size_t strlength( const wchar_t* str )
	{
		size_t ullLength = 0;
		NTSTATUS Status = RtlStringCchLengthW( str, NTSTRSAFE_MAX_CCH, &ullLength );
		if( !NT_SUCCESS( Status ) )
			return 0;

		return ullLength;
	}

	size_t strlength( const char* str )
	{
		size_t ullLength = 0;
		NTSTATUS Status = RtlStringCchLengthA( str, NTSTRSAFE_MAX_CCH, &ullLength );
		if( !NT_SUCCESS( Status ) )
			return 0;

		return ullLength;
	}

	WCHAR* ReverseFindW( __in_z WCHAR* wszString, WCHAR ch )
	{
		size_t length = strlength( wszString );

		for( size_t i = length; i >= 0; i-- )
		{
			if( wszString[ i ] == ch )
			{
				return &wszString[ i ];
			}
		}

		return NULL;
	}

	WCHAR* ForwardFindW( __in_z WCHAR* wszString, WCHAR ch )
	{
		size_t length = strlength( wszString );

		for( size_t i = 0; i < length; i++ )
		{
			if( wszString[ i ] == ch )
			{
				return &wszString[ i ];
			}
		}

		return NULL;
	}

} // nsUtils
