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
#include "messageObjectRepository.h"
#include "messageObjectExecutor.h"
#include "messageReplicator.h"
#include "tcpMessageInternal.h"
#include "tcpMessageServer.h"

#include "base/socket/include/tcpServer.h"
#include "base/socket/include/tcpClient.h"
#include "base/replication/include/replicationDataModelRepository.h"

namespace base
{
    namespace net
    {
        //--

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

        void TcpMessageServerConnection::sendPtr(uint32_t targetObjectId, const void* messageData, Type messageClass)
        {
            if (auto server = m_server.lock())
                return server->send(m_id, targetObjectId, messageData, messageClass);
        }

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(TcpMessageServerConnection);
        RTTI_END_TYPE();

        //--

        TcpMessageServerConnectionState::TcpMessageServerConnectionState(const socket::ConnectionID id, const replication::DataModelRepositoryPtr& sharedModelRepository, const MessageObjectRepositoryPtr& sharedObjectRepository, const socket::Address& address)
            : m_id(id)
            , m_address(address)
            , m_replicator(sharedModelRepository, sharedObjectRepository)
            , m_executor(MessageConnection::GetStaticClass())
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

        TcpMessageExecutorForwarder::TcpMessageExecutorForwarder(MessageObjectExecutor& executor, MessagePool& pool, MessageObjectRepository& objects)
            : m_executor(executor)
            , m_objects(objects)
            , m_pool(pool)
        {}

        Message* TcpMessageExecutorForwarder::allocateMessage(const replication::DataMappedID targetObjectId, Type dataType)
        {
            // don't bother decoding if the target object does not exist
            auto targetObject = m_objects.resolveObject(targetObjectId);
            if (!targetObject)
                return nullptr;

            // don't bother decoding if we don't support the message on the target object
            if (!m_executor.checkMessageSupport(targetObject->cls(), dataType))
                return nullptr;

            return m_pool.allocate(dataType, targetObject);
        }

        void TcpMessageExecutorForwarder::dispatchMessageForExecution(Message* message)
        {
            m_executor.queueMessage(message);
        }

        //--

    } // net
} // base