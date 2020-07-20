/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages\tcp #]
***/

#pragma once

#include "messageConnection.h"
#include "messagePool.h"
#include "messageReplicator.h"
#include "messageReassembler.h"

#include "base/socket/include/address.h"
#include "base/replication/include/replicationDataModelRepository.h"
#include "base/containers/include/queue.h"

namespace base
{
    namespace net
    {
        //--

        struct TcpMessageTransportHeader
        {
            uint16_t m_magic = 0;
            uint16_t m_checksum = 0;
            uint32_t m_length = 0;
        };

        //--

        class TcpMessageServerConnection : public MessageConnection
        {
            RTTI_DECLARE_VIRTUAL_CLASS(TcpMessageServerConnection, MessageConnection);

        public:
            TcpMessageServerConnection(TcpMessageServer* server, const socket::ConnectionID id, const socket::Address& localAddress, const socket::Address& remoteAddress);

            virtual uint32_t connectionId() const override final;
            virtual StringBuf localAddress() const override final;
            virtual StringBuf remoteAddress() const override final;
            virtual bool isConnected() const override final;
            virtual void close() override final;

            virtual void sendPtr(const void* messageData, Type messageClass) override final;

            virtual MessagePtr pullNextMessage() override final;

            void pushNextMessage(const MessagePtr& message);

        private:
            RefWeakPtr<TcpMessageServer> m_server;

            SpinLock m_receivedMessagesQueueLock;
            Queue<MessagePtr> m_receivedMessagesQueue;

            socket::ConnectionID m_id;
            socket::Address m_localAddress;
            socket::Address m_remoteAddress;
        };

        //--

        class TcpMessageReassemblerHandler : public net::IMessageReassemblerInspector
        {
        public:
            TcpMessageReassemblerHandler();
            virtual net::ReassemblerResult tryParseHeader(const uint8_t* currentData, uint32_t currentDataSize, uint32_t& outTotalMessageSize) const override final;
            virtual net::ReassemblerResult tryParseMessage(const uint8_t* messageData, uint32_t messageDataSize) const override final;

            static TcpMessageReassemblerHandler& GetInstance();
        };

        //--

        struct TcpMessageServerConnectionState : public NoCopy
        {
            socket::ConnectionID m_id;
            socket::Address m_address;
            MessageReplicator m_replicator;
            MessageReassembler m_reassembler;
            MessageConnectionPtr m_handler;
            bool m_fatalError = false;

            TcpMessageServerConnectionState(const socket::ConnectionID id, const replication::DataModelRepositoryPtr& sharedModelRepository, const socket::Address& address);
        };

        //--

        class TcpMessageServerReplicatorDataSink : public IMessageReplicatorDataSink
        {
        public:
            TcpMessageServerReplicatorDataSink(socket::tcp::Server& server, const socket::ConnectionID id);

            virtual void sendMessage(const void* data, uint32_t size) override final;

        private:
            socket::tcp::Server& m_server;
            socket::ConnectionID m_id;
        };

        //--

        class TcpMessageClientReplicatorDataSink : public IMessageReplicatorDataSink
        {
        public:
            TcpMessageClientReplicatorDataSink(socket::tcp::Client& client);

            virtual void sendMessage(const void* data, uint32_t size) override final;

        private:
            socket::tcp::Client& m_client;
        };

        //--

        class TcpMessageQueueCollector : public IMessageReplicatorDispatcher
        {
        public:
            TcpMessageQueueCollector(TcpMessageServerConnection* connection);
            TcpMessageQueueCollector(TcpMessageClient* client);

            virtual void dispatchMessageForExecution(Message* message) override final;

        private:
            TcpMessageServerConnection* m_connection = nullptr;
            TcpMessageClient* m_client = nullptr;
        };

        //--

    } // net
} // base