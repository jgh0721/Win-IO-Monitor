
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <tchar.h>
#include <windows.h>

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtSql>

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#include "cmnBase.hpp"

#ifdef _WIN64

    #ifdef _DEBUG
        #ifdef _DLL
            #pragma comment(lib, "WinIOMonitorAPI-v141_xp-x64-s-d")
        #else
            #pragma comment(lib, "WinIOMonitorAPI-v141_xp-x64-s-mt-d")
        #endif
    #else
        #ifdef _DLL
            #pragma comment(lib, "WinIOMonitorAPI-v141_xp-x64-s-r")
        #else
            #pragma comment(lib, "WinIOMonitorAPI-v141_xp-x64-s-mt-r")
        #endif
    #endif

#else

    #ifdef _DEBUG
        #ifdef _DLL
            #pragma comment(lib, "WinIOMonitorAPI-v141_xp-x86-s-d")
        #else
            #pragma comment(lib, "WinIOMonitorAPI-v141_xp-x86-s-mt-d")
        #endif
    #else
        #ifdef _DLL
            #pragma comment(lib, "WinIOMonitorAPI-v141_xp-x86-s-r")
        #else
            #pragma comment(lib, "WinIOMonitorAPI-v141_xp-x86-s-mt-r")
        #endif
    #endif

#endif

