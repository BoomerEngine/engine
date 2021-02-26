/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tcp #]
***/

#include "build.h"
#include "selector.h"

#include "tcpServer.h"
#include "tcpSocket.h"

#include "core/system/include/thread.h"
#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(socket::tcp)

//--

IServerHandler::~IServerHandler()
{}

//--

void ConnectionStats::print(IFormatStream& f) const
{
    auto aliveTime  = startTime.timeTillNow().toSeconds();

    if (totalDataSent)
        f.appendf("  Data sent: {} ({}/s)\n", MemSize(totalDataSent), MemSize((double)totalDataSent / aliveTime));
    if (totalDataReceived)
        f.appendf("  Data recv: {} ({}/s)", MemSize(totalDataReceived), MemSize((double)totalDataReceived / aliveTime));
}

//--

Server::Server(IServerHandler* handler, const ServerConfig& config /*= ServerConfig()*/)
    : m_handler(handler)
    , m_nextConnectionID(1)
    , m_listeningFlag(0)
    , m_initFlag(0)
    , m_config(config)
{
    m_receciveBuffer.resize(config.recvBufferSize);
}

Server::~Server()
{
    close();
}

bool Server::init(const Address& listenAddress)
{
    // lock
    auto flag  = m_initFlag.exchange(1);
    if (flag != 0)
    {
        TRACE_ERROR("TCP Server: server is already running");
        return true;
    }

    // open socket
    if (!m_socket.listen(listenAddress, &m_address))
    {
        TRACE_ERROR("TCP Server: failed to create socket at address '{}'", listenAddress);
        m_initFlag.exchange(0);
        return false;
    }

    // inform about assigned port
    if (listenAddress.port() == 0)
    {
        TRACE_INFO("TCP Server: Auto assigned socket '{}'", m_address.port());
    }

    // try to set socket in non-blocking mode
    if (!m_socket.blocking(false))
    {
        TRACE_ERROR("TCP Server: failed to set socket into non blocking mode");
        m_initFlag.exchange(0);
        m_socket.close();
        return false;
    }

    // Try to set socket buffer limits
    /*if (!rawSocket->bufferSize(Constants::MAX_DATAGRAM_SIZE, Constants::MAX_DATAGRAM_SIZE))
    {
        TRACE_ERROR("TCP Server error: failed to check buffer sizes on socket");
        rawSocket->close();
        rawSocket.release();
        return false;
    }*/

    // set address
    m_listeningFlag.exchange(1);

    // Create thread processing the data
    ThreadSetup setup;
    setup.m_function = [this]() { threadFunc(); };
    setup.m_priority = ThreadPriority::AboveNormal;
    setup.m_name = "TCPServerThread";

    // Start selector thread
    m_thread.init(setup);

    // Server is alive
    TRACE_INFO("TCP Server: Started local server on '{}'", m_address);
    return true;
}

void Server::close()
{
    // make sure we close only once
    if (1 == m_initFlag.exchange(0))
    {
        TRACE_INFO("TCP Server: Closing local server on '{}'", m_address);

        // close listening socket, this will exit the select() and than the loop
        m_socket.close();

        // close socket processing thread, it should be fine now since we closed the master socket
        m_thread.close();

        // close all connections
        m_activeConnections.clearPtr();
        m_activeConnectionsSocketMap.clear();
        m_activeConnectionsIDMap.clear();
    }
}

bool Server::send(ConnectionID id, const void* data, uint32_t dataSize)
{
    auto lock  = CreateLock(m_activeConnectionsLock);

    // don't send shit via dead server
    if (!m_listeningFlag.load())
        return false;

    // get target address for connection
    Connection* connection = nullptr;
    if (!m_activeConnectionsIDMap.find(id, connection))
        return false;

    // push data and let the system worry
    uint32_t sentLeft = dataSize;
    auto sentPtr  = (const uint8_t*)data;
    while (sentLeft > 0)
    {
        auto sentSize  = connection->rawSocket.send(sentPtr, sentLeft);
        if (sentSize < 0)
        {
            TRACE_ERROR("TCP Server: Failed to send {} bytes to {} ({}), closing", dataSize, id, connection->address);
            connection->closeRequest.exchange(1);
            return false;
        }
        else if (sentSize != (int) dataSize)
        {
            TRACE_WARNING("TCP Server: Truncated send ({} -> {}) when sending {} bytes to {} ({})", sentLeft, sentSize, id, connection->address);
            Sleep(10);
        }

        sentPtr += sentSize;
        sentLeft -= sentSize;
    }

    // update stats
    connection->stats.totalDataSent += dataSize;

    // data sent
    return true;
}

bool Server::disconnect(ConnectionID id)
{
    // dead server
    if (!m_listeningFlag.load())
        return false;

    auto lock  = CreateLock(m_activeConnectionsLock);

    // get target address for connection
    Connection* connection = nullptr;
    if (!m_activeConnectionsIDMap.find(id, connection))
        return false;

    // close the connection
    TRACE_INFO("TCP Server: Closing connection {} ({})", id, connection->address);
    connection->closeRequest.exchange(1);
    return true;
}

//--

