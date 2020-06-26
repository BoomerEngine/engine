/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [# dependency: base_system, base_memory, base_containers, base_config, base_fibers #]
***/

#include "build.h"
#include "static_init.inl"

#ifdef PLATFORM_WINDOWS
    #include <WinSock2.h>
    #pragma comment(lib, "Ws2_32.lib")
#endif

namespace base
{
    namespace socket
    {
        void Initialize()
        {
#ifdef PLATFORM_WINDOWS
            WSADATA wsaData;
            int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (result != NO_ERROR) {
                FATAL_ERROR(TempString("WSAStartup failed with error {}", result));
                Shutdown();
            }
#endif
        }

        void Shutdown()
        {
#ifdef PLATFORM_WINDOWS
            WSACleanup();
#endif
        }

        int GetSocketError()
        {
#if defined (PLATFORM_WINDOWS)
            return WSAGetLastError();
#else
            return errno;
#endif
        }

        bool WouldBlock(int error)
        {
#if defined (PLATFORM_WINDOWS)
            if (error == WSAEWOULDBLOCK)
                return true;
#else
            if (error == EWOULDBLOCK)
                return true;
#endif
            return false;
        }

        bool PortUnreachable(int error)
        {
#if defined (PLATFORM_WINDOWS)
            return error == 10054;
#else
            // TODO: Do other systems do the naughty things Windows does?
            return false;
#endif
        }

    } // socket
} // base

DECLARE_MODULE(PROJECT_NAME)
{
	base::socket::Initialize();
}

