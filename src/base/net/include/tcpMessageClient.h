/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages\tcp #]
***/

#pragma once

#include "base/socket/include/tcpClient.h"
#include "messageConnection.h"

BEGIN_BOOMER_NAMESPACE(base::net)

//---

/// high level integration of message system and TCP client
class BASE_NET_API TcpMessageClient : public MessageConnection, public socket::tcp::IClientHandler
{
    RTTI_DECLARE_VIRTUAL_CLASS(TcpMessageClient, MessageConnection);

public:
    TcpMessageClient();
    virtual ~TcpMessageClient();

    ///---

    /// get the ID of the connection as seen by the owner
    virtual uint32_t connectionId() const override final;

    /// get local address
    /// NOTE: does NOT have to be a network address
    virtual StringBuf localAddress() const override final;

    /// get remote address
    /// NOTE: does NOT have to be a network address
    /// NOTE: in principle it should be possible to use this address to reconnect the connection
    virtual StringBuf remoteAddress() const override final;

    /// are we still connected ?
    virtual bool isConnected() const override final;

    /// close connection to server
    virtual void close() override final;

    //---

    /// pull next message from queue of received messages, returns NULL if there are no more messages in the queue
    virtual MessagePtr pullNextMessage() override final;

    //---

    /// connect to TCP server
    bool connect(const socket::Address& address);

    /// push message onto the queue (NOTE: does not have to be a real one)
    void pushNextMessage(const MessagePtr& message);

    //---

private:
    replication::DataModelRepositoryPtr m_models;

    UniquePtr<MessageReplicator> m_replicator;

    UniquePtr<MessageReassembler> m_reassembler;

    socket::tcp::Client m_client;
    uint32_t m_connectionId;

    bool m_fatalError;

    Mutex m_lock;

    //--

    SpinLock m_receivedMessagesQueueLock;
    Queue<MessagePtr> m_receivedMessagesQueue;

    //--

    virtual void sendPtr(const void* messageData, Type messageClass) override final;

    virtual void handleConnectionClosed(socket::tcp::Client* client, const socket::Address& address) override final;
    virtual void handleConnectionData(socket::tcp::Client* client, const socket::Address& address, const void* data, uint32_t dataSize) override final;
};

//---

END_BOOMER_NAMESPACE(base::net)