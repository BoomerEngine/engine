/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages\tcp #]
***/

#include "build.h"
#include "tcpMessageClient.h"
#include "tcpMessageInternal.h"

#include "messageReassembler.h"
#include "messagePool.h"
#include "messageObjectExecutor.h"
#include "messageReplicator.h"

#include "core/replication/include/replicationDataModelRepository.h"

BEGIN_BOOMER_NAMESPACE_EX(net)

//--

RTTI_BEGIN_TYPE_CLASS(TcpMessageClient);
RTTI_END_TYPE();

TcpMessageClient::TcpMessageClient()
    : m_connectionId(1)
    , m_client(this)
    , m_fatalError(false)
{
    m_models = RefNew<replication::DataModelRepository>();

    m_replicator.create(m_models);
    m_reassembler.create(&TcpMessageReassemblerHandler::GetInstance());
}

TcpMessageClient::~TcpMessageClient()
{}

///---

bool TcpMessageClient::connect(const socket::Address& address)
{
    if (!m_client.connect(address))
        return false;

    m_connectionId += 1;
    m_fatalError = false;
    return true;
}

bool TcpMessageClient::isConnected() const
{
    return m_client.isConnected() && !m_fatalError;
}

void TcpMessageClient::close()
{
    m_client.close();
}

uint32_t TcpMessageClient::connectionId() const
{
    return m_connectionId;
}

StringBuf TcpMessageClient::localAddress() const
{
    TempString str;
    m_client.localAddress().print(str);
    return str;
}

StringBuf TcpMessageClient::remoteAddress() const
{
    TempString str;
    m_client.remoteAddress().print(str);
    return str;
}

///---

void TcpMessageClient::sendPtr(const void* messageData, Type messageClass)
{
    // if we are in error state don't do anything more
    if (m_fatalError)
        return;

    TcpMessageClientReplicatorDataSink dataSink(m_client);
    m_replicator->send(messageData, messageClass, &dataSink);
}


MessagePtr TcpMessageClient::pullNextMessage()
{
    auto lock = CreateLock(m_receivedMessagesQueueLock);

    if (m_receivedMessagesQueue.empty())
        return nullptr;

    auto msg = m_receivedMessagesQueue.top();
    m_receivedMessagesQueue.pop();

    return msg;
}

void TcpMessageClient::pushNextMessage(const MessagePtr& message)
{
    if (message)
    {
        auto lock = CreateLock(m_receivedMessagesQueueLock);
        m_receivedMessagesQueue.push(message);
    }
}

//---

void TcpMessageClient::handleConnectionClosed(socket::tcp::Client* client, const socket::Address& address)
{

}


void TcpMessageClient::handleConnectionData(socket::tcp::Client* client, const socket::Address& address, const void* data, uint32_t dataSize)
{
    // if we are in error state don't do anything more
    if (m_fatalError)
        return;

    // push data to reassembler, we will buffer it until we can assemble a packet
    if (!m_reassembler->pushData(data, dataSize))
    {
        TRACE_ERROR("TcpMessageClient: Fatal error on message reassembly from '{}'", m_client.remoteAddress());
        m_fatalError = true;
        return;
    }

    // process messages
    bool needsMore = true;
    while (needsMore)
    {
        const uint8_t* messageData = nullptr;
        uint32_t messageSize = 0;
        switch (m_reassembler->reassemble(messageData, messageSize))
        {
            case ReassemblerResult::NeedsMore:
            {
                needsMore = false;
                break;
            }

            case ReassemblerResult::Valid:
            {
                auto header  = (const TcpMessageTransportHeader*) messageData;

                TRACE_INFO("TcpMessageClient: reassembled message, size {} from '{}'", header->m_length, m_client.remoteAddress());

                TcpMessageQueueCollector collector(this);
                m_replicator->processMessageData(messageData + sizeof(TcpMessageTransportHeader), header->m_length - sizeof(TcpMessageTransportHeader), &collector);
                break;
            }

            case ReassemblerResult::Corruption:
            {
                TRACE_ERROR("TcpMessageClient: Fatal error on message reassembly from '{}'", m_client.remoteAddress());
                m_fatalError = true;
                break;
            }
        }
    }
}

//---

END_BOOMER_NAMESPACE_EX(net)
