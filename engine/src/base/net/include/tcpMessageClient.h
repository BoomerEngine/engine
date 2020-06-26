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

namespace base
{
    namespace net
    {

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

            /// connect to TCP server
            bool connect(const socket::Address& address);

            ///---

            /// get an unused ID for object registration
            uint32_t allocObjectId();

            /// attach object to object table, it will be able to receive messages if targeted with it's number
            /// NOTE: objects are auto matically  detached when deleted
            void attachObject(uint32_t id, const ObjectPtr& ptr);

            /// detach object from object table, it will no longer receive messages
            void dettachObject(uint32_t id);

            /// get object that we have registered for given ID
            ObjectPtr resolveObject(uint32_t id) const;

            //---

            /// execute all pending messages
            void executePendingMessages();

            //---

        private:
            replication::DataModelRepositoryPtr m_models;
            MessageObjectRepositoryPtr m_objects;

            UniquePtr<MessagePool> m_pool;
            UniquePtr<MessageReplicator> m_replicator;
            UniquePtr<MessageObjectExecutor> m_executor;

            UniquePtr<MessageReassembler> m_reassembler;

            socket::tcp::Client m_client;
            uint32_t m_connectionId;

            bool m_fatalError;

            Mutex m_lock;

            //

            virtual void sendPtr(uint32_t targetObjectId, const void* messageData, Type messageClass) override final;

            virtual void handleConnectionClosed(socket::tcp::Client* client, const socket::Address& address) override final;
            virtual void handleConnectionData(socket::tcp::Client* client, const socket::Address& address, const void* data, uint32_t dataSize) override final;
        };

        //---

    } // net
} // base