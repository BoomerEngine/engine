/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tcp #]
***/

#pragma once

#include "address.h"
#include "baseSocket.h"
#include "tcpSocket.h"

#include "base/system/include/timing.h"
#include "base/system/include/thread.h"
#include "base/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE(base::socket::tcp)

//--

/// client event handler
/// NOTE: all events happen from NON-FIBER network threads
class BASE_SOCKET_API IClientHandler : public NoCopy
{
public:
    virtual ~IClientHandler();

    /// connection we had got closed
    virtual void handleConnectionClosed(Client* client, const Address& address) = 0;

    /// data was received by connection, returns the payload
    virtual void handleConnectionData(Client* client, const Address& address, const void* data, uint32_t dataSize) = 0;
};

//--

// client configuration params
struct ClientConfig
{
    uint32_t pollTimeout = 50; // timeout for internal poll calls
    uint32_t recvBufferSize = 8192; // size of the receive buffer (we read data from sockets in batches of this size)
};

//---

/// "No expenses spared" TCP client that has it's onw thread, etc
/// This class manages the one TcpSocket as a client to a TcpServer
/// NOTE: we do not do any message framing here, it's just straight pass-though to higher layer
class BASE_SOCKET_API Client : public NoCopy
{
public:
    Client(IClientHandler* handler, const ClientConfig& config = ClientConfig());
    ~Client();

    //--

    // get the local address
    INLINE const Address& localAddress() const { return m_localAddress; }

    // get the address we are connected to, valid only if connected
    INLINE const Address& remoteAddress() const { return m_remoteAddress; }

    // are we connected ?
    INLINE bool isConnected() const { return m_connectedFlag.load(); }

    //--

    // connect to remote address
    bool connect(const Address& targetAddress);

    // close endpoint without destroying all data and other setup
    void close();

    //--

    // send data via connection to server
    bool send( const void* data, uint32_t dataSize);

    // get stats for this client connection
    void stat(ConnectionStats& outStats) const;

private:
    IClientHandler* m_handler;

    ClientConfig m_config;

    RawSocket m_socket;
    Address m_remoteAddress;
    Address m_localAddress;

    Thread m_thread;

    std::atomic<uint32_t> m_initFlag;
    std::atomic<uint32_t> m_connectedFlag;

    ConnectionStats m_stats;
    SpinLock m_statsLock;

    Array<uint8_t> m_receciveBuffer;

    //--

    void threadFunc();

    void handleCloseEvent();
    bool handleIncomingData();
};

//---

END_BOOMER_NAMESPACE(base::socket::tcp)