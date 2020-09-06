#ifndef HDR_INTERNAL_STRING
#define HDR_INTERNAL_STRING

#include "cmnBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsCmn
{
    namespace nsDetail
    {
        std::string                             format_arg_list( const char* fmt, va_list args );
        std::wstring                            format_arg_list( const wchar_t* fmt, va_list args );
    } // nsDetail

    std::string                             format( const char* fmt, ... );
    std::wstring                            format( const wchar_t* fmt, ... );

    // 0 = same, <> 0 = not same
    int                                     chrcmpi( char w1, char wMatch );
    int                                     chrcmpiw( wchar_t w1, wchar_t wMatch );

    // 0 = same, < 0, > 0 , C runtime 과 같음
    int                                     strcmplocaleA( unsigned int dwFlags, const char* pwsz1, int cch1, const char* pwsz2, int cch2 );
    int                                     strcmplocaleW( unsigned int dwFlags, const wchar_t* pwsz1, int cch1, const wchar_t* pwsz2, int cch2 );

    char*                                   strchri( const char* lpStart, char wMatch );
    wchar_t*                                strchriw( const wchar_t* lpStart, wchar_t wMatch );
    char*                                   strstri( const char* lpFirst, const char* lpSrch );
    wchar_t*                                strstriw( const wchar_t* lpFirst, const wchar_t* lpSrch );
    int                                     strcmpni( const char* lpStr1, const char* lpStr2, int nChar );
    int                                     strcmpniw( const wchar_t* lpStr1, const wchar_t* lpStr2, int nChar );

    bool                                    strcontains( const char* lhs, const char* rhs, bool isCaseSensitive = false );
    bool                                    strcontains( const wchar_t* lhs, const wchar_t* rhs, bool isCaseSensitive = false );
    bool                                    strcontains( const std::string& lhs, const std::string& rhs, bool isCaseSensitive = false );
    bool                                    strcontains( const std::wstring& lhs, const std::wstring& rhs, bool isCaseSensitive = false );

} // nsCmn

#endif // HDR_INTERNAL_STRING