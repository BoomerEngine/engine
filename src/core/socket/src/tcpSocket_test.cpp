/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: raw #]
***/

#include "build.h"
#include "core/test/include/gtest/gtest.h"
#include "core/fibers/include/fiberSystem.h"
#include "core/fibers/include/fiberWaitList.h"
#include "core/system/include/thread.h"
#include "core/system/include/timing.h"

#include "address.h"
#include "tcpSocket.h"

DECLARE_TEST_FILE(NewTcpSocket);

BEGIN_BOOMER_NAMESPACE()

template< uint32_t N >
struct DataBlock
{
    uint8_t data[N];

    DataBlock()
    {
        for (uint32_t i=0; i<N; ++i)
            data[i] = rand();
    }

    operator void*()
    {
        return data;
    }

    uint32_t size() const
    {
        return N;
    }

    bool operator==(const DataBlock<N>& other) const
    {
        return 0 == memcmp(data, other.data, N);
    }
};

enum class AddressType : uint8_t
{
    Local,
    Any,
};

struct BlockingTcpWrapper
{
    socket::tcp::RawSocket sock;
    socket::Address address;

    void block_connect(const socket::Address& remoteAddress)
    {
        address = socket::Address::Any4(0);

        auto ok = sock.connect(remoteAddress, &address);
        ASSERT_TRUE(ok);
        TRACE_INFO("TCP Test: Connected to {}, local address {}", remoteAddress, address);
    }

    void block_listen()
    {
        address = socket::Address::Local4(m_portBase++);

        bool ok = sock.listen(address);
        ASSERT_TRUE(ok);
        TRACE_INFO("TCP Test: Bound to {}", address);
    }

    void block_send(void* data, uint32_t size, double timeout = 2.0, bool shouldTimeout = false)
    {
        auto maxTimeout = NativeTimePoint::Now() + timeout;

        auto readPtr  = (const uint8_t* ) data;
        uint32_t left = size;
        while (left && !maxTimeout.reached())
        {
            auto sent = sock.send(readPtr, left);
            if (sent < 0)
            {
                TRACE_INFO("TCP Test: Socket send errored");
                break;
            }

            ASSERT_LE(sent, left);

            if (sent == 0)
            {
                TRACE_INFO("TCP Test: Socket sent blocked");
                Sleep(100);
            }
            else
            {
                if (sent != left)
                    TRACE_INFO("TCP Test: Partial send of {}", sent);

                readPtr += sent;
                left -= sent;
            }
        }

        if (left == 0)
        {
            TRACE_INFO("Sent '{}' bytes to '{}'", size, address);
            ASSERT_FALSE(shouldTimeout);
        }
        else
        {
            ASSERT_TRUE(shouldTimeout);
        }
    }

    void block_recv(void* outData, uint32_t size, double timeout = 2.0, bool shouldTimeout = false)
    {
        auto maxTimeout = NativeTimePoint::Now() + timeout;

        auto readPtr  = (uint8_t* ) outData;
        uint32_t left = size;
        while (left && !maxTimeout.reached())
        {
            auto got = sock.receive(readPtr, left);
            if (got < 0)
            {
                TRACE_INFO("TCP Test: Socket recv errored");
                break;
            }

            ASSERT_LE(got, left);

            if (got == 0)
            {
                TRACE_INFO("TCP Test: Socket recv blocked");
                Sleep(100);
            }
            else
            {
                if (got != left)
                    TRACE_INFO("TCP Test: Partial receive of {}", got);

                readPtr += got;
                left -= got;
            }
        }

        if (left == 0)
        {
            TRACE_INFO("Received '{}' bytes from '{}'", size, address);
            ASSERT_FALSE(shouldTimeout);
        }
        else
        {
            ASSERT_TRUE(shouldTimeout);
        }
    }

    void block_accept(BlockingTcpWrapper& outSocket, double timeout = 2.0)
    {
        auto maxTimeout = NativeTimePoint::Now() + timeout;

        for (;;)
        {
            if (outSocket.sock.accept(sock, &outSocket.address))
            {
                TRACE_INFO("Accepted connection from '{}' on '{}'", outSocket.address, address);
                break;
            }

            TRACE_INFO("TCP Test: Socket accept blocked");
            Sleep(100);
        }
    }

    void async_accept(BlockingTcpWrapper& outSocket, double timeout = 2.0)
    {
        auto fence = Fibers::GetInstance().createCounter("Acceept");
        fences.pushFence(fence);

        RunFiber("ServerAccept") << [this, &outSocket, fence](FIBER_FUNC)
        {
            block_accept(outSocket);
            Fibers::GetInstance().signalCounter(fence);
        };
    }

    void sync()
    {
        fences.sync();
    }

    ~BlockingTcpWrapper()
    {
        sock.close();
        fences.sync();
    }

    fibers::WaitList fences;

    static uint16_t m_portBase;
};

uint16_t BlockingTcpWrapper::m_portBase = 3337;

TEST(Socket, TcpSocket_IPv4_SimpleConnect_Local)
{
    BlockingTcpWrapper server;
    BlockingTcpWrapper client;
    BlockingTcpWrapper serverToClient;

    server.block_listen();
    server.async_accept(serverToClient);
    client.block_connect(server.address);
    server.sync();
}

TEST(Socket, TcpSocket_IPv4_SimpleConnect_SendData)
{
    BlockingTcpWrapper server;
    BlockingTcpWrapper client;
    BlockingTcpWrapper serverToClient;

    server.block_listen();
    server.async_accept(serverToClient);
    client.block_connect(server.address);
    server.sync();

    DataBlock<1024> sendData;
    client .block_send(sendData, sendData.size());

    DataBlock<1024> recvData;
    serverToClient.block_recv(recvData, recvData.size());

    ASSERT_EQ(sendData, recvData);
}

TEST(Socket, TcpSocket_IPv4_SimpleConnect_SendData_ServerToClient)
{
    BlockingTcpWrapper server;
    BlockingTcpWrapper client;
    BlockingTcpWrapper serverToClient;

    server.block_listen();
    server.async_accept(serverToClient);
    client.block_connect(server.address);
    server.sync();

    DataBlock<1024> sendData;
    serverToClient.block_send(sendData, sendData.size());

    DataBlock<1024> recvData;
    client.block_recv(recvData, recvData.size());

    ASSERT_EQ(sendData, recvData);
}

END_BOOMER_NAMESPACE()