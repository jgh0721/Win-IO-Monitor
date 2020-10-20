#include "fltCmnLibs_string.hpp"

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

    int stricmp( const wchar_t* lhs, const wchar_t* rhs )
    {
		return stricmp( lhs, strlength( lhs ), rhs );
    }

    int stricmp( const wchar_t* lhs, ULONG cchCount, const wchar_t* rhs )
    {
		auto cchRhsCount = strlength( rhs );

		// based on wineHQ, https://www.winehq.org/pipermail/wine-cvs/2016-May/113431.html
		LONG ret = 0;
		SIZE_T len = min( cchCount, cchRhsCount );
		while( !ret && len-- )
			ret = RtlUpcaseUnicodeChar( *lhs++ ) - RtlUpcaseUnicodeChar( *rhs++ );

		if( !ret )
			ret = cchCount - cchRhsCount;

		return ret;
    }

    WCHAR* ReverseFindW( __in_z WCHAR* wszString, WCHAR ch, int iCchLength /* = -1 */ )
	{
		size_t length = iCchLength == -1 ? strlength( wszString ) : iCchLength;

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

    WCHAR* EndsWithW( WCHAR* wszString, const WCHAR* wszPattern, int iCchLength /* = -1 */ )
    {
		auto lhs = iCchLength == -1 ? strlength( wszString ) : iCchLength;
		auto rhs = strlength( wszPattern );

		if( lhs < rhs )
			return NULLPTR;

		auto base = lhs - rhs;
		for( auto idx = base; idx < lhs; ++idx )
		{
			if( RtlUpcaseUnicodeChar( wszString[ idx ] ) != RtlUpcaseUnicodeChar( wszPattern[ idx - base ] ) )
			{
				return NULLPTR;
			}
		}

		return &wszString[ base ];
    }

    WCHAR* StartsWithW( const WCHAR* wszString, const WCHAR* wszPattern )
    {
		auto lhs = strlength( wszString );
		auto rhs = strlength( wszPattern );

		if( lhs < rhs )
			return NULLPTR;

		for( size_t idx = 0; idx < rhs; ++idx )
		{
			if( RtlUpcaseUnicodeChar( wszString[ idx ] ) != RtlUpcaseUnicodeChar( wszPattern[ idx ] ) )
			{
				return NULLPTR;
			}
		}

		return &((const_cast< WCHAR* >( wszString ))[ 0 ]);
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