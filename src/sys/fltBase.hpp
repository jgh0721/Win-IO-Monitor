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

#endif //WINIOMONITOR_FLTBASE_HPP
