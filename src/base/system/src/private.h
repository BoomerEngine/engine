/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#ifdef PLATFORM_WINDOWS

    // Define windows versions
    #ifdef _WIN32_WINNT
        #undef _WIN32_WINNT
    #endif

    #define _WIN32_WINNT _WIN32_WINNT_VISTA
    #define NTDDI_VERSION NTDDI_VISTASP1

    #ifdef _WIN32_WINDOWS
        #undef _WIN32_WINDOWS
    #endif

    #define _WIN32_WINDOWS 0x0501

    // Suppress stuff from windows.h
    #define _TCHAR_DEFINED
    
    // Security stuff
    #ifndef _CRT_NON_CONFORMING_SWPRINTFS
        #define _CRT_NON_CONFORMING_SWPRINTFS
    #endif

    // Security stuff
    #ifndef _CRT_SECURE_NO_DEPRECATE
        #define _CRT_SECURE_NO_DEPRECATE
    #endif

    // Security stuff
    #ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
        #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #endif

    // Windows specific headers
    #include <winsock2.h>
    #include <windows.h>
    #include <locale.h>
    #include <process.h>

#elif defined(PLATFORM_LINUX)


#else

    #error "Please define the platform"

#endif


