/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages\tcp #]
***/

#pragma once

#include "base/socket/include/tcpServer.h"

namespace base
{
    namespace net
    {

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

            /// get an unused ID for object registration
            uint32_t allocObjectId();

            /// attach object to object table, it will be able to receive messages if targeted with it's number
            /// NOTE: objects are auto matically  detached when deleted
            void attachObject(uint32_t id, const ObjectPtr& ptr);

            /// detach object from object table, it will no longer receive messages
            void dettachObject(uint32_t id);

            /// get object that we have registered for given ID
            ObjectPtr resolveObject(uint32_t id) const;

            ///---

            /// broadcast message to a given object over all connected connections
            template< typename T >
            INLINE void broadcast(uint32_t targetObjectId, const T& messageData)
            {
                broadcast(targetObjectId, &messageData, reflection::GetTypeObject<T>());
            }

            //---

            /// execute all pending messages
            void executePendingMessages();

        private:
            replication::DataModelRepositoryPtr m_models;
            MessageObjectRepositoryPtr m_objects;

            UniquePtr<MessagePool> m_pool;

            socket::tcp::Server m_server;

            Array<TcpMessageServerConnectionState*> m_activeConnections;
            HashMap<socket::ConnectionID, TcpMessageServerConnectionState*> m_activeConnectionMap;
            Mutex m_activeConnectionsLock;

            friend class TcpMessageServerConnection;

            //

            bool checkConnectionStatus(socket::ConnectionID id);

            void broadcast(uint32_t targetObjectId, const void* messageData, Type messageClass);
            void send(socket::ConnectionID, uint32_t targetObjectId, const void* messageData, Type messageClass);

            virtual void handleConnectionAccepted(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection) override final;
            virtual void handleConnectionClosed(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection) override final;
            virtual void handleConnectionData(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection, const void* data, uint32_t dataSize) override final;
            virtual void handleServerClose(socket::tcp::Server* server) override final;

        };

    } // net
} // base