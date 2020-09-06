#include "stdafx.h"
#include "cmnString.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsCmn
{
    namespace nsDetail
    {
        /* 유용한 std::string 에 대한 formatting 함수 */
        FORCEINLINE std::string format_arg_list( const char* fmt, va_list args )
        {
            if( !fmt ) return "";
            int   result = -1, length = 512;
            char* buffer = 0;
            while( result == -1 )
            {
                if( buffer )
                    delete[] buffer;
                buffer = new char[ length + 1 ];
                memset( buffer, 0, ( length + 1 ) * sizeof( char ) );
                result = _vsnprintf_s( buffer, length, _TRUNCATE, fmt, args );
                length *= 2;
            }
            std::string s( buffer );
            delete[] buffer;
            return s;
        }

        FORCEINLINE std::wstring format_arg_list( const wchar_t* fmt, va_list args )
        {
            if( !fmt ) return L"";
            int   result = -1, length = 512;
            wchar_t* buffer = 0;
            while( result == -1 )
            {
                if( buffer )
                    delete[] buffer;
                buffer = new wchar_t[ length + 1 ];
                memset( buffer, 0, ( length + 1 ) * sizeof( wchar_t ) );
                result = _vsnwprintf_s( buffer, length, _TRUNCATE, fmt, args );
                length *= 2;
            }
            std::wstring s( buffer );
            delete[] buffer;
            return s;
        }
    } // nsDetail

    std::string format( const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );
        std::string s = nsDetail::format_arg_list( fmt, args );
        va_end( args );
        return s;
    }

    std::wstring format( const wchar_t* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );
        std::wstring s = nsDetail::format_arg_list( fmt, args );
        va_end( args );
        return s;
    }

    int chrcmpi( char w1, char wMatch )
    {
        char sz1[ 2 ], sz2[ 2 ];

        sz1[ 0 ] = w1;
        sz1[ 1 ] = '\0';
        sz2[ 0 ] = wMatch;
        sz2[ 1 ] = '\0';

        return _stricmp( sz1, sz2 );
    }

    int chrcmpiw( wchar_t w1, wchar_t wMatch )
    {
        wchar_t sz1[ 2 ], sz2[ 2 ];

        sz1[ 0 ] = w1;
        sz1[ 1 ] = '\0';
        sz2[ 0 ] = wMatch;
        sz2[ 1 ] = '\0';

        return _wcsicmp( sz1, sz2 );
    }

    int strcmplocaleA( unsigned int dwFlags, const char* pwsz1, int cch1, const char* pwsz2, int cch2 )
    {
#ifdef _WINDOWS_
        int i = CompareStringA( GetThreadLocale(), dwFlags, pwsz1, cch1, pwsz2, cch2 );
        if( !i )
        {
            i = CompareStringA( LOCALE_SYSTEM_DEFAULT, dwFlags, pwsz1, cch1, pwsz2, cch2 );
        }
        return i - CSTR_EQUAL;
#else
        return stricmp( pwsz1, pwsz2 );
#endif
    }

    int strcmplocaleW( unsigned int dwFlags, const wchar_t* pwsz1, int cch1, const wchar_t* pwsz2, int cch2 )
    {
#ifdef _WINDOWS_
        int i = CompareStringW( GetThreadLocale(), dwFlags, pwsz1, cch1, pwsz2, cch2 );
        if( !i )
        {
            i = CompareStringW( LOCALE_SYSTEM_DEFAULT, dwFlags, pwsz1, cch1, pwsz2, cch2 );
        }
        return i - CSTR_EQUAL;
#else
        return stricmp( pwsz1, pwsz2 );
#endif
    }

    char* strchri( const char* lpStart, char wMatch )
    {
        if( lpStart )
        {
            for( ; *lpStart; lpStart++ )
            {
                if( !chrcmpi( *lpStart, wMatch ) )
                    return( ( char* )lpStart );
            }
        }
        return ( NULL );
    }

    wchar_t* strchriw( const wchar_t* lpStart, wchar_t wMatch )
    {
        //         RIPMSG( lpStart && IS_VALID_STRING_PTRW( lpStart, -1 ), "StrChrIW: caller passed bad lpStart" );
        if( lpStart )
        {
            for( ; *lpStart; lpStart++ )
            {
                if( !chrcmpiw( *lpStart, wMatch ) )
                    return( ( wchar_t* )lpStart );
            }
        }
        return ( NULL );
    }

    char* strstri( const char* lpFirst, const char* lpSrch )
    {
        if( lpFirst && lpSrch )
        {
            unsigned int uLen = ( unsigned int )strlen( lpSrch );
            char wMatch = *lpSrch;

            for( ; ( lpFirst = strchri( lpFirst, wMatch ) ) != 0 && strcmpni( lpFirst, lpSrch, uLen );
                 lpFirst++ )
                continue; /* continue until we hit the end of the string or get a match */

            return ( char* )lpFirst;
        }
        return NULL;
    }

    wchar_t* strstriw( const wchar_t* lpFirst, const wchar_t* lpSrch )
    {
        //         RIPMSG( lpFirst && IS_VALID_STRING_PTRW( lpFirst, -1 ), "StrStrIW: Caller passed bad lpFirst" );
        //         RIPMSG( lpSrch && IS_VALID_STRING_PTRW( lpSrch, -1 ), "StrStrIW: Caller passed bad lpSrch" );
        if( lpFirst && lpSrch )
        {
            unsigned int uLen = ( unsigned int )wcslen( lpSrch );
            wchar_t wMatch = *lpSrch;

            for( ; ( lpFirst = strchriw( lpFirst, wMatch ) ) != 0 && strcmpniw( lpFirst, lpSrch, uLen );
                 lpFirst++ )
                continue; /* continue until we hit the end of the string or get a match */

            return ( wchar_t* )lpFirst;
        }
        return NULL;
    }

    int strcmpni( const char* lpStr1, const char* lpStr2, int nChar )
    {
        return strcmplocaleA( 0x00000001 | 0x10000000, lpStr1, nChar, lpStr2, nChar );
    }

    int strcmpniw( const wchar_t* lpStr1, const wchar_t* lpStr2, int nChar )
    {
        // NORM_IGNORECASE = 0x00000001
        // NORM_STOP_ON_NULL = 0x10000000;
        return strcmplocaleW( 0x00000001 | 0x10000000, lpStr1, nChar, lpStr2, nChar );
    }

    bool strcontains( const char* lhs, const char* rhs, bool isCaseSensitive /*= false */ )
    {
        if( isCaseSensitive == true )
            return strstr( lhs, rhs ) != NULLPTR;

        return nsCmn::strstri( lhs, rhs ) != NULLPTR;
    }

    bool strcontains( const wchar_t* lhs, const wchar_t* rhs, bool isCaseSensitive /*= false */ )
    {
        if( isCaseSensitive == true )
            return wcsstr( lhs, rhs ) != NULLPTR;

        return nsCmn::strstriw( lhs, rhs ) != NULLPTR;
    }

    bool strcontains( const std::string& lhs, const std::string& rhs, bool isCaseSensitive /*= false */ )
    {
        if( isCaseSensitive == true )
            return lhs.find( rhs ) != std::string::npos;

        return nsCmn::strstri( lhs.c_str(), rhs.c_str() ) != NULLPTR;
    }

    bool strcontains( const std::wstring& lhs, const std::wstring& rhs, bool isCaseSensitive /*= false */ )
    {
        if( isCaseSensitive == true )
            return lhs.find( rhs ) != std::wstring::npos;

        return nsCmn::strstriw( lhs.c_str(), rhs.c_str() ) != NULLPTR;
    }

} // nsCmn
