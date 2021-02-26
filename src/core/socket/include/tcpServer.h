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

#include "core/system/include/thread.h"
#include "core/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE_EX(socket::tcp)

//--

/// server event handler
/// NOTE: all events happen from NON-FIBER network threads
class CORE_SOCKET_API IServerHandler : public NoCopy
{
public:
    virtual ~IServerHandler();

    /// we got a connection
    virtual void handleConnectionAccepted(Server* server, const Address& address, ConnectionID connection) = 0;

    /// connection we had got closed
    virtual void handleConnectionClosed(Server* server, const Address& address, ConnectionID connection) = 0;

    /// data was received by connection, returns the payload
    virtual void handleConnectionData(Server* server, const Address& address, ConnectionID connection, const void* data, uint32_t dataSize) = 0;

    /// server got closed unexpectedly
    virtual void handleServerClose(Server* server) = 0;
};

//--

// server configuration params
struct ServerConfig
{
    uint32_t pollTimeout = 50; // timeout for internal poll calls
    uint32_t recvBufferSize = 8192; // size of the receive buffer (we read data from sockets in batches of this size)
};

//---

/// "No expenses spared" TCP server that has it's onw thread, etc
/// This class managed the one TcpSocket and automatically handles connections/disconnections
/// NOTE: we do not do any message framing here, it's just straight pass-though to higher layer
class CORE_SOCKET_API Server : public NoCopy
{
public:
    Server(IServerHandler* handler, const ServerConfig& config = ServerConfig());
    ~Server();

    //--

    // get the address we are listening on
    INLINE const Address& address() const { return m_address; }

    // are we listening ? this gets reset if our main socket has some errors
    INLINE bool isListening() const { return m_listeningFlag.load(); }

    //--

    // bind a TCP socket on given address (usually a 0.0.0.0 + some port)
    // connections can be established as soon as this function returns
    bool init(const Address& listenAddress);

    // close endpoint without destroying all data and other setup
    void close();

    //--

    // send data via connection, fails only if connection ID is no longer valid
    // NOTE: data may arrive in different "chunks" than sent, remember about framing
    bool send(ConnectionID id, const void* data, uint32_t dataSize);

    // disconnect a given connection from server
    bool disconnect(ConnectionID id);

    // get stats for given connection
    bool stat(ConnectionID id, ConnectionStats& outStats) const;

private:
    IServerHandler* m_handler;

    ServerConfig m_config;

    RawSocket m_socket;
    Address m_address;

    std::atomic<ConnectionID> m_nextConnectionID;
    std::atomic<uint32_t> m_listeningFlag;
    std::atomic<uint32_t> m_initFlag;

    Thread m_thread;

    //--

    struct Connection : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_NET)

    public:
        Address address; // remote address
        ConnectionID id = 0; // assigned ID
        NativeTimePoint connectedTime; // to count the transfer rates/debug, etc
        NativeTimePoint lastMessageTime; // so ma close inactive users
        RawSocket rawSocket; // the raw TCP socket serving this connection
        ConnectionStats stats; // connection stats

        std::atomic<uint32_t> closeRequest; // we got external request to close this connection
    };

    Array<Connection*> m_activeConnections;
    HashMap<ConnectionID, Connection*> m_activeConnectionsIDMap;
    HashMap<SocketType , Connection*> m_activeConnectionsSocketMap;
    SpinLock m_activeConnectionsLock;

    Array<uint8_t> m_receciveBuffer;

    //--

    void threadFunc();

    void collectActiveConnections(Array<SocketType>& outActiveSockets);
    void purgeConnection(Connection* connection);

    void serviceListenerClose();
    void serviceListenerSocket(SocketType socket);
    void serviceConnectionSocket(SocketType socket, bool error);
};

//---

END_BOOMER_NAMESPACE_EX(socket::tcp)
