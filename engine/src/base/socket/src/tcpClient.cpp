/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tcp #]
***/

#include "build.h"
#include "selector.h"

#include "tcpClient.h"
#include "tcpSocket.h"

#include "base/system/include/thread.h"
#include "base/containers/include/inplaceArray.h"

namespace base
{
    namespace socket
    {
        namespace tcp
        {
            //--

            IClientHandler::~IClientHandler()
            {}

            //--

            Client::Client(IClientHandler* handler, const ClientConfig& config /*= ClientConfig()*/)
                : m_handler(handler)
                , m_config(config)
                , m_connectedFlag(0)
                , m_initFlag(0)
            {
                m_receciveBuffer.resizeWith(config.recvBufferSize, 0);
            }

            Client::~Client()
            {
                close();
            }

            bool Client::connect(const Address& targetAddress)
            {
                // make sure we connect only once
                auto flag = m_initFlag.exchange(1);
                if (flag != 0)
                {
                    TRACE_ERROR("TCP Client: client is already running");
                    return true;
                }

                // connect to target
                Address localAddress;
                if (!m_socket.connect(targetAddress, &localAddress))
                {
                    m_initFlag.exchange(0);
                    TRACE_ERROR("TCP Client: unable to connect to '{}'", targetAddress);
                    return false;
                }

                // use sockets in non blocking mode
                if (!m_socket.blocking(false))
                {
                    m_initFlag.exchange(0);
                    m_socket.close();
                    TRACE_ERROR("TCP Client: unable to change socket mode to unblocking");
                    return false;
                }

                // remember addresses
                m_localAddress = localAddress;
                m_remoteAddress = targetAddress;
                m_connectedFlag.exchange(1);

                // start processing thread
                ThreadSetup setup;
                setup.m_function = [this]() { threadFunc(); };
                setup.m_priority = ThreadPriority::AboveNormal;
                setup.m_name = "TCPClientThread";

                // Start selector thread
                m_thread.init(setup);
                TRACE_INFO("TCP Client: Connected to '{}', local address: {}", targetAddress, localAddress);;
                return true;
            }

            void Client::close()
            {
                // make sure we close only once
                if (1 == m_initFlag.exchange(0))
                {
                    TRACE_INFO("TCP Client: Closing connection to '{}'\n{}", m_remoteAddress, m_stats);

                    // close listening socket, this will exit the select() and than the loop
                    m_socket.close();

                    // close socket processing thread, it should be fine now since we closed the master socket
                    m_thread.close();
                }
            }

            bool Client::send( const void* data, uint32_t dataSize)
            {
                // don't send shit via dead client connection
                if (!m_connectedFlag.load())
                    return false;

                // push data and let the system worry
                uint32_t sentLeft = dataSize;
                auto sentPtr  = (const uint8_t*)data;
                while (sentLeft > 0)
                {
                    auto sentSize = m_socket.send(sentPtr, sentLeft);
                    if (sentSize < 0)
                    {
                        TRACE_ERROR("TCP Client: Failed to send {} bytes to {}, closing", dataSize, m_remoteAddress);
                        m_connectedFlag.exchange(0);
                        return false;
                    }
                    else if (sentSize != (int) dataSize)
                    {
                        TRACE_WARNING("TCP Client: Truncated send ({} -> {}) when sending {} bytes to {}", sentLeft, sentSize, m_remoteAddress);
                        Sleep(10);
                    }

                    sentPtr += sentSize;
                    sentLeft -= sentSize;
                }

                // update stats
                auto lock = base::CreateLock(m_statsLock);
                m_stats.totalDataSent += dataSize;
                return true;
            }

            void Client::stat(ConnectionStats& outStats) const
            {
                auto lock = CreateLock(m_statsLock);
                outStats = m_stats;
            }

            //--

            void Client::handleCloseEvent()
            {
                if (1 == m_connectedFlag.exchange(0))
                    m_handler->handleConnectionClosed(this, m_remoteAddress);
            }

            bool Client::handleIncomingData()
            {
                while (1)
                {
                    auto dataSize = m_socket.receive(m_receciveBuffer.data(), m_receciveBuffer.dataSize());
                    if (dataSize < 0)
                    {
                        TRACE_ERROR("TCP Client: revc() error");
                        handleCloseEvent();
                        return false;
                    }
                    else if (dataSize > 0)
                    {
                        {
                            auto lock = base::CreateLock(m_statsLock);
                            m_stats.totalDataReceived += dataSize;
                        }

                        // pass to handler
                        m_handler->handleConnectionData(this, m_remoteAddress, m_receciveBuffer.data(), dataSize);
                    }
                    else
                    {
                        // no more data
                        break;
                    }
                }

                return true;
            }

            void Client::threadFunc()
            {
                Selector selector;

                SocketType connectionSocket = m_socket.systemSocket();

                // loop
                bool keepRunning = true;
                while (keepRunning)
                {
                    // wait for something
                    switch (selector.wait(SelectorOp::Read, &connectionSocket, 1, m_config.pollTimeout))
                    {
                        case SelectorEvent::Ready:
                        {
                            // wow, some work
                            for (auto& result : selector)
                            {
                                if (result.socket == connectionSocket)
                                {
                                    if (result.error)
                                    {
                                        TRACE_ERROR("TCP Client: Connection closed/lost, existing thread");
                                        handleCloseEvent();
                                        keepRunning = false;
                                    }
                                    else
                                    {
                                        if (!handleIncomingData())
                                            keepRunning = false;
                                    }
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
                            TRACE_ERROR("TCP Client: Error in poll(), existing server loop");
                            m_connectedFlag.exchange(0);
                            keepRunning = false;
                            break;
                        }
                    }
                }
            }

        } // tcp
    } // socket
} // base