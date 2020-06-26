/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tcp #]
***/

#include "build.h"
#include "tcpServer.h"
#include "address.h"

#include "base/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(TcpServerTest);

using namespace base;
using namespace base::socket;

namespace test
{
    class TestServer : public tcp::IServerHandler
    {
    public:
        TestServer()
                : m_server(this)
        {
            static uint32_t nextPort = 20000;

            auto listenAddress = Address::Any4(nextPort++);
            m_server.init(listenAddress);
        }

        virtual void handleConnectionAccepted(tcp::Server* server, const Address& address, ConnectionID connection) override final
        {
            TRACE_INFO("Got connection from '{}', ID {}", address, connection);
        }

        virtual void handleConnectionClosed(tcp::Server* server, const Address& address, ConnectionID connection) override final
        {
            TRACE_INFO("Lost connection to '{}', ID {}", address, connection);
        }

        virtual void handleConnectionData(tcp::Server* server, const Address& address, ConnectionID connection, const void* data, uint32_t dataSize) override final
        {
            TRACE_INFO("Got data from '{}', ID {}, size {}", address, connection, dataSize);
        }

        virtual void handleServerClose(tcp::Server* server) override final
        {
            TRACE_INFO("Got server close notification");
        }

        //-

        tcp::Server m_server;
    };
}

TEST(TcpServer, StartupTeardown)
{
    test::TestServer server;
    /*for (;;)
    {
        Sleep(100);
    }*/
}
