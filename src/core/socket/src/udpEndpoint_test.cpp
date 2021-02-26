/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tcp #]
***/

#include "build.h"
#include "tcpServer.h"

#include "core/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(UdpEndpointTest);

#include "build.h"
#include "block.h"
#include "udpEndpoint.h"

#include "core/test/include/gtest/gtest.h"
#include "core/system/include/thread.h"

DECLARE_TEST_FILE(NetEndPoint);

BEGIN_BOOMER_NAMESPACE_EX(socket)

static socket::udp::EndpointConfig GetConfigForTesting()
{
    socket::udp::EndpointConfig config;
    config.connectionTimeoutMs = 2000;
    config.timeoutProbeIntervalMs = 200;
    config.maxConnectionRetries = 3;
    // config.maxMtu = 200;
    return config;
}

struct EndpointTestEvent
{
    socket::udp::Endpoint* endpoint;
    socket::Address address;
    socket::ConnectionID connection;
    socket::Block* data = nullptr;
};

struct Waiter
{
	Waiter(uint32_t maxMS = 5000)
		: timeLeft(maxMS)
	{}

	bool wait(uint32_t step = 100)
	{
		Sleep(step);

		if (timeLeft <= step)
			return false;

        timeLeft -= step;
		return true;
	}

private:
	uint32_t timeLeft;

};

#define NET_FUNC const socket::Address& address, socket::ConnectionID connection, socket::Block* data

class EndpointTest : public socket::udp::IEndpointHandler
{
public:

    typedef std::function<void(NET_FUNC)> TEventFunc;

    TEventFunc OnConnectionSucceeded;
    TEventFunc OnConnectionClosed;
    TEventFunc OnConnectionRequest;
    TEventFunc OnConnectionData;

    //--

    EndpointTest()
            : m_endpoint(this, GetConfigForTesting())
    {
        m_address = socket::Address::Local4(m_portBase++);

        OnConnectionSucceeded = [](NET_FUNC)
        {
            ASSERT_TRUE(!"Unexpected ConnectionSucceeded");
        };

        OnConnectionClosed = [](NET_FUNC)
        {
            ASSERT_TRUE(!"Unexpected ConnectionClosed");
        };

        OnConnectionRequest = [](NET_FUNC)
        {
            ASSERT_TRUE(!"Unexpected ConnectionRequest");
        };

        OnConnectionData = [](NET_FUNC)
        {
            ASSERT_TRUE(!"Unexpected ConnectionData");
        };
    }

    ~EndpointTest()
    {
        m_endpoint.close();
    }

    void init()
    {
        bool ok = m_endpoint.init(m_address);
        ASSERT_TRUE(ok);
    }


    virtual void handleConnectionSucceeded(socket::udp::Endpoint* endpoint, const socket::Address& address, socket::ConnectionID connection) override
    {
        TRACE_INFO("TEST: Got ConnectionSucceeded from endpoint '{}', address '{}', ID {}", endpoint->address(), address, connection);
        if (OnConnectionSucceeded)
            OnConnectionSucceeded(address, connection, nullptr);
    }

    virtual void handleConnectionClosed(socket::udp::Endpoint* endpoint, const socket::Address& address, socket::ConnectionID connection) override
    {
        TRACE_INFO("TEST: Got ConnectionClosed from endpoint '{}', address '{}', ID {}", endpoint->address(), address, connection);
        if (OnConnectionClosed)
            OnConnectionClosed(address, connection, nullptr);
    }

    virtual void handleConnectionRequest(socket::udp::Endpoint* endpoint, const socket::Address& address, socket::ConnectionID connection) override
    {
        TRACE_INFO("TEST: Got ConnectionRequest from endpoint '{}', address '{}', ID {}", endpoint->address(), address, connection);
        if (OnConnectionRequest)
            OnConnectionRequest(address, connection, nullptr);
    }

    virtual void handleConnectionData(socket::udp::Endpoint* endpoint, const socket::Address& address, socket::ConnectionID connection, socket::Block* dataPayload) override
    {
        TRACE_INFO("TEST: Got ConnectionData from endpoint '{}', address '{}', ID {}, data size {}", endpoint->address(), address, connection, dataPayload->dataSize());
        if (OnConnectionData)
            OnConnectionData(address, connection, dataPayload);
    }

    virtual void handleEndpointError(socket::udp::Endpoint* endpoint) override
    {
        TRACE_INFO("TEST: Got EndpointError from endpoint '{}'", endpoint->address());
        ASSERT_FALSE(!"Endpoint error");
    }

