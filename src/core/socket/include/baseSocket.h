﻿/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: protocol #]
***/

#pragma once

#include "core/containers/include/queue.h"
#include "core/system/include/mutex.h"
#include "core/system/include/spinLock.h"
#include "core/system/include/scopeLock.h"

BEGIN_BOOMER_NAMESPACE_EX(socket)

//---

/// Base socket wrapper
class CORE_SOCKET_API BaseSocket : public NoCopy
{
public:
    BaseSocket();
    BaseSocket(BaseSocket&& other);
    BaseSocket& operator=(BaseSocket&& other);
    ~BaseSocket();

    // retrieve raw socket descriptor
    INLINE SocketType systemSocket() const { return m_socket; }

    // is this valid socket ?
    INLINE operator bool() const { return m_socket != SocketInvalid; }

    // is this socket is using IPv6
    INLINE bool isIP6() const { return m_ipv6; }

    // close the socket and clean up resources
    void close();

    // Set socket blocking mode
    bool blocking(bool blocking);

protected:
    SocketType m_socket;
    bool m_ipv6;
};

//---

END_BOOMER_NAMESPACE_EX(socket)
