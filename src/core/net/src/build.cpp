/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [# dependency: base_system, base_memory, base_containers, base_io, base_config, base_fibers, base_reflection #]
* [# dependency: base_socket, base_replication #]
* [# privatelib: curl #]
***/

#include "build.h"
#include "static_init.inl"
#include "reflection.inl"

#ifdef PLATFORM_WINDOWS
    #include <WinSock2.h>
    #pragma comment(lib, "Ws2_32.lib")
#endif

DECLARE_MODULE(PROJECT_NAME)
{
}