    socket::udp::Endpoint m_endpoint;
    socket::Address m_address;

    static uint16_t m_portBase;
};

uint16_t EndpointTest::m_portBase = 1337;

TEST(UDPEndpointTest, Endpoint_Connect_Local)
{
    EndpointTest serverEndpoint;
    EndpointTest clientEndpoint;

    serverEndpoint.init();
    clientEndpoint.init();

    //-

    bool connectionRequested = false;
    socket::ConnectionID serverConnectionID = 0;
    serverEndpoint.OnConnectionRequest = [&serverEndpoint, &clientEndpoint, &connectionRequested, &serverConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionRequested);
        ASSERT_EQ(0, serverConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(clientEndpoint.m_address, address);
        connectionRequested = true;
        serverConnectionID = connection;
    };

    bool connectionAccepted  = false;
    socket::ConnectionID clientConnectionID = 0;
    clientEndpoint.OnConnectionSucceeded = [&serverEndpoint, &clientEndpoint, &connectionAccepted, &clientConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionAccepted);
        ASSERT_EQ(0, clientConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(serverEndpoint.m_address, address);
        connectionAccepted = true;
        clientConnectionID = connection;
    };

    // connect
    auto clientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(1000);
		while ((!connectionRequested || !connectionAccepted) && waiter.wait()) {}
	}
    ASSERT_TRUE(connectionRequested);
    ASSERT_TRUE(serverConnectionID != 0);
    ASSERT_TRUE(connectionAccepted);
    ASSERT_TRUE(clientConnectionID != 0);
    ASSERT_TRUE(clientConnectionID == clientIDFromConnection);
}

TEST(UDPEndpointTest, Endpoint_Connection_Fails_If_Server_Dead)
{
    EndpointTest serverEndpoint;
    EndpointTest clientEndpoint;

    serverEndpoint.init();
    clientEndpoint.init();

    serverEndpoint.m_endpoint.close();

    //-

    clientEndpoint.OnConnectionSucceeded = [&serverEndpoint, &clientEndpoint](NET_FUNC)
    {
        ASSERT_TRUE(!"Unexpected success");
    };

    socket::ConnectionID closedConnectionID = 0;
    clientEndpoint.OnConnectionClosed = [&serverEndpoint, &clientEndpoint, &closedConnectionID](NET_FUNC)
    {
        ASSERT_EQ(0, closedConnectionID);
        ASSERT_NE(0, connection);
        closedConnectionID = connection;
    };

    // connect
    auto clientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(10000);
		while (!closedConnectionID && waiter.wait()) {};
    }
    ASSERT_TRUE(closedConnectionID != 0);
    ASSERT_TRUE(closedConnectionID == clientIDFromConnection);
}

TEST(UDPEndpointTest, Endpoint_Connect_ClientToServerDisconnect)
{
    EndpointTest serverEndpoint;
    EndpointTest clientEndpoint;

    serverEndpoint.init();
    clientEndpoint.init();

    //-

    bool connectionRequested = false;
    socket::ConnectionID serverConnectionID = 0;
    serverEndpoint.OnConnectionRequest = [&serverEndpoint, &clientEndpoint, &connectionRequested, &serverConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionRequested);
        ASSERT_EQ(0, serverConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(clientEndpoint.m_address, address);
        connectionRequested = true;
        serverConnectionID = connection;
    };

    bool connectionAccepted  = false;
    socket::ConnectionID clientConnectionID = 0;
    clientEndpoint.OnConnectionSucceeded = [&serverEndpoint, &clientEndpoint, &connectionAccepted, &clientConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionAccepted);
        ASSERT_EQ(0, clientConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(serverEndpoint.m_address, address);
        connectionAccepted = true;
        clientConnectionID = connection;
    };

    // connect
    auto clientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(1000);
		while ((!connectionRequested || !connectionAccepted) && waiter.wait()) {};
	}
    ASSERT_TRUE(connectionRequested);
    ASSERT_TRUE(serverConnectionID != 0);
    ASSERT_TRUE(connectionAccepted);
    ASSERT_TRUE(clientConnectionID != 0);
    ASSERT_TRUE(clientConnectionID == clientIDFromConnection);

    socket::ConnectionID closedServerConnectionID = 0;
    serverEndpoint.OnConnectionClosed = [&serverEndpoint, &clientEndpoint, &closedServerConnectionID](NET_FUNC)
    {
        ASSERT_EQ(0, closedServerConnectionID);
        ASSERT_NE(0, connection);
        closedServerConnectionID = connection;
    };

    socket::ConnectionID closedClientConnectionID = 0;
    clientEndpoint.OnConnectionClosed = [&serverEndpoint, &clientEndpoint, &closedClientConnectionID](NET_FUNC)
    {
        ASSERT_EQ(0, closedClientConnectionID);
        ASSERT_NE(0, connection);
        closedClientConnectionID = connection;
    };

    auto ok = clientEndpoint.m_endpoint.disconnect(clientConnectionID);
    ASSERT_TRUE(ok);

	{
		Waiter waiter(1000);
		while ((!closedServerConnectionID || !closedClientConnectionID) && waiter.wait()) {};
    }

    ASSERT_TRUE(closedServerConnectionID == serverConnectionID);
    ASSERT_TRUE(closedClientConnectionID == clientIDFromConnection);
}

