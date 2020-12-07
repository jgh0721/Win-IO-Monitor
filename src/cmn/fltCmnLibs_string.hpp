#ifndef HDR_FLT_CMNLIBS_STRING
#define HDR_FLT_CMNLIBS_STRING

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsUtils
{
    /**
     * @brief 문자열 길이를 반환하는 함수
     * @param str
     * @return 반환하는 길이에는 NULL 문자를 포함하지 않음

        IRQL = PASSIVE_LEVEL
    */
    size_t strlength( __in_z const wchar_t* str );
    size_t strlength( __in_z const char* str );

    int stricmp( __in_z const wchar_t* lhs, __in_z const wchar_t* rhs );
    int stricmp( __in_z const wchar_t* lhs, __in ULONG cchCount, __in_z const wchar_t* rhs );

    // from Wine
    int isctype( __in wint_t wc, wctype_t wctypeFlags );
    // from Wine
    unsigned long stoul( __in_z const wchar_t* str, wchar_t** endptr, int base );
    unsigned long stoul( __in_z const char* str, char** endptr, int base );

    WCHAR* ReverseFindW( __in_z WCHAR* wszString, WCHAR ch, __in_opt int iCchLength = -1 );
    size_t ReverseFindPos( __in_z const WCHAR* wszString, WCHAR ch, __in_opt int iCchLength = -1 );
    WCHAR* ForwardFindW( __in_z WCHAR* wszString, WCHAR ch );
    size_t ForwardFindPos( __in_z const WCHAR* wszString, WCHAR ch, __in_opt int iCchLength = -1 );
    WCHAR* UpperWString( __inout_z WCHAR* wszString );

    /**
     * @brief Find wszPattern in wszString from end position to begin
     * @param wszString 
     * @param wszPattern 
     * @return if found begin pattern begin pointer in wszString or NULLPTR

     *  IRQL = PAASIVE_LEVEL
    */
    WCHAR* EndsWithW( __in_z WCHAR* wszString, __in_z const WCHAR* wszPattern, __in_opt int iCchLength = -1 );
    WCHAR* StartsWithW( __in_z const WCHAR* wszString, __in_z const WCHAR* wszPattern );

    /**
     * @brief 지정한 문자열에서 와일드카드(*,?) 를 이용하여 일치하는지 검사
     * @param pszString
     * @param pszMatch
     * @param isCaseSensitive
     * @return

        IRQL <= APC_LEVEL
        http://www.codeproject.com/Articles/188256/A-Simple-Wildcard-Matching-Function
    */
    bool                                    WildcardMatch_straight( const char* pszString, const char* pszMatch, bool isCaseSensitive = false );
    bool                                    WildcardMatch_straight( const wchar_t* pszString, const wchar_t* pszMatch, bool isCaseSensitive = false );

    /*!
        내부적으로 메모리를 할당하여 문자열을 서식화하고 할당한 메모리릅 반환한다

        NonPagedPool 을 사용한다, 호출자는 반드시 ExFreePool 을 호출하여 메모리를 해제해야한다
    */
    char*                                   format( const char* fmt, ... );
    wchar_t*                                format( const wchar_t* fmt, ... );

    /*!
        // File:        match.cpp
        // Author:      Robert A. van Engelen, engelen@genivia.com
        // Date:        August 5, 2019
        // License:     The Code Project Open License (CPOL)
        //              https://www.codeproject.com/info/cpol10.aspx

        변경사항 : 경로구분자 ('/' 와 '\' ) 를 모두 적용되도록 변경
        @param isDotGlob            => set to true to enable dotglob: *. ?, and [] match a . (dotfile) at the begin or after each path separator
        @param isCaseSensitive      => case sensitive compare

        예). a                      => 일  치  a, x/a, x/y/a
                                       불일치  b, x/b, a/a/b
             *                      =>         a, b, x/a, x/y/b
             /*                     =>         a, b
                                               x/a, x/b, x/y/a
            /a                      =>         a
                                               x/a, x/y/a
            a/*\/b                  =>         a/x/b, a/y/b
                                               a/b, a/x/y/b
            a/**\/b                 =>         a/b, a/x/b, a/x/y/b
                                               x/a/b, a/b/x
            a/**                    =>         a/x, a/y, a/x/y
                                               a, b/x
            *.h                     =>         foo.h, bar/foo.h
            /*.h                    =>         foo.h
                                               bar/foo.h
            bar/*.h                 =>         bar/foo.h
                                               foo.h, bar/bar/foo.h

        ※ 예제에는 C++ 주석의 끝맺음 문자와 겹치는 관계로 \ 문자가 추가되어있음.
    */
    template< typename CHAR = wchar_t >
    bool                                    globMatchLikeGitIgnore( const CHAR* text, const CHAR* glob, bool isDotGlob = true, bool isCaseSensitive = false, int textLength = -1, int globLength = -1 )
    {
        static const size_t npos = -1;

        size_t i = 0;
        size_t j = 0;
        size_t n = textLength >= 0 ? textLength : strlength( text );
        size_t m = globLength >= 0 ? globLength : strlength( glob );

        size_t text1_backup = npos;
        size_t glob1_backup = npos;
        size_t text2_backup = npos;
        size_t glob2_backup = npos;
        bool nodot = !isDotGlob;

#ifndef CASE
#define CASE(c) (isCaseSensitive ? (c) : RtlUpcaseUnicodeChar(c))
#endif

        // match pathname if glob contains a / otherwise match the basename
        if( j + 1 < m && glob[ j ] == CHAR( '/' ) )
        {
            // if pathname starts with ./ then ignore these pairs
            while( i + 1 < n && text[ i ] == CHAR( '.' ) && ( ( text[ i + 1 ] == CHAR( '/' ) ) || ( text[ i + 1 ] == CHAR( '\\' ) ) ) )
                i += 2;
            j++;
        }
        else if( ForwardFindPos( glob, CHAR('/' ) ) == npos )
        {
            size_t sep = ReverseFindPos( text, CHAR( '/' ) );
            if( sep == npos )
                sep = ReverseFindPos( text, CHAR( '\\' ) );

            if( sep != npos )
                i = sep + 1;
        }
        while( i < n )
        {
            if( j < m )
            {
                switch( glob[ j ] )
                {
                    case CHAR( '*' ):
                        // match anything except . after /
                        if( nodot && text[ i ] == CHAR( '.' ) )
                            break;
                        if( ++j < m && glob[ j ] == CHAR( '*' ) )
                        {
                            // trailing ** match everything after /
                            if( ++j >= m )
                                return true;
                            // ** followed by a / match zero or more directories
                            if( glob[ j ] != CHAR( '/' ) )
                                return false;
                            // new **-loop, discard *-loop
                            text1_backup = npos;
                            glob1_backup = npos;
                            text2_backup = i;
                            glob2_backup = ++j;
                            continue;
                        }
                        // trailing * matches everything except /
                        text1_backup = i;
                        glob1_backup = j;
                        continue;
                    case CHAR( '?' ):
                        // match anything except . after /
                        if( nodot && text[ i ] == CHAR( '.' ) )
                            break;
                        // match any character except /
                        if( ( text[ i ] == CHAR( '/' ) ) || ( text[ i ] == CHAR( '\\' ) ) )
                            break;
                        i++;
                        j++;
                        continue;
                    case CHAR( '[' ):
                    {
                        // match anything except . after /
                        if( nodot && text[ i ] == CHAR( '.' ) )
                            break;
                        // match any character in [...] except /
                        if( text[ i ] == CHAR( '/' ) || text[ i ] == CHAR( '\\' ) )
                            break;
                        int lastchr;
                        bool matched = false;
                        bool reverse = j + 1 < m && ( glob[ j + 1 ] == CHAR( '^' ) || glob[ j + 1 ] == CHAR( '!' ) );
                        // inverted character class
                        if( reverse )
                            j++;
                        // match character class
                        for( lastchr = 256; ++j < m && glob[ j ] != CHAR( ']' ); lastchr = CASE( glob[ j ] ) )
                            if( lastchr < 256 && glob[ j ] == CHAR( '-' ) && j + 1 < m ? CASE( text[ i ] ) <= CASE( glob[ ++j ] ) && CASE( text[ i ] ) >= lastchr : CASE( text[ i ] ) == CASE( glob[ j ] ) )
                                matched = true;
                        if( matched == reverse )
                            break;
                        i++;
                        if( j < m )
                            j++;
                        continue;
                    }
                    case CHAR( '\\' ):
                        // literal match \-escaped character
                        if( j + 1 < m )
                            j++;
                        // FALLTHROUGH
                    default:
                        // match the current non-NUL character
                        if( CASE( glob[ j ] ) != CASE( text[ i ] ) && !( ( glob[ j ] == CHAR( '/' ) ) && ( ( text[ i ] == CHAR( '/' ) ) || ( text[ i ] == CHAR( '\\' ) ) ) ) )
                            break;
                        // do not match a . with *, ? [] after /
                        nodot = !isDotGlob && glob[ j ] == CHAR( '/' );
                        i++;
                        j++;
                        continue;
                }
            }
            if( glob1_backup != npos && ( ( text[ text1_backup ] != CHAR( '/' ) ) && ( text[ text1_backup ] != CHAR( '\\' ) ) ) )
            {
                // *-loop: backtrack to the last * but do not jump over /
                i = ++text1_backup;
                j = glob1_backup;
                continue;
            }
            if( glob2_backup != npos )
            {
                // **-loop: backtrack to the last **
                i = ++text2_backup;
                j = glob2_backup;
                continue;
            }
            return false;
        }
#ifdef CASE
#undef CASE
#endif
        // ignore trailing stars
        while( j < m && glob[ j ] == CHAR( '*' ) )
            j++;
        // at end of text means success if nothing else is left to match
        return j >= m;
    }

} // nsUtils

#endif // HDR_FLT_CMNLIBS_STRING