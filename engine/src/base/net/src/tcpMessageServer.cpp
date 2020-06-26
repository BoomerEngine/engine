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
#include "tcpMessageServer.h"
#include "tcpMessageInternal.h"

#include "base/replication/include/replicationDataModelRepository.h"

namespace base
{
    namespace net
    {

        //--

        RTTI_BEGIN_TYPE_CLASS(TcpMessageServer);
        RTTI_END_TYPE();

        TcpMessageServer::TcpMessageServer()
            : m_server(this)
        {
            m_objects = CreateSharedPtr<MessageObjectRepository>();
            m_models = CreateSharedPtr<replication::DataModelRepository>();

            m_pool.create();
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

        ///---

        uint32_t TcpMessageServer::allocObjectId()
        {
            return m_objects->allocateObjectId();
        }

        void TcpMessageServer::attachObject(uint32_t id, const ObjectPtr& ptr)
        {
            m_objects->attachObject(id, ptr);
        }

        void TcpMessageServer::dettachObject(uint32_t id)
        {
            m_objects->detachObject(id);
        }

        ObjectPtr TcpMessageServer::resolveObject(uint32_t id) const
        {
            return m_objects->resolveObject(id);
        }

        //--

        bool TcpMessageServer::checkConnectionStatus(socket::ConnectionID id)
        {
            auto lock  = CreateLock(m_activeConnectionsLock);
            return m_activeConnectionMap.contains(id);
        }

        void TcpMessageServer::broadcast(uint32_t targetObjectId, const void* messageData, Type messageClass)
        {
            auto lock  = CreateLock(m_activeConnectionsLock);

            for (auto info  : m_activeConnections)
            {
                // compose a message via the replicator and send the generated data down the TCP link
                TcpMessageServerReplicatorDataSink dataSink(m_server, info->m_id);
                info->m_replicator.send(targetObjectId, messageData, messageClass, &dataSink);
            }
        }

        void TcpMessageServer::send(socket::ConnectionID id, uint32_t targetObjectId, const void* messageData, Type messageClass)
        {
            auto lock  = CreateLock(m_activeConnectionsLock);

            // get live connection
            TcpMessageServerConnectionState* state = nullptr;
            if (m_activeConnectionMap.find(id, state))
            {
                // compose a message via the replicator and send the generated data down the TCP link
                TcpMessageServerReplicatorDataSink dataSink(m_server, id);
                state->m_replicator.send(targetObjectId, messageData, messageClass, &dataSink);
            }
        }

        //--

        void TcpMessageServer::handleConnectionAccepted(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection)
        {
            TRACE_INFO("New connection to message server from '{}'", address);

            auto info  = MemNewPool(POOL_NET, TcpMessageServerConnectionState, connection, m_models, m_objects, address);
            info->m_handler = base::CreateSharedPtr<TcpMessageServerConnection>(this, connection, m_server.address(), address);

            auto lock  = CreateLock(m_activeConnectionsLock);
            m_activeConnections.pushBack(info);
            m_activeConnectionMap[connection] = info;
        }

        void TcpMessageServer::handleConnectionClosed(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection)
        {
            TRACE_INFO("Closed connection to message server from '{}'", address);

            auto lock  = CreateLock(m_activeConnectionsLock);

            TcpMessageServerConnectionState* info = nullptr;
            if (m_activeConnectionMap.find(connection, info))
            {
                m_activeConnections.remove(info);
                m_activeConnectionMap.remove(connection);
                MemDelete(info);
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
                    TRACE_ERROR("TcpMessageServer: Fatal error on message reassembly from '{}'", info->m_address);
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

                            TRACE_INFO("TcpMessageServer: reassembled message, size {} from '{}'", header->m_length, info->m_address);

                            TcpMessageExecutorForwarder executor(info->m_executor, *m_pool, *m_objects);
                            info->m_replicator.processMessageData(messageData + sizeof(TcpMessageTransportHeader), header->m_length - sizeof(TcpMessageTransportHeader), &executor);
                            break;
                        }

                        case ReassemblerResult::Corruption:
                        {
                            TRACE_ERROR("TcpMessageServer: Fatal error on message reassembly from '{}'", info->m_address);
                            info->m_fatalError = true;
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

        void TcpMessageServer::executePendingMessages()
        {
            auto lock = CreateLock(m_activeConnectionsLock);

            // TODO: this MAY crash if

            for (auto info  : m_activeConnections)
                info->m_executor.executeQueuedMessges(info->m_handler);
        }

        //--

} // net
} // base