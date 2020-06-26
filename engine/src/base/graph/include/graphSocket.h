/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#pragma once

#include "graphBlock.h"

namespace base
{
    namespace graph
    {
        // socket in block
        class BASE_GRAPH_API Socket : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Socket, IObject);
            
        public:
            Socket();
            Socket(StringID name, const BlockSocketStyleInfo& info);
            virtual ~Socket();

            //--

            // get the parent block
            Block* block() const;

            //--

            // get name of the socket
            INLINE StringID name() const { return m_name; }

            // get socket layout information
            INLINE const BlockSocketStyleInfo& info() const { return m_info; }

            //--
            
            // do we have any connections ?
            INLINE bool hasConnections() const { return !m_connections.empty(); }

            // get number of connections
            INLINE uint32_t numConnections() const { return m_connections.size(); }

            // get connections (read only)
            typedef Array< ConnectionPtr > TConnections;
            INLINE const TConnections& connections() const { return m_connections; }

            //---

            /// update socket information (placement, color, etc)
            void updateSocketInfo(const BlockSocketStyleInfo& info);

            //---

            /// remove all connections on this socket
            void removeAllConnections();

            /// remove all connections to given socket
            void removeAllConnectionsToSocket(const Socket* to);

            /// remove all connections to given block
            void removeAllConnectionsToBlock(const Block* to);

            /// are we connected to given socket ?
            bool hasConnectionsToSocket(const Socket* to) const;

            /// are we connected to given block ?
            bool hasConnectionsToBlock(const Block* to) const;

            /// try to connect to given socket (NOTE: creates symmetrical connection in target socket as well)
            /// returns true if connection was successful or false if it was not
            bool connectTo(Socket* to);

            //---

            /// can we connect two sockets ?
            /// NOTE: this is directional check
            static bool CanConnect(const Socket& from, const Socket& to, const base::Array<Connection*>* removedConnections=nullptr);

        private:
            StringID  m_name;
            TConnections m_connections;
            BlockSocketStyleInfo m_info;

            //--

            void notifyConnectionsChanged();

            //--

            static bool CheckDirections(const Socket& a, const Socket& b);
            static bool CheckTags(const Socket& a, const Socket& b);
            static bool CheckDuplicates(const Socket& a, const Socket& b, const base::Array<Connection*>* removedConnections);
            static bool CheckMulticonnections(const Socket& a, const base::Array<Connection*>* removedConnections);

            friend class Connection;
        };

    } // graph
} // base