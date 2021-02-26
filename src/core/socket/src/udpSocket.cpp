/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: udp #]
***/

#include "build.h"
#include "udpSocket.h"
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

BEGIN_BOOMER_NAMESPACE_EX(socket::udp)

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

RawSocket::RawSocket()
{}

bool RawSocket::open(const Address& address, Address* outLocalAddress)
{
    m_ipv6 = address.type() == AddressType::AddressIPv6;
    m_socket = ::socket(m_ipv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);

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

    if (bind(m_socket, listenAddress, listenAddressSize) < 0)
    {
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

int RawSocket::receive(void* data, int size, Address* outSourceAddress)
{
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

    int bytesReceived = recvfrom(m_socket,
        reinterpret_cast<char *>(data),
        size,
        0,
        receiveAddress,
        &receiveAddressSize);

    if (bytesReceived < 0)
    {
        int error = GetSocketError();
        if (WouldBlock(error))
            return 0;

        if (PortUnreachable(error))
        {
            TRACE_ERROR("Previously sent UDP datagram was dropped by recipient because the port was unreachable");
            return 0;
        }

        TRACE_ERROR("recvfrom() failed with error {}", error);
        return -1;
    }

    networkToAddress(receiveAddress, outSourceAddress);

    return bytesReceived;
}

int RawSocket::send(const void* data, int size, const Address& destinationAddress)
{
    int bytesSent = 0;

    if (m_ipv6)
    {
        sockaddr_in6 address;
        memzero(&address, sizeof(address));
        address.sin6_family = AF_INET6;
        address.sin6_port = htons(destinationAddress.port());
        addressToNetwork6(destinationAddress, &address.sin6_addr);
        bytesSent = sendto(m_socket, reinterpret_cast<const char *>(data), size, 0, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    }
    else
    {
        sockaddr_in address;
        memzero(&address, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(destinationAddress.port());
        addressToNetwork(destinationAddress, &address.sin_addr);
        bytesSent = sendto(m_socket, reinterpret_cast<const char *>(data), size, 0, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    }

    if (bytesSent < 0)
    {
        int error = GetSocketError();
        if (WouldBlock(error))
            return 0;

        TRACE_ERROR("sendto() failed with error {}", error);
        return -1;
    }

    return bytesSent;
}

bool RawSocket::allowFragmentation(bool allowFragmentation)
{
#if defined(PLATFORM_WINDOWS)
    char doNotFragment = allowFragmentation ? 0 : 1;
    if (setsockopt(m_socket, IPPROTO_IP, m_ipv6 ? IPV6_DONTFRAG : IP_DONTFRAGMENT, &doNotFragment, sizeof(doNotFragment)) != 0)
    {
        TRACE_ERROR(TempString("Failed to change IP-level fragmentation policy with error {}", GetSocketError()));
        return false;
    }
#else
    int doNotFragment = allowFragmentation ? 0 : 1;
#if defined(IPDONTFRAG)
    if (!m_ipv6)
    {
        if (setsockopt(rawSocket, IPPROTO_IP, IP_DONTFRAG, &doNotFragment, sizeof(doNotFragment)) != 0)
        {
            TRACE_ERROR(TempString("Failed to change IP-level fragmentation policy with error {}", GetSocketError()));
            return false;
        }
    }
#endif
#if defined(IPV6_DONTFRAG)
    if (m_ipv6)
    {
        if (setsockopt(rawSocket, IPPROTO_IP, IPV6_DONTFRAG, &doNotFragment, sizeof(doNotFragment)) != 0)
        {
            TRACE_ERROR(TempString("Failed to change IP-level fragmentation policy with error {}", GetSocketError()));
            return false;
        }
    }
#endif
#endif
    return true;
}

bool RawSocket::bufferSize(int sendBufferBytes, int receiveBufferBytes)
{
#if defined(PLATFORM_WINDOWS)
    if (setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char *>(&sendBufferBytes), sizeof(sendBufferBytes)) != 0)
    {
        TRACE_ERROR(TempString("Failed to change send buffer size with error {}", GetSocketError()));
        return false;
    }

    if (setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&receiveBufferBytes), sizeof(receiveBufferBytes)) != 0)
    {
        TRACE_ERROR(TempString("Failed to change receive buffer size with error {}", GetSocketError()));
        return false;
    }
#else
    if (setsockopt(rawSocket, SOL_SOCKET, SO_SNDBUF, &sendBufferBytes, sizeof(sendBufferBytes)) != 0)
    {
        TRACE_ERROR(TempString("Failed to change send buffer size with error {}", GetSocketError()));
        return false;
    }

    if (setsockopt(rawSocket, SOL_SOCKET, SO_RCVBUF, &receiveBufferBytes, sizeof(receiveBufferBytes)) != 0)
    {
        TRACE_ERROR(TempString("Failed to change receive buffer size with error {}", GetSocketError()));
        return false;
    }
#endif

    return true;
}

END_BOOMER_NAMESPACE_EX(socket::udp)