TEST(UDPEndpointTest, Endpoint_Connect_ServerToClientDisconnect)
{
    EndpointTest serverEndpoint;
    EndpointTest clientEndpoint;

    serverEndpoint.init();
    clientEndpoint.init();

    //-

    bool connectionRequested = false;
    socket::ConnectionID serverConnectionID = 0;
    serverEndpoint.OnConnectionRequest = [&serverEndpoint, &clientEndpoint, &connectionRequested, &serverConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionRequested);
        ASSERT_EQ(0, serverConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(clientEndpoint.m_address, address);
        connectionRequested = true;
        serverConnectionID = connection;
    };

    bool connectionAccepted  = false;
    socket::ConnectionID clientConnectionID = 0;
    clientEndpoint.OnConnectionSucceeded = [&serverEndpoint, &clientEndpoint, &connectionAccepted, &clientConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionAccepted);
        ASSERT_EQ(0, clientConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(serverEndpoint.m_address, address);
        connectionAccepted = true;
        clientConnectionID = connection;
    };

    // connect
    auto clientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(1000);
		while ((!connectionRequested || !connectionAccepted) && waiter.wait()) {};
    }
    ASSERT_TRUE(connectionRequested);
    ASSERT_TRUE(serverConnectionID != 0);
    ASSERT_TRUE(connectionAccepted);
    ASSERT_TRUE(clientConnectionID != 0);
    ASSERT_TRUE(clientConnectionID == clientIDFromConnection);

    socket::ConnectionID closedServerConnectionID = 0;
    serverEndpoint.OnConnectionClosed = [&serverEndpoint, &clientEndpoint, &closedServerConnectionID](NET_FUNC)
    {
        ASSERT_EQ(0, closedServerConnectionID);
        ASSERT_NE(0, connection);
        closedServerConnectionID = connection;
    };

    socket::ConnectionID closedClientConnectionID = 0;
    clientEndpoint.OnConnectionClosed = [&serverEndpoint, &clientEndpoint, &closedClientConnectionID](NET_FUNC)
    {
        ASSERT_EQ(0, closedClientConnectionID);
        ASSERT_NE(0, connection);
        closedClientConnectionID = connection;
    };

    auto ok = serverEndpoint.m_endpoint.disconnect(serverConnectionID);
    ASSERT_TRUE(ok);

	{
		Waiter waiter(1000);
		while ((!closedServerConnectionID || !closedClientConnectionID) && waiter.wait()) {};
    }

    ASSERT_TRUE(closedServerConnectionID == serverConnectionID);
    ASSERT_TRUE(closedClientConnectionID == clientIDFromConnection);
}

