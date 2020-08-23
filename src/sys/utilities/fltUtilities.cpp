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

    WCHAR* UpperWString( WCHAR* wszString )
    {
		ASSERT( wszString != nullptr );
		if( wszString == nullptr )
			return nullptr;

		size_t length = strlength( wszString );

		for( size_t i = 0; i < length; i++ )
			wszString[ i ] = RtlUpcaseUnicodeChar( wszString[ i ] );

		return wszString;
    }

	bool WildcardMatch_straight( const char* pszString, const char* pszMatch, bool isCaseSensitive /* = false */ )
	{
		const char* mp = NULL;
		const char* cp = NULL;

		ASSERT( pszString != nullptr && pszMatch != nullptr );
		if( pszString == nullptr || pszMatch == nullptr )
			return false;

		while( *pszString )
		{
			if( *pszMatch == char( '*' ) )
			{
				if( !*++pszMatch )
					return true;
				mp = pszMatch;
				cp = pszString + 1;
			}
			else if( ( isCaseSensitive == true && *pszMatch == char( '?' ) ) ||
					 ( isCaseSensitive == false && ( ( *pszMatch == char( '?' ) ) || ( RtlUpcaseUnicodeChar( *pszMatch ) == RtlUpcaseUnicodeChar( *pszString ) ) ) )
					 )
			{
				pszMatch++;
				pszString++;
			}
			else if( !cp )
				return false;
			else
			{
				pszMatch = mp;
				pszString = cp++;
			}
		}

		while( *pszMatch == char( '*' ) )
			pszMatch++;

		return !*pszMatch;
	}

	bool WildcardMatch_straight( const wchar_t* pszString, const wchar_t* pszMatch, bool isCaseSensitive /* = false */ )
	{
		const wchar_t* mp = NULL;
		const wchar_t* cp = NULL;

		ASSERT( pszString != nullptr && pszMatch != nullptr );
		if( pszString == nullptr || pszMatch == nullptr )
			return false;

		while( *pszString )
		{
			if( *pszMatch == wchar_t( '*' ) )
			{
				if( !*++pszMatch )
					return true;
				mp = pszMatch;
				cp = pszString + 1;
			}
			else if( ( isCaseSensitive == true && *pszMatch == wchar_t( '?' ) ) ||
					 ( isCaseSensitive == false && ( ( *pszMatch == wchar_t( '?' ) ) || ( RtlUpcaseUnicodeChar( *pszMatch ) == RtlUpcaseUnicodeChar( *pszString ) ) ) )
					 )
			{
				pszMatch++;
				pszString++;
			}
			else if( !cp )
				return false;
			else
			{
				pszMatch = mp;
				pszString = cp++;
			}
		}

		while( *pszMatch == wchar_t( '*' ) )
			pszMatch++;

		return !*pszMatch;
	}

} // nsUtils
