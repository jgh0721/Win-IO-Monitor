#ifndef WIN_IO_MONITOR_STDAFX_HPP
#define WIN_IO_MONITOR_STDAFX_HPP

#define _WIN32_WINNT 0x0501
#define WINVER 0x0501
#define WIN32_LEAN_AND_MEAN
#define NTDDI_VERSION 0x05010200

#pragma warning(push)
#pragma warning(disable: 4510)
#pragma warning(disable: 4512)
#pragma warning(disable: 4610)
#pragma warning(disable: 4995)

#ifdef __cplusplus
extern "C" {
#endif

#include <ntifs.h>
#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdm.h>
#include <fltkernel.h>
#include <sal.h>

#ifdef __cplusplus
}
#endif

#pragma warning(pop)

#pragma comment( lib, "ntoskrnl.lib" )

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#endif //WIN_IO_MONITOR_STDAFX_HPP
