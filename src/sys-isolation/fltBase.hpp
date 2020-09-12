#ifndef WINIOMONITOR_FLTBASE_HPP
#define WINIOMONITOR_FLTBASE_HPP

#include "stdafx.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#if defined(__cplusplus)
    #define EXTERN_C extern "C"
    #define EXTERN_C_BEGIN extern "C" {
    #define EXTERN_C_END }
#else
#define EXTERN_C extern
    #define EXTERN_C_BEGIN {
    #define EXTERN_C_END }
#endif

#define FlagOnAll( F, T )                                                    \
    (FlagOn( F, T ) == T)

#define NULLPTR nullptr

#define IF_FAILED_BREAK( var, expr ) if( !NT_SUCCESS( ( (var) = (expr) ) ) ) break;
#define IF_FAILED_LEAVE( var, expr ) if( !NT_SUCCESS( ( (var) = (expr) ) ) ) __leave;

#ifndef NO_VTABLE
    #if CMN_MSC_VER > 0  
        #define NO_VTABLE __declspec(novtable) 
    #else
        #define NO_VTABLE 
    #endif
#endif

#include "pool.hpp"

#endif //WINIOMONITOR_FLTBASE_HPP
