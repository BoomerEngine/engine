/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher #]
***/

#pragma once

// NOTE: this header must NOT leak into public namespace

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif

#ifdef PLATFORM_WINDOWS

#ifdef CONSOLE

#define LAUNCHER_MAIN() int wmain(int argc, wchar_t **argv)
#define LAUNCHER_APP_HANDLE() GetModuleHandle(NULL)
#define LAUNCHER_PARSE_CMDLINE() cmdline.parse(GetCommandLineW(), true)

#else

#define LAUNCHER_MAIN() int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#define LAUNCHER_APP_HANDLE() hInstance
#define LAUNCHER_PARSE_CMDLINE() cmdline.parse(lpCmdLine, false)

#endif
#else

#define LAUNCHER_MAIN() int main(int argc, char **argv)
#define LAUNCHER_APP_HANDLE() nullptr
#define LAUNCHER_PARSE_CMDLINE() cmdline.parse(argc, argv)

#endif
