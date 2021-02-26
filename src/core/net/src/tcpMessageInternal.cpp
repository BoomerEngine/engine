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
#include "tcpMessageInternal.h"
#include "tcpMessageServer.h"
#include "tcpMessageClient.h"

#include "core/socket/include/tcpServer.h"
#include "core/socket/include/tcpClient.h"
#include "core/replication/include/replicationDataModelRepository.h"

BEGIN_BOOMER_NAMESPACE_EX(net)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(TcpMessageServerConnection);
RTTI_END_TYPE();

TcpMessageServerConnection::TcpMessageServerConnection(TcpMessageServer* server, const socket::ConnectionID id, const socket::Address& localAddress, const socket::Address& remoteAddress)
    : m_id(id)
    , m_localAddress(localAddress)
    , m_remoteAddress(remoteAddress)
    , m_server(AddRef(server))
{}

uint32_t TcpMessageServerConnection::connectionId() const
{
    return m_id;
}

StringBuf TcpMessageServerConnection::localAddress() const
{
    TempString str;
    m_localAddress.print(str);
    return str;
}

StringBuf TcpMessageServerConnection::remoteAddress() const
{
    TempString str;
    m_remoteAddress.print(str);
    return str;
}

bool TcpMessageServerConnection::isConnected() const
{
    if (auto server = m_server.lock())
        return server->checkConnectionStatus(m_id);
    return false;
}

void TcpMessageServerConnection::close()
{
    if (auto server = m_server.lock())
        server->closeConnection(m_id);
}

void TcpMessageServerConnection::sendPtr(const void* messageData, Type messageClass)
{
    if (auto server = m_server.lock())
        return server->send(m_id, messageData, messageClass);
}

MessagePtr TcpMessageServerConnection::pullNextMessage()
{
    auto lock = CreateLock(m_receivedMessagesQueueLock);

    if (m_receivedMessagesQueue.empty())
        return nullptr;

    auto msg = m_receivedMessagesQueue.top();
    m_receivedMessagesQueue.pop();

    return msg;
}

void TcpMessageServerConnection::pushNextMessage(const MessagePtr& message)
{
    if (message)
    {
        auto lock = CreateLock(m_receivedMessagesQueueLock);
        m_receivedMessagesQueue.push(message);
    }
}

//--

TcpMessageServerConnectionState::TcpMessageServerConnectionState(const socket::ConnectionID id, const replication::DataModelRepositoryPtr& sharedModelRepository, const socket::Address& address)
    : m_id(id)
    , m_address(address)
    , m_replicator(sharedModelRepository)
    , m_reassembler(&TcpMessageReassemblerHandler::GetInstance())
{
}

//--

TcpMessageServerReplicatorDataSink::TcpMessageServerReplicatorDataSink(socket::tcp::Server& server, const socket::ConnectionID id)
    : m_server(server)
    , m_id(id)
{}

void TcpMessageServerReplicatorDataSink::sendMessage(const void* data, uint32_t size)
{
    TcpMessageTransportHeader header;
    header.m_magic = 0xF00D;
    header.m_length = sizeof(header) + size;

    CRC32 crc;
    crc.append(data, size);
    header.m_checksum = (uint16_t)crc.crc();
    m_server.send(m_id, &header, sizeof(header));

    m_server.send(m_id, data, size);
}

//--

TcpMessageClientReplicatorDataSink::TcpMessageClientReplicatorDataSink(socket::tcp::Client& client)
    : m_client(client)
{}

void TcpMessageClientReplicatorDataSink::sendMessage(const void* data, uint32_t size)
{
    TcpMessageTransportHeader header;
    header.m_magic = 0xF00D;
    header.m_length = sizeof(header) + size;

    CRC32 crc;
    crc.append(data, size);
    header.m_checksum = (uint16_t)crc.crc();
    m_client.send(&header, sizeof(header));

    m_client.send(data, size);
}

//--

TcpMessageReassemblerHandler::TcpMessageReassemblerHandler()
{}

net::ReassemblerResult TcpMessageReassemblerHandler::tryParseHeader(const uint8_t* currentData, uint32_t currentDataSize, uint32_t& outTotalMessageSize) const
{
    if (currentDataSize < sizeof(TcpMessageTransportHeader))
        return net::ReassemblerResult::NeedsMore;

    auto header  = (const TcpMessageTransportHeader*)  currentData;
    if (header->m_magic != 0xF00D)
        return net::ReassemblerResult::Corruption;

    outTotalMessageSize = header->m_length;
    return net::ReassemblerResult::Valid;
}

net::ReassemblerResult TcpMessageReassemblerHandler::tryParseMessage(const uint8_t* messageData, uint32_t messageDataSize) const
{
    auto header  = (const TcpMessageTransportHeader*)messageData;

    // TODO: check CRC

    return net::ReassemblerResult::Valid;
}

TcpMessageReassemblerHandler& TcpMessageReassemblerHandler::GetInstance()
{
    static TcpMessageReassemblerHandler theInstance;
    return theInstance;
}

//--

TcpMessageQueueCollector::TcpMessageQueueCollector(TcpMessageServerConnection* connection)
    : m_connection(connection)
{}

TcpMessageQueueCollector::TcpMessageQueueCollector(TcpMessageClient* client)
    : m_client(client)
{}

void TcpMessageQueueCollector::dispatchMessageForExecution(Message* message)
{
    if (m_connection)
        m_connection->pushNextMessage(AddRef(message));

    if (m_client)
        m_client->pushNextMessage(AddRef(message));
}

//--

END_BOOMER_NAMESPACE_EX(net)