TEST(UDPEndpointTest, Endpoint_Send_Small_Data)
{
    EndpointTest serverEndpoint;
    EndpointTest clientEndpoint;

    serverEndpoint.init();
    clientEndpoint.init();

    //-

    bool connectionRequested = false;
    socket::ConnectionID serverConnectionID = 0;
    serverEndpoint.OnConnectionRequest = [&serverEndpoint, &clientEndpoint, &connectionRequested, &serverConnectionID](NET_FUNC)
    {
        connectionRequested = true;
        serverConnectionID = connection;
    };

    bool connectionAccepted  = false;
    socket::ConnectionID clientConnectionID = 0;
    clientEndpoint.OnConnectionSucceeded = [&serverEndpoint, &clientEndpoint, &connectionAccepted, &clientConnectionID](NET_FUNC)
    {
        connectionAccepted = true;
        clientConnectionID = connection;
    };

    // connect
    auto clientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(1000);
		while ((!connectionRequested || !connectionAccepted) && waiter.wait()) {};
    }

    ASSERT_TRUE(connectionRequested);
    ASSERT_TRUE(serverConnectionID != 0);
    ASSERT_TRUE(connectionAccepted);
    ASSERT_TRUE(clientConnectionID != 0);
    ASSERT_TRUE(clientConnectionID == clientIDFromConnection);

    //--

    const char* sendData = "KEK!";

    socket::Block * receivedBlock = nullptr;
    clientEndpoint.OnConnectionData = [&receivedBlock](NET_FUNC)
    {
        ASSERT_TRUE(data != nullptr);
        receivedBlock = data;
    };

    auto ok = serverEndpoint.m_endpoint.send(serverConnectionID, socket::BlockPart(sendData, sizeof(sendData)));
    ASSERT_TRUE(ok);

	{
		Waiter waiter(1000);
		while (!receivedBlock && waiter.wait()) {};
    }

    ASSERT_EQ(receivedBlock->dataSize(), sizeof(sendData));
    ASSERT_EQ(0, memcmp(receivedBlock->data(), sendData, sizeof(sendData)));
    receivedBlock->release();
}

TEST(UDPEndpointTest, Endpoint_Send_LotsOfSmallData)
{
    EndpointTest serverEndpoint;
    EndpointTest clientEndpoint;

    serverEndpoint.init();
    clientEndpoint.init();

    //-

    bool connectionRequested = false;
    socket::ConnectionID serverConnectionID = 0;
    serverEndpoint.OnConnectionRequest = [&serverEndpoint, &clientEndpoint, &connectionRequested, &serverConnectionID](NET_FUNC)
    {
        connectionRequested = true;
        serverConnectionID = connection;
    };

    bool connectionAccepted  = false;
    socket::ConnectionID clientConnectionID = 0;
    clientEndpoint.OnConnectionSucceeded = [&serverEndpoint, &clientEndpoint, &connectionAccepted, &clientConnectionID](NET_FUNC)
    {
        connectionAccepted = true;
        clientConnectionID = connection;
    };

    // connect
	auto clientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(1000);
		while ((!connectionRequested || !connectionAccepted) && waiter.wait()) {};
    }

    ASSERT_TRUE(connectionRequested);
    ASSERT_TRUE(serverConnectionID != 0);
    ASSERT_TRUE(connectionAccepted);
    ASSERT_TRUE(clientConnectionID != 0);
    ASSERT_TRUE(clientConnectionID == clientIDFromConnection);

    //--

    const char* sendData = "KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!KEK!";

    Array<socket::Block*> receivedBlocks;
    clientEndpoint.OnConnectionData = [&receivedBlocks](NET_FUNC)
    {
        ASSERT_TRUE(data != nullptr);
        receivedBlocks.pushBack(data);
    };

    uint32_t numPackets = 100;
    for (uint32_t i=0; i<numPackets; ++i)
    {
        auto ok = serverEndpoint.m_endpoint.send(serverConnectionID, socket::BlockPart(sendData, sizeof(sendData)));
        ASSERT_TRUE(ok);
    }

	{
		Waiter waiter;
		while (receivedBlocks.size() != 100 && waiter.wait()) {};
    }

    for (auto receivedBlock  : receivedBlocks)
    {
        ASSERT_EQ(receivedBlock->dataSize(), sizeof(sendData));
        ASSERT_EQ(0, memcmp(receivedBlock->data(), sendData, sizeof(sendData)));
        receivedBlock->release();
    }
}

