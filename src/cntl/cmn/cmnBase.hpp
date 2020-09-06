#ifndef HDR_INTERNAL_BASE
#define HDR_INTERNAL_BASE

#include <math.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <tuple>

#include <iterator>
#include <utility>
#include <float.h>
#include <limits>
#include <tuple>

#include "cmnCompilerDetection.hpp"
#include "cmnSystemDetection.hpp"
#include "cmnConfig.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define CMN_BUILD_DEBUG 0
#define CMN_BUILD_RELEASE 1

#if defined(_DEBUG) || defined(DEBUG)
#   define CMN_BUILD_MODE CMN_BUILD_DEBUG
#else
#   if defined(NDEBUG) || defined(QT_NO_DEBUG)
#       define CMN_BUILD_MODE CMN_BUILD_RELEASE
#   else
#       define CMN_BUILD_MODE CMN_BUILD_DEBUG
#   endif
#endif

#define ONE_SECOND_MS (1000)
#define ONE_MINUTE_MS (ONE_SECOND_MS * 60)
#define ONE_HOUR_MS   (ONE_MINUTE_MS * 60)
#define ONE_SECOND_SEC (ONE_SECOND_MS / ONE_SECOND_MS)
#define ONE_MINUTE_SEC (ONE_MINUTE_MS / ONE_SECOND_MS)
#define ONE_HOUR_SEC   (ONE_HOUR_MS / ONE_SECOND_MS)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define __FILENAMEW__ (wcsrchr(__FILEW__, L'/') ? wcsrchr(__FILEW__, L'/') + 1 : wcsrchr(__FILEW__, L'\\') ? wcsrchr(__FILEW__, L'\\') + 1 : __FILEW__)

#define __FUNCNAME__  ( strrchr(__FUNCTION__, ':' ) ? strrchr( __FUNCTION__, ':' ) + 1 : __FUNCTION__ )
#define __FUNCNAMEW__ ( wcsrchr(__FUNCTIONW__, L':' ) ? wcsrchr( __FUNCTIONW__, L':' ) + 1 : __FUNCTIONW__ )

#define IF_FALSE_BREAK( var, expr ) if( ( (var) = (expr) ) == false ) break;
#define IF_SUCCESS_BREAK( var, expr ) if( ( (var) = (expr) ) == true ) break;


#include "cmnTypes.hpp"

#endif // HDR_INTERNAL_BASE