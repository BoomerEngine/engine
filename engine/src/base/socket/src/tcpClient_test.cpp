/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tcp #]
***/

#include "build.h"
#include "tcpClient.h"
#include "tcpServer.h"
#include "address.h"

#include "base/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(TcpClientTest);

using namespace base;
using namespace base::socket;

namespace test
{
    static uint16_t GetNextTestPort()
    {
        static uint16_t port = 2700;
        return port++;
    }

    class EchoServer : public tcp::IServerHandler
    {
    public:
        EchoServer()
            : m_server(this)
        {}

        bool init(Address& outAddress)
        {
			for (uint32_t i=0; i<100; ++i)
			{
				uint16_t port = GetNextTestPort();
				outAddress = Address::Local4(port);
				if (m_server.init(outAddress))
					return true;
			}

			return false;
        }

    private:
        virtual void handleConnectionAccepted(tcp::Server* server, const Address& address, ConnectionID connection) override
        {
            TRACE_INFO("Got connection from '{}'", address);
        }

        virtual void handleConnectionClosed(tcp::Server* server, const Address& address, ConnectionID connection) override
        {
            TRACE_INFO("Closed connection to '{}'", address);
        }

        virtual void handleServerClose(tcp::Server* server) override {};

        virtual void handleConnectionData(tcp::Server* server, const Address& address, ConnectionID connection, const void* data, uint32_t dataSize)
        {
            // send back
            server->send(connection, data, dataSize);
        }

        tcp::Server m_server;
    };

    class TestReciver : public tcp::IClientHandler
    {
    public:
        TestReciver()
        {}

        bool m_gotClosed = false;
        Array<uint8_t> m_dataReceived;

        virtual void handleConnectionClosed(tcp::Client* client, const Address& address) override final
        {
            TRACE_INFO("Got disconnected from '{}'", address);
            m_gotClosed = true;
        }

        virtual void handleConnectionData(tcp::Client* client, const Address& address, const void* data, uint32_t dataSize) override final
        {
            TRACE_INFO("Got {} bytes of data from '{}'", dataSize, address);
            auto target  = m_dataReceived.allocateUninitialized(dataSize);
            memcpy(target, data, dataSize);
        }
    };
}

TEST(TcpClient, ConnectToPingService)
{
    auto maxLogSize = 20;

    // create echo server
    test::EchoServer echoServer;
    Address echoServerAddress;
    ASSERT_TRUE(echoServer.init(echoServerAddress));

    // prepare payload
    Array<uint8_t> echoBytes;
    echoBytes.resize(1U << maxLogSize);
    for (uint32_t i=0; i<echoBytes.dataSize(); ++i)
        echoBytes[i] = 32 + rand() % 60;

    // send payloads of different sizes
    for (uint32_t payloadLogSize=10; payloadLogSize<=maxLogSize; ++payloadLogSize)
    {
        auto size = 1U << payloadLogSize;
        TRACE_INFO("Testing payload size Log{}: {}", payloadLogSize, size);

        // connect to echo server
        test::TestReciver copyData;
        tcp::ClientConfig clientConfig;
        clientConfig.recvBufferSize = 1 << 20;
        tcp::Client client(&copyData, clientConfig);
        ASSERT_TRUE(client.connect(echoServerAddress));

        // send data
        client.send(echoBytes.data(), size);

        // wait
        Sleep(800);

        // make sure we got it
        ASSERT_FALSE(copyData.m_gotClosed);
        ASSERT_EQ(size, copyData.m_dataReceived.size());
        for (uint32_t i=0; i<size; ++i)
            ASSERT_EQ(echoBytes[i], copyData.m_dataReceived[i]);
    }
}
