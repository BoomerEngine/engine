/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: raw #]
***/

#include "build.h"
#include "address.h"
#include "baseSocket.h"

#if defined (PLATFORM_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#endif

namespace base
{
    namespace socket
    {
        static void addressToNetwork(const Address& address, in_addr* networkAddress)
        {
            ASSERT(address.type() == AddressType::AddressIPv4);
#if defined (PLATFORM_WINDOWS)
            networkAddress->S_un.S_addr = *reinterpret_cast<const u_long *>(address.address());
#else
            networkAddress->s_addr = *reinterpret_cast<const u_long *>(address.address());
#endif
        }

        static void addressToNetwork6(const Address& address, in6_addr* networkAddress)
        {
            ASSERT(address.type() == AddressType::AddressIPv6);
            memcpy(networkAddress, address.address(), sizeof(in6_addr));
        }

        static void networkToAddress(const sockaddr* networkAddress, Address* address)
        {
            if (networkAddress->sa_family == AF_INET6)
            {
                const sockaddr_in6* a = reinterpret_cast<const sockaddr_in6 *>(networkAddress);
#if defined (PLATFORM_WINDOWS)
                address->set(AddressType::AddressIPv6, ntohs(a->sin6_port), reinterpret_cast<const uint8_t *>(&a->sin6_addr.u.Byte));
#else
                address->set(AddressType::AddressIPv6, ntohs(a->sin6_port), reinterpret_cast<const uint8_t *>(&a->sin6_addr.__in6_u.__u6_addr8));
#endif
            }
            else if (networkAddress->sa_family == AF_INET)
            {
                const sockaddr_in* a = reinterpret_cast<const sockaddr_in *>(networkAddress);
#if defined (PLATFORM_WINDOWS)
                address->set(AddressType::AddressIPv4, ntohs(a->sin_port), reinterpret_cast<const uint8_t *>(&a->sin_addr.S_un.S_addr));
#else
                address->set(AddressType::AddressIPv4, ntohs(a->sin_port), reinterpret_cast<const uint8_t *>(&a->sin_addr.s_addr));
#endif
            }
            else
            {
                ASSERT_EX(false, "Unsupported address family");
            }
        }

        BaseSocket::BaseSocket()
            : m_socket(SocketInvalid)
            , m_ipv6(false)
        {}

        BaseSocket::BaseSocket(BaseSocket&& other)
            : m_socket(other.m_socket)
            , m_ipv6(other.m_ipv6)
        {
            other.m_socket = SocketInvalid;
            other.m_ipv6 = false;
        }

        BaseSocket& BaseSocket::operator=(BaseSocket&& other)
        {
            if (this != &other)
            {
                close();
                m_socket = other.m_socket;
                m_ipv6 = other.m_ipv6;
                other.m_socket = SocketInvalid;
                other.m_ipv6 = false;
            }

            return *this;
        }

        BaseSocket::~BaseSocket()
        {
            close();
        }

        void BaseSocket::close()
        {
            if (m_socket != SocketInvalid)
            {
                TRACE_INFO("closing socket {}", m_socket);
#if defined (PLATFORM_WINDOWS)
                closesocket(m_socket);
#else
                ::close(rawSocket);
#endif
                m_socket = SocketInvalid;
            }
        }

        bool BaseSocket::blocking(bool blocking)
        {
#if defined (PLATFORM_WINDOWS)
            DWORD nonBlocking = blocking ? 0 : 1;
            if (ioctlsocket(m_socket, FIONBIO, &nonBlocking) != 0)
            {
                TRACE_ERROR(TempString("Failed to set socket in non-blocking mode with error {}", GetSocketError()));
                return false;
            }
#else
            int flags = fcntl(rawSocket, F_GETFL, 0);
            if (blocking)
            {
                flags &= O_NONBLOCK;
            }
            else
            {
                flags |= O_NONBLOCK;
            }

            if (fcntl(rawSocket, F_SETFL, flags) == -1)
            {
                TRACE_ERROR(TempString("Failed to set socket in non-blocking mode with error {}", GetSocketError()));
                return false;
            }
#endif
            return true;
        }

    } // socket
} // base