void Server::serviceListenerSocket(SocketType socket)
{
    Address remoteAddress;
    RawSocket acceptedSocket;
    if (acceptedSocket.accept(m_socket, &remoteAddress))
    {
        TRACE_INFO("TCP Server: accepted connection from {}, socket: {}", remoteAddress, acceptedSocket.systemSocket());

        acceptedSocket.blocking(false);

        auto con = new Connection;
        con->id = ++m_nextConnectionID;
        con->connectedTime.resetToNow();
        con->lastMessageTime.resetToNow();
        con->address = remoteAddress;
        con->rawSocket = std::move(acceptedSocket);
        con->stats.startTime.resetToNow();

        // add to list of active connections, next time we will loop on the receive thread we will start processing the messages
        {
            auto lock = CreateLock(m_activeConnectionsLock);
            m_activeConnections.pushBack(con);
            m_activeConnectionsIDMap[con->id] = con;
            m_activeConnectionsSocketMap[con->rawSocket.systemSocket()] = con;
        }

        // inform the handler about the new connection
        m_handler->handleConnectionAccepted(this, remoteAddress, con->id);
    }
}

void Server::serviceConnectionSocket(SocketType socket, bool error)
{
    // find connection for given socket
    Connection *con = nullptr;
    {
        auto lock = CreateLock(m_activeConnectionsLock);
        m_activeConnectionsSocketMap.find(socket, con);
    }

    // not found
    if (!con)
    {
        TRACE_ERROR("TCP Server: Got data from unrecognized socket");
        return;
    }

    // close
    if (error)
    {
        TRACE_ERROR("TCP Server: got error for connection {}", con->address);
        con->closeRequest.exchange(1);
        return;
    }

    // get data
    while (1)
    {
        auto dataSize = con->rawSocket.receive(m_receciveBuffer.data(), m_receciveBuffer.dataSize());
        if (dataSize < 0)
        {
            con->closeRequest.exchange(1);
            break;
        }
        else if (dataSize > 0)
        {
            // update stats
            con->stats.totalDataReceived += dataSize;

            // pass to handler
            m_handler->handleConnectionData(this, con->address, con->id, m_receciveBuffer.data(), dataSize);
        }
        else
        {
            // no more data
            break;
        }
    }
}

void Server::threadFunc()
{
    Selector selector;

    InplaceArray<SocketType, 64> activeSockets;

    SocketType listenSocket = m_socket.systemSocket();

    // loop
    bool keepRunning = true;
    while (keepRunning)
    {
        // always put the main socket on the list since we are waiting for the connections
        activeSockets.reset();
        activeSockets.pushBack(listenSocket);

        // process the list of active connections, get list of sockets to look for
        collectActiveConnections(activeSockets);

        // wait for something
        switch (selector.wait(SelectorOp::Read, activeSockets.typedData(), activeSockets.size(), 50))
        {
            case SelectorEvent::Ready:
            {
                // wow, some work
                for (auto& result : selector)
                {
                    if (result.socket == listenSocket)
                    {
                        if (result.error)
                        {
                            TRACE_ERROR("TCP Server: Listener socket closed/lost, existing thread");
                            serviceListenerClose();
                            keepRunning = false;
                        }
                        else
                        {
                            serviceListenerSocket(result.socket);
                        }
                    }
                    else
                    {
                        serviceConnectionSocket(result.socket, result.error);
                    }
                }

                break;
            }

            case SelectorEvent::Busy:
            {
                break;
            }

            case SelectorEvent::Error:
            {
                TRACE_ERROR("TCP Server: Error in poll(), existing server loop");
                serviceListenerClose();
                keepRunning = false;
            }
        }
    }

    // close all active connections
    if (!m_activeConnections.empty())
    {
        TRACE_INFO("TCP server: closing remaining {} connections", m_activeConnections.size());
        m_activeConnections.clearPtr();
        m_activeConnectionsIDMap.clear();
        m_activeConnectionsSocketMap.clear();
    }
}

void Server::collectActiveConnections(Array<SocketType>& outActiveSockets)
{
    auto lock = CreateLock(m_activeConnectionsLock);

    for (int i=m_activeConnections.lastValidIndex(); i >= 0; --i)
    {
        auto con  = m_activeConnections[i];
        ASSERT(con->rawSocket);

        if (con->closeRequest.load())
        {
            TRACE_INFO("TCP Server: Got external request to close connection {} ({}), {}", con->id, con->address, con->rawSocket.systemSocket());
            purgeConnection(con);
        }
        else
        {
            outActiveSockets.pushBack(con->rawSocket.systemSocket());
        }
    }
}

void Server::serviceListenerClose()
{
    if (1 == m_listeningFlag.exchange(0))
        m_handler->handleServerClose(this);
}

void Server::purgeConnection(Connection* con)
{
    // handle notification
    TRACE_INFO("TCP Server: connection '{}' closed\n{}", con->address, con->stats);
    m_handler->handleConnectionClosed(this, con->address, con->id);

    // socket is dead, it's now safe place to delete it
    m_activeConnectionsSocketMap.remove(con->rawSocket.systemSocket());
    m_activeConnectionsIDMap.remove(con->id);
    m_activeConnections.remove(con);

    // close handle
    con->rawSocket.close();

    // cleanup object
    delete con;
}

//--

END_BOOMER_NAMESPACE_EX(socket::tcp)
