/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages\tcp #]
***/

#include "build.h"
#include "messageConnection.h"
#include "messagePool.h"
#include "messageReplicator.h"
#include "tcpMessageServer.h"
#include "tcpMessageInternal.h"

#include "core/replication/include/replicationDataModelRepository.h"

BEGIN_BOOMER_NAMESPACE_EX(net)

//--

RTTI_BEGIN_TYPE_CLASS(TcpMessageServer);
RTTI_END_TYPE();

TcpMessageServer::TcpMessageServer()
    : m_server(this)
{
    m_models = RefNew<replication::DataModelRepository>();
}

TcpMessageServer::~TcpMessageServer()
{
    stopListening();
}

socket::Address TcpMessageServer::listeningAddress() const
{
    return m_server.address();
}

bool TcpMessageServer::startListening(uint16_t port)
{
    return m_server.init(socket::Address::Any4(port));
}

void TcpMessageServer::stopListening()
{
    m_server.close();

    auto lock  = CreateLock(m_activeConnectionsLock);
    m_activeConnections.clear();
}

bool TcpMessageServer::isListening() const
{
    return m_server.isListening();
}

//--

void TcpMessageServer::closeConnection(socket::ConnectionID id)
{
    auto lock = CreateLock(m_activeConnectionsLock);

    TcpMessageServerConnectionState* info = nullptr;
    if (m_activeConnectionMap.find(id, info))
    {
        TRACE_INFO("TcpMessage: Connection to remote client '{}' closed by server", info->m_address);

        m_activeConnections.removeUnordered(info);
        m_activeConnectionMap.remove(id);
        delete info;

        m_server.disconnect(id);
    }            
}

bool TcpMessageServer::checkConnectionStatus(socket::ConnectionID id)
{
    auto lock  = CreateLock(m_activeConnectionsLock);
    return m_activeConnectionMap.contains(id);
}

void TcpMessageServer::broadcast(const void* messageData, Type messageClass)
{
    auto lock  = CreateLock(m_activeConnectionsLock);

    for (auto info  : m_activeConnections)
    {
        // compose a message via the replicator and send the generated data down the TCP link
        TcpMessageServerReplicatorDataSink dataSink(m_server, info->m_id);
        info->m_replicator.send(messageData, messageClass, &dataSink);
    }
}

void TcpMessageServer::send(socket::ConnectionID id, const void* messageData, Type messageClass)
{
    auto lock = CreateLock(m_activeConnectionsLock);

    // get live connection
    TcpMessageServerConnectionState* state = nullptr;
    if (m_activeConnectionMap.find(id, state))
    {
        StringBuilder messageContent;
        messageClass->printToText(messageContent, messageData);
        TRACE_INFO("TcpMessage: Sending {}: {} via connection {}", messageClass, messageContent, m_id);

        // compose a message via the replicator and send the generated data down the TCP link
        TcpMessageServerReplicatorDataSink dataSink(m_server, id);
        state->m_replicator.send(messageData, messageClass, &dataSink);
    }
    else
    {
        TRACE_WARNING("TcpMessage: Sending {} to closed connection {}", messageClass, id);
    }
}

MessageConnectionPtr TcpMessageServer::pullNextAcceptedConnection()
{
    MessageConnectionPtr ret;

    {
        auto lock = CreateLock(m_activeConnectionsLock);
        if (!m_newConnections.empty())
        {
            ret = m_newConnections.top();
            m_newConnections.pop();

            TRACE_INFO("TcpMessage: Collected new connection from '{}'", ret->remoteAddress());
        }
    }

    return ret;
}

//--

void TcpMessageServer::handleConnectionAccepted(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection)
{
    TRACE_INFO("TcpMessage: New connection to message server from '{}'", address);

    auto info = new TcpMessageServerConnectionState(connection, m_models, address);
    info->m_handler = RefNew<TcpMessageServerConnection>(this, connection, m_server.address(), address);

    {
        auto lock = CreateLock(m_activeConnectionsLock);
        m_activeConnections.pushBack(info);
        m_activeConnectionMap[connection] = info;
    }
        
    {
        auto lock = CreateLock(m_newConnectionsLock);
        m_newConnections.push(info->m_handler);
    }
}

void TcpMessageServer::handleConnectionClosed(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection)
{
    auto lock  = CreateLock(m_activeConnectionsLock);

    TcpMessageServerConnectionState* info = nullptr;
    if (m_activeConnectionMap.find(connection, info))
    {
        TRACE_INFO("TcpMessage: Connection to remote client '{}' closed by client", address);

        m_activeConnections.removeUnordered(info);
        m_activeConnectionMap.remove(connection);
        delete info;
    }
}

void TcpMessageServer::handleConnectionData(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection, const void* data, uint32_t dataSize)
{
    auto lock = CreateLock(m_activeConnectionsLock);

    TcpMessageServerConnectionState* info = nullptr;
    if (m_activeConnectionMap.find(connection, info))
    {
        // if we are in error state don't do anything more
        if (info->m_fatalError)
            return;

        // push data to reassembler, we will buffer it until we can assemble a packet
        if (!info->m_reassembler.pushData(data, dataSize))
        {
            TRACE_ERROR("TcpMessage: Fatal error on message reassembly from '{}'", info->m_address);
            info->m_fatalError = true;
            return;
        }

        // process messages
        bool needsMore = true;
        while (needsMore)
        {
            const uint8_t* messageData = nullptr;
            uint32_t messageSize = 0;
            switch (info->m_reassembler.reassemble(messageData, messageSize))
            {
                case ReassemblerResult::NeedsMore:
                {
                    needsMore = false;
                    break;
                }

                case ReassemblerResult::Valid:
                {
                    auto header  = (const TcpMessageTransportHeader*) messageData;

                    //TRACE_INFO("TcpMessage: reassembled message, size {} from '{}'", header->m_length, info->m_address);

                    TcpMessageQueueCollector executor(static_cast<TcpMessageServerConnection*>(info->m_handler.get()));
                    info->m_replicator.processMessageData(messageData + sizeof(TcpMessageTransportHeader), header->m_length - sizeof(TcpMessageTransportHeader), &executor);
                    break;
                }

                case ReassemblerResult::Corruption:
                {
                    TRACE_ERROR("TcpMessage: Fatal error on message reassembly from '{}'", info->m_address);
                    info->m_fatalError = true;
                    needsMore = false;
                    break;
                }
            }
        }
    }
}

void TcpMessageServer::handleServerClose(socket::tcp::Server* server)
{
    // TODO
}

//--

END_BOOMER_NAMESPACE_EX(net)
