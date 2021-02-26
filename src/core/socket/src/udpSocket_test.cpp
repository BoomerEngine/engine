/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: udp #]
***/

#include "build.h"
#include "core/test/include/gtest/gtest.h"

#include "address.h"
#include "udpSocket.h"

DECLARE_TEST_FILE(NewUdpSocket);

BEGIN_BOOMER_NAMESPACE()

TEST(Socket, UdpSocket_IPv4SimpleSendReceive_Local)
{
    socket::Address senderAddress(socket::Address::Local4(1337));
    socket::Address receiverAddress(socket::Address::Local4(1338));

    socket::udp::RawSocket sender;
    bool senderOk = sender.open(senderAddress);
    ASSERT_TRUE(senderOk);
    
    socket::udp::RawSocket receiver;
    bool receiverOk = receiver.open(receiverAddress);
    ASSERT_TRUE(receiverOk);

    uint8_t payload[4] = {'k', 'e', 'k', '\0'};
    size_t bytesSent = sender.send(payload, sizeof(payload), receiverAddress);
    ASSERT_EQ(bytesSent, sizeof(payload));

    socket::Address receiveAddress(socket::AddressType::AddressIPv4);

    uint8_t receiveBuffer[4] = {0};
    auto bytesReceived = receiver.receive(receiveBuffer, sizeof(receiveBuffer), &receiveAddress);
    ASSERT_EQ(sizeof(payload), bytesReceived);
    ASSERT_EQ(0, memcmp(payload, receiveBuffer, sizeof(receiveBuffer)));
    ASSERT_EQ(senderAddress, receiveAddress);
}

#if !defined(FUCKED_UP_NETSTACK)
TEST(Network, UdpSocket_IPv6SimpleSendReceive_Local)
{
    socket::Address senderAddress(socket::Address::Local6(1337));
    socket::Address receiverAddress(socket::Address::Local6(1338));

    socket::udp::RawSocket sender;
    bool senderOk = sender.open(senderAddress);
    ASSERT_TRUE(senderOk);

    socket::udp::RawSocket receiver;
    bool receiverOk = receiver.open(receiverAddress);
    ASSERT_TRUE(receiverOk);

    uint8_t payload[4] = { 'k', 'e', 'k', '\0' };
    size_t bytesSent = sender.send(payload, sizeof(payload), receiverAddress);
    ASSERT_EQ(sizeof(payload), bytesSent);

    socket::Address receiveAddress(socket::AddressType::AddressIPv6);

    uint8_t receiveBuffer[4] = { 0 };
    auto bytesReceived = receiver.receive(receiveBuffer, sizeof(receiveBuffer), &receiveAddress);
    ASSERT_EQ(sizeof(payload), bytesReceived);
    ASSERT_EQ(0, memcmp(payload, receiveBuffer, sizeof(receiveBuffer)));
    ASSERT_EQ(senderAddress, receiveAddress);
}

#endif

END_BOOMER_NAMESPACE()