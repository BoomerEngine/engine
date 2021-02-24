/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: udp #]
***/

#pragma once

#include "baseSocket.h"

BEGIN_BOOMER_NAMESPACE(base::socket::udp)

// Raw UDP socket wrapper
class BASE_SOCKET_API RawSocket : public BaseSocket
{
public:
    RawSocket();
    RawSocket(RawSocket&& other) = default;
    RawSocket& operator=(RawSocket&& other) = default;

    //---

    // Open bidirectional datagram socket and bind it to specified address
    bool open(const Address& address, Address* outLocalAddress = nullptr);

    // Receive data from the socket
    int receive(void* data, int size, Address* outSourceAddress);

    // Send data through the socket
    int send(const void* data, int size, const Address& destinationAddress);

    // Set IP-level fragmentation policy
    bool allowFragmentation(bool allowFragmentation);

    // Set send and receive buffer sizes
    bool bufferSize(int sendBufferBytes, int receiveBufferBytes);
};

END_BOOMER_NAMESPACE(base::socket::udp)
