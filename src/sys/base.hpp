#pragma once

#define _WIN32_WINNT 0x0501
#define WINVER 0x0501
#define WIN32_LEAN_AND_MEAN
#define NTDDI_VERSION 0x05010200

#if defined(__cplusplus)
#define EXTERN_C extern "C"
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C extern
#define EXTERN_C_BEGIN {
#define EXTERN_C_END }
#endif

#pragma warning(push)
#pragma warning(disable: 4510)
#pragma warning(disable: 4512)
#pragma warning(disable: 4610)
#pragma warning(disable: 4995)

EXTERN_C_BEGIN

#include <ntifs.h>
#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdm.h>
#include <fltkernel.h>
#include <sal.h>
#include <WinDef.h>
#include <dontuse.h>
#include <suppress.h>

EXTERN_C_END

#include <stdlib.h>
#include <stdarg.h>

#pragma warning(pop)

#pragma comment( lib, "ntoskrnl.lib" )

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof(x[0]))
#endif

#ifndef NULLPTR
#define NULLPTR __nullptr
#endif

#ifndef NO_VTABLE
    #if defined(_MSC_VER) && _MSC_VER > 0  
        #define NO_VTABLE __declspec(novtable) 
    #else
        #define NO_VTABLE 
    #endif
#endif

typedef signed char         int8_t;
typedef short               int16_t;
typedef int                 int32_t;
typedef long long           int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef long long           intmax_t;
typedef unsigned long long  uintmax_t;