TEST(UDPEndpointTest, Endpoint_Send_FragmentedPacket)
{
    EndpointTest serverEndpoint;
    EndpointTest clientEndpoint;

    serverEndpoint.init();
    clientEndpoint.init();

    //-

    bool connectionRequested = false;
    socket::ConnectionID serverConnectionID = 0;
    serverEndpoint.OnConnectionRequest = [&serverEndpoint, &clientEndpoint, &connectionRequested, &serverConnectionID](NET_FUNC)
    {
        connectionRequested = true;
        serverConnectionID = connection;
    };

    bool connectionAccepted  = false;
    socket::ConnectionID clientConnectionID = 0;
    clientEndpoint.OnConnectionSucceeded = [&serverEndpoint, &clientEndpoint, &connectionAccepted, &clientConnectionID](NET_FUNC)
    {
        connectionAccepted = true;
        clientConnectionID = connection;
    };

    // connect
    auto clientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(1000);
		while ((!connectionRequested || !connectionAccepted) && waiter.wait()) {};
    }

    ASSERT_TRUE(connectionRequested);
    ASSERT_TRUE(serverConnectionID != 0);
    ASSERT_TRUE(connectionAccepted);
    ASSERT_TRUE(clientConnectionID != 0);
    ASSERT_TRUE(clientConnectionID == clientIDFromConnection);

    //--

    Array<uint32_t> randomData;
    for (uint32_t i=0; i<2000; ++i)
        randomData.pushBack(i);

    socket::Block * receivedBlock = nullptr;
    clientEndpoint.OnConnectionData = [&receivedBlock](NET_FUNC)
    {
        ASSERT_TRUE(data != nullptr);
        ASSERT_TRUE(receivedBlock == nullptr);
        receivedBlock = data;
    };

    auto ok = serverEndpoint.m_endpoint.send(serverConnectionID, socket::BlockPart(randomData.data(), randomData.dataSize()));
    ASSERT_TRUE(ok);

	{
		Waiter waiter(1000);
		while (!receivedBlock && waiter.wait()) {};
    }

    ASSERT_EQ(receivedBlock->dataSize(), randomData.dataSize());

    auto readPtr  = (const uint32_t*)receivedBlock->data();
    for (uint32_t i=0; i<2000; ++i)
    {
        ASSERT_EQ(i, readPtr[i]);
    }

    ASSERT_EQ(0, memcmp(receivedBlock->data(), randomData.data(), randomData.dataSize()));
    receivedBlock->release();
}

TEST(UDPEndpointTest, Endpoint_Connect_AutoDisconnectOnLackOfPings)
{
    EndpointTest serverEndpoint;
    EndpointTest clientEndpoint;

    serverEndpoint.init();
    clientEndpoint.init();

    //-

    bool connectionRequested = false;
    socket::ConnectionID serverConnectionID = 0;
    serverEndpoint.OnConnectionRequest = [&serverEndpoint, &clientEndpoint, &connectionRequested, &serverConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionRequested);
        ASSERT_EQ(0, serverConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(clientEndpoint.m_address, address);
        connectionRequested = true;
        serverConnectionID = connection;
    };

    bool connectionAccepted  = false;
    socket::ConnectionID clientConnectionID = 0;
    clientEndpoint.OnConnectionSucceeded = [&serverEndpoint, &clientEndpoint, &connectionAccepted, &clientConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionAccepted);
        ASSERT_EQ(0, clientConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(serverEndpoint.m_address, address);
        connectionAccepted = true;
        clientConnectionID = connection;
    };

    // connect
    auto clientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(1000);
		while ((!connectionRequested || !connectionAccepted) && waiter.wait()) {};
    }
    ASSERT_TRUE(connectionRequested);
    ASSERT_TRUE(serverConnectionID != 0);
    ASSERT_TRUE(connectionAccepted);
    ASSERT_TRUE(clientConnectionID != 0);
    ASSERT_TRUE(clientConnectionID == clientIDFromConnection);

    socket::ConnectionID closedServerConnectionID = 0;
    serverEndpoint.OnConnectionClosed = [&serverEndpoint, &clientEndpoint, &closedServerConnectionID](NET_FUNC)
    {
        ASSERT_EQ(0, closedServerConnectionID);
        ASSERT_NE(0, connection);
        closedServerConnectionID = connection;
    };

    // close client endpoint
    clientEndpoint.m_endpoint.close();

    // wait for the system to detect closed connection
	{
		Waiter waiter(10000);
		while (!closedServerConnectionID && waiter.wait()) {};
    }

    ASSERT_TRUE(closedServerConnectionID == serverConnectionID);
}

