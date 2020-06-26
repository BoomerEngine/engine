/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: raw #]
***/

#include "build.h"
#include "tcpSocket.h"
#include "address.h"

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
        namespace tcp
        {

            //--

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

            //--

            bool RawSocket::listen(const Address& address, Address* outLocalAddress /*= nullptr*/)
            {
                close();

                m_ipv6 = address.type() == AddressType::AddressIPv6;
                m_socket = ::socket(m_ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);

                if (m_socket == SocketInvalid)
                {
                    TRACE_ERROR("Failed to create socket {}", GetSocketError());
                    return false;
                }

                sockaddr* listenAddress = nullptr;
                int listenAddressSize = 0;

                sockaddr_in6 socketAddress6;
                sockaddr_in socketAddress4;

                if (m_ipv6)
                {
                    memzero(&socketAddress6, sizeof(socketAddress6));
                    socketAddress6.sin6_family = AF_INET6;
                    socketAddress6.sin6_port = htons(address.port());
                    addressToNetwork6(address, &socketAddress6.sin6_addr);

                    listenAddress = reinterpret_cast<sockaddr *>(&socketAddress6);
                    listenAddressSize = sizeof(socketAddress6);
                }
                else
                {
                    memzero(&socketAddress4, sizeof(socketAddress4));
                    socketAddress4.sin_family = AF_INET;
                    socketAddress4.sin_port = htons(address.port());
                    addressToNetwork(address, &socketAddress4.sin_addr);

                    listenAddress = reinterpret_cast<sockaddr *>(&socketAddress4);
                    listenAddressSize = sizeof(socketAddress4);
                }

                if (::bind(m_socket, listenAddress, listenAddressSize) < 0)
                {
                    TRACE_ERROR("Failed to bind socket to {}: {}", address, GetSocketError());
                    close();
                    return false;
                }

                if (::listen(m_socket, 64) != 0)
                {
                    TRACE_ERROR("Failed to listen on socket {}: {}", address, GetSocketError());
                    close();
                    return false;
                }

                if (outLocalAddress)
                {
                    if (0 != ::getsockname(m_socket, listenAddress, &listenAddressSize))
                    {
                        int error = GetSocketError();
                        TRACE_ERROR("getsockname() failed with error {}", error);
                        *outLocalAddress = Address(); // TODO: find better way
                    }
                    else
                    {
                        networkToAddress(listenAddress, outLocalAddress);
                    }
                }

                return true;
            }

            bool RawSocket::connect(const Address& address, Address* outLocalAddress /*= nullptr*/)
            {
                close();

                m_ipv6 = address.type() == AddressType::AddressIPv6;
                m_socket = ::socket(m_ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);

                sockaddr* connectAddress = nullptr;
                socklen_t connectAddressSize = 0;

                sockaddr_in6 socketAddress6;
                sockaddr_in socketAddress4;

                if (m_ipv6)
                {
                    memzero(&socketAddress6, sizeof(socketAddress6));
                    socketAddress6.sin6_family = AF_INET6;
                    socketAddress6.sin6_port = htons(address.port());
                    addressToNetwork6(address, &socketAddress6.sin6_addr);

                    connectAddress = reinterpret_cast<sockaddr *>(&socketAddress6);
                    connectAddressSize = sizeof(socketAddress6);
                }
                else
                {
                    memzero(&socketAddress4, sizeof(socketAddress4));
                    socketAddress4.sin_family = AF_INET;
                    socketAddress4.sin_port = htons(address.port());
                    addressToNetwork(address, &socketAddress4.sin_addr);

                    connectAddress = reinterpret_cast<sockaddr *>(&socketAddress4);
                    connectAddressSize = sizeof(socketAddress4);
                }

                auto ret  = ::connect(m_socket, connectAddress, connectAddressSize);
                if (ret < 0)
                {
                    int error = GetSocketError();
                    if (WouldBlock(error))
                        return false;

                    if (PortUnreachable(error))
                    {
                        TRACE_ERROR("Previously sent TCP packet was dropped by recipient because the port was unreachable");
                        return false;
                    }

                    TRACE_ERROR("connect() failed with error {}", error);
                    close();
                    return false;
                }

                if (outLocalAddress)
                {
                    if (0 != ::getsockname(m_socket, connectAddress, &connectAddressSize))
                    {
                        int error = GetSocketError();
                        TRACE_ERROR("getsockname() failed with error {}", error);
                        *outLocalAddress = Address(); // TODO: find better way
                    }
                    else
                    {
                        networkToAddress(connectAddress, outLocalAddress);
                    }
                }

                return true;
            }

            bool RawSocket::accept(const RawSocket& listener, Address* outSourceAddress /*= nullptr*/)
            {
                if (listener.m_socket == SocketInvalid)
                {
                    TRACE_ERROR("accept() called on bad socket");
                    return false;
                }

                sockaddr* acceptAddress = nullptr;
                socklen_t acceptAddressSize = 0;

                sockaddr_in6 socketAddress6;
                sockaddr_in socketAddress4;

                if (listener.m_ipv6)
                {
                    memzero(&socketAddress6, sizeof(socketAddress6));
                    acceptAddress = reinterpret_cast<sockaddr *>(&socketAddress6);
                    acceptAddressSize = sizeof(socketAddress6);
                }
                else
                {
                    memzero(&socketAddress4, sizeof(socketAddress4));
                    acceptAddress = reinterpret_cast<sockaddr *>(&socketAddress4);
                    acceptAddressSize = sizeof(socketAddress4);
                }

                int ret = ::accept(listener.m_socket, acceptAddress, &acceptAddressSize);
                if (ret < 0)
                {
                    int error = GetSocketError();
                    if (WouldBlock(error))
                        return false;

                    if (PortUnreachable(error))
                    {
                        TRACE_ERROR("Previously sent TCP packet was dropped by recipient because the port was unreachable");
                        return false;
                    }

                    TRACE_ERROR("accept() failed with error {}", error);
                    return false;
                }

                close();

                m_ipv6 = listener.m_ipv6;
                m_socket = ret;

                networkToAddress(acceptAddress, outSourceAddress);
                return true;
            }

            int RawSocket::receive(void* data, int size)
            {
                if (m_socket == SocketInvalid)
                {
                    TRACE_ERROR("receive() called on bad socket");
                    return false;
                }

                sockaddr* receiveAddress = nullptr;
                socklen_t receiveAddressSize = 0;

                if (m_ipv6)
                {
                    sockaddr_in6 address;
                    receiveAddress = reinterpret_cast<sockaddr *>(&address);
                    receiveAddressSize = sizeof(address);
                }
                else
                {
                    sockaddr_in address;
                    receiveAddress = reinterpret_cast<sockaddr *>(&address);
                    receiveAddressSize = sizeof(address);
                }

                int bytesReceived = recv(m_socket, reinterpret_cast<char *>(data), size, 0);
                if (bytesReceived < 0)
                {
                    int error = GetSocketError();
                    if (WouldBlock(error))
                        return 0;

                    if (PortUnreachable(error))
                    {
                        TRACE_ERROR("Previously sent TCP packet was dropped by recipient because the port was unreachable");
                        return 0;
                    }

                    TRACE_ERROR("recv() failed with error {}", error);
                    return -2;
                }
                else if (bytesReceived == 0)
                {
                    TRACE_ERROR("recv() detected orderly shutdown");
                    return -1;
                }

                return bytesReceived;
            }

            int RawSocket::send(const void* data, int size)
            {
                if (m_socket == SocketInvalid)
                {
                    TRACE_ERROR("send() called on bad socket");
                    return false;
                }

                int bytesSent = ::send(m_socket, reinterpret_cast<const char *>(data), size, 0);
                if (bytesSent < 0)
                {
                    int error = GetSocketError();
                    if (WouldBlock(error))
                        return 0;

                    TRACE_ERROR("send() failed with error {}", error);
                    return -1;
                }

                return bytesSent;
            }

        } // tcp
    } // socket
} // base