/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# dependency: base_system, base_memory, base_containers #]
* [# dependency: base_object, base_reflection, base_app, base_parser, base_config, base_resource #]
***/

#include "build.h"
#include "reflection.inl"
#include "static_init.inl"

#ifdef PLATFORM_WINDOWS
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

DECLARE_MODULE(PROJECT_NAME)
{
    // custom module initialization code
}

void Dupa()
{

}