/***
* Boomer Engine v4
* Written by Lukasz "Krawiec" Krawczyk
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_socket_glue.inl"

BEGIN_BOOMER_NAMESPACE_EX(socket)

class Address;
class Block;
class BlockAllocator;
class BaseSocket;
class Selector;

typedef uint32_t ConnectionID;

//--

namespace udp
{
    class RawSocket;

    class Packet;
    class Endpoint;
    class IEndpointHandler;
} // udp

//--

namespace tcp
{
    class RawSocket;
    class Server;
    class IServerHandler;
    class Client;
    class IClientHandler;
} // udp

//--

// Retrieve the most recently set error code, has to be checked immediately, fter failing call
extern CORE_SOCKET_API int GetSocketError();

// Check if the error code means that the call would have blocked
extern CORE_SOCKET_API bool WouldBlock(int error);

// Check if the error resulted from ICMP message sent to UDP socket on unreachable destination port
extern CORE_SOCKET_API bool PortUnreachable(int error);

// Initialize network
extern CORE_SOCKET_API void Initialize();

// Shut down network layer
extern CORE_SOCKET_API void Shutdown();

//--

// simple union
template<typename R, typename S>
struct ResultWithStatus
{
    R result;
    S status;

    INLINE ResultWithStatus(R result, S status)
        : result(result)
        , status(status)
    {}
};

//--

// part of data
struct BlockPart
{
    const void* dataPtr = nullptr;
    uint32_t size = 0;

    INLINE BlockPart() {};
    INLINE BlockPart(const BlockPart& other) = default;
    INLINE BlockPart& operator=(const BlockPart& other) = default;
    INLINE BlockPart(const void* dataPtr, uint32_t size) : dataPtr(dataPtr), size(size) {};
};

//--

// VirtualSocket type
#ifdef PLATFORM_32BIT
typedef uint32_t SocketType;
#elif defined(PLATFORM_64BIT)
typedef uint64_t SocketType;
#endif

//--

static const SocketType SocketInvalid = (SocketType)-1;

//--

END_BOOMER_NAMESPACE_EX(socket)