TEST(UDPEndpointTest, Endpoint_Connect_NormalConnectionNotDisconnected)
{
    EndpointTest serverEndpoint;
    EndpointTest clientEndpoint;

    serverEndpoint.init();
    clientEndpoint.init();

    //-

    bool connectionRequested = false;
    socket::ConnectionID serverConnectionID = 0;
    serverEndpoint.OnConnectionRequest = [&serverEndpoint, &clientEndpoint, &connectionRequested, &serverConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionRequested);
        ASSERT_EQ(0, serverConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(clientEndpoint.m_address, address);
        connectionRequested = true;
        serverConnectionID = connection;
    };

    bool connectionAccepted  = false;
    socket::ConnectionID clientConnectionID = 0;
    clientEndpoint.OnConnectionSucceeded = [&serverEndpoint, &clientEndpoint, &connectionAccepted, &clientConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionAccepted);
        ASSERT_EQ(0, clientConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(serverEndpoint.m_address, address);
        connectionAccepted = true;
        clientConnectionID = connection;
    };

    // connect
    auto clientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(5000);
		while ((!connectionRequested || !connectionAccepted) && waiter.wait()) {};
    }
    ASSERT_TRUE(connectionRequested);
    ASSERT_TRUE(serverConnectionID != 0);
    ASSERT_TRUE(connectionAccepted);
    ASSERT_TRUE(clientConnectionID != 0);
    ASSERT_TRUE(clientConnectionID == clientIDFromConnection);
}


TEST(UDPEndpointTest, Endpoint_Connect_OldSocketReusable)
{
    EndpointTest serverEndpoint;
    EndpointTest clientEndpoint;

    serverEndpoint.init();
    clientEndpoint.init();

    //-

    bool connectionRequested = false;
    socket::ConnectionID serverConnectionID = 0;
    serverEndpoint.OnConnectionRequest = [&serverEndpoint, &clientEndpoint, &connectionRequested, &serverConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionRequested);
        ASSERT_EQ(0, serverConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(clientEndpoint.m_address, address);
        connectionRequested = true;
        serverConnectionID = connection;
    };

    bool connectionAccepted  = false;
    socket::ConnectionID clientConnectionID = 0;
    clientEndpoint.OnConnectionSucceeded = [&serverEndpoint, &clientEndpoint, &connectionAccepted, &clientConnectionID](NET_FUNC)
    {
        ASSERT_FALSE(connectionAccepted);
        ASSERT_EQ(0, clientConnectionID);
        ASSERT_NE(0, connection);
        ASSERT_EQ(serverEndpoint.m_address, address);
        connectionAccepted = true;
        clientConnectionID = connection;
    };

    // connect
    auto clientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(10000);
		while ((!connectionRequested || !connectionAccepted) && waiter.wait()) {};
    }
    ASSERT_TRUE(connectionRequested);
    ASSERT_TRUE(serverConnectionID != 0);
    ASSERT_TRUE(connectionAccepted);
    ASSERT_TRUE(clientConnectionID != 0);
    ASSERT_TRUE(clientConnectionID == clientIDFromConnection);

    socket::ConnectionID closedServerConnectionID = 0;
    serverEndpoint.OnConnectionClosed = [&serverEndpoint, &clientEndpoint, &closedServerConnectionID](NET_FUNC)
    {
        ASSERT_EQ(0, closedServerConnectionID);
        ASSERT_NE(0, connection);
        closedServerConnectionID = connection;
    };

    socket::ConnectionID closedClientConnectionID = 0;
    clientEndpoint.OnConnectionClosed = [&serverEndpoint, &clientEndpoint, &closedClientConnectionID](NET_FUNC)
    {
        ASSERT_EQ(0, closedClientConnectionID);
        ASSERT_NE(0, connection);
        closedClientConnectionID = connection;
    };

    // close the server endpoint
    serverEndpoint.m_endpoint.close();

    // wait for the system to notice closed connection
	{
		Waiter waiter(10000);
		while (!closedClientConnectionID && waiter.wait()) {};
    }

    ASSERT_TRUE(closedClientConnectionID == clientIDFromConnection);

    // reopen the server end
    serverEndpoint.m_endpoint.init(serverEndpoint.m_address);

    // connect again
    connectionAccepted  = false;
    clientConnectionID = 0;
    connectionRequested = false;
    serverConnectionID = 0;

    // connect
    auto newClientIDFromConnection = clientEndpoint.m_endpoint.connect(serverEndpoint.m_address);
	{
		Waiter waiter(10000);
		while ((!connectionRequested || !connectionAccepted) && waiter.wait()) {};
    }
    ASSERT_TRUE(connectionRequested);
    ASSERT_TRUE(serverConnectionID != 0);
    ASSERT_TRUE(connectionAccepted);
    ASSERT_TRUE(clientConnectionID != 0);
    ASSERT_TRUE(clientConnectionID == newClientIDFromConnection);

    closedClientConnectionID = 0;
    closedServerConnectionID = 0;
}

END_BOOMER_NAMESPACE_EX(socket)