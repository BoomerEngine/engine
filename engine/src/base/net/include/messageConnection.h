/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages #]
***/

#pragma once

namespace base
{
    namespace net
    {

        //---

        /// high level integration of message system
        /// this class represents a connection to a remote RPC system (but not necerassly the TCP connection)
        /// NOTE: lifetime of this object does not control the connection itself
        /// NOTE: the connection object is virtualized to allow for "virtual" connections: loopback, delay, file, etc
        class BASE_NET_API MessageConnection : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(MessageConnection, IObject);

        public:
            MessageConnection();
            virtual ~MessageConnection();

            ///---

            /// get the ID of the connection as seen by the owner
            virtual uint32_t connectionId() const = 0;

            /// get local address
            /// NOTE: does NOT have to be a network address
            virtual StringBuf localAddress() const = 0;

            /// get remote address
            /// NOTE: does NOT have to be a network address
            /// NOTE: in principle it should be possible to use this address to reconnect the connection
            virtual StringBuf remoteAddress() const = 0;

            /// are we still connected ?
            virtual bool isConnected() const = 0;

            ///---

            /// send message over the connection
            template< typename T >
            INLINE void send(uint32_t targetObjectId, const T& messageData)
            {
                sendPtr(targetObjectId, &messageData, reflection::GetTypeObject<T>());
            }

        protected:
            /// send message over the connection
            virtual void sendPtr(uint32_t targetObjectId, const void* messageData, Type messageClass) = 0;
        };

        //---;

    } // net
} // base