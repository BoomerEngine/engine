/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages\tcp #]
***/

#pragma once

#include "base/socket/include/tcpServer.h"

BEGIN_BOOMER_NAMESPACE(base::net)

class TcpMessageServerConnection;
struct TcpMessageServerConnectionState;

/// high level integration of message system and TCP server
/// hosts a server with collection of objects registered first by attachObject that can receive messages from connected clients
class BASE_NET_API TcpMessageServer : public IObject, public socket::tcp::IServerHandler
{
    RTTI_DECLARE_VIRTUAL_CLASS(TcpMessageServer, IObject);

public:
    TcpMessageServer();
    virtual ~TcpMessageServer();

    ///---

    /// get local listening address
    socket::Address listeningAddress() const;

    /// start server on local port
    bool startListening(uint16_t port);

    /// finish listening
    void stopListening();

    /// are we listening ?
    bool isListening() const;

    ///---

    /// broadcast message over all connections
    template< typename T >
    INLINE void broadcast(const T& messageData)
    {
        broadcast(&messageData, reflection::GetTypeObject<T>());
    }

    //---

    /// get next new connection that got since last call
    MessageConnectionPtr pullNextAcceptedConnection();

    //---

private:
    replication::DataModelRepositoryPtr m_models;
            
    socket::tcp::Server m_server;

    Mutex m_activeConnectionsLock;
    Array<TcpMessageServerConnectionState*> m_activeConnections;
    HashMap<socket::ConnectionID, TcpMessageServerConnectionState*> m_activeConnectionMap;

    Mutex m_newConnectionsLock;
    Queue<MessageConnectionPtr> m_newConnections;

    friend class TcpMessageServerConnection;

    //

    bool checkConnectionStatus(socket::ConnectionID id);
    void closeConnection(socket::ConnectionID id);

    void broadcast(const void* messageData, Type messageClass);
    void send(socket::ConnectionID, const void* messageData, Type messageClass);

    virtual void handleConnectionAccepted(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection) override final;
    virtual void handleConnectionClosed(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection) override final;
    virtual void handleConnectionData(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection, const void* data, uint32_t dataSize) override final;
    virtual void handleServerClose(socket::tcp::Server* server) override final;

};

END_BOOMER_NAMESPACE(base::net)