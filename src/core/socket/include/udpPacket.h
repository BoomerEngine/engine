/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: udp #]
***/

#pragma once

#include "address.h"

BEGIN_BOOMER_NAMESPACE_EX(socket::udp)

//--

// encoded packet type for the UDP layer
enum class PacketType : uint8_t
{
    // Default value.
    Unknown = 0,

    // Connection request from client to server
    Connect = 1,

    // Disconnection notification. Sent as courtesy to the other peer, connection can be severed without notice
    Disconnect = 2,

    // Confirmation packet for connection request
    Acknowledge = 3,

    // Application-specific payload
    Data = 4,

    // Confirmation packet for data fragment
    DataAcknowledge = 5,

    // This packet is only sent internally to unblock waiting fibers
    TimeoutProbe = 6
};

//--

// packed header for the UDP packet
struct PacketHeader
{
    uint32_t type: 8;
    uint32_t checksum: 24;

    INLINE bool checksumValid(uint32_t checksum) const
    {
        return checksum == (checksum % (1 << 24));
    }
};

//--

// header for the UDP data packet (internal)
struct DataPacketHeader
{
    uint32_t id = 0;
    uint16_t sequenceNumber = 0;
    uint16_t fragmentIndex = 0;
    uint16_t dataSize = 0;
    uint16_t padding0 = 0;
    uint32_t totalSize = 0;
};

struct DataAcknowledgeHeader
{
    uint16_t sequenceNumber = 0;
    uint16_t fragmentIndex = 0;
};

//--

// UDP packet, created out of a block
class CORE_SOCKET_API Packet : public NoCopy
{
public:
    Packet(Block* block, const uint8_t* data, const Address& address);

    // get packet header
    INLINE const PacketHeader& header() const;

    // get data packet header
    // NOTE: valid only for data packets
    INLINE const DataPacketHeader& dataHeader() const;

    // get the ack header
    INLINE const DataAcknowledgeHeader& ackHeader() const;

    // get data payload
    // NOTE: valid only for data packets
    INLINE const uint8_t* data() const;

    // get the address this packet was sent from
    INLINE const Address& address() const { return m_address; }

    //--

    // release packet (releases the owned block that packed is part of)
    void release();

    // debug dump
    void print(IFormatStream& f) const;

    //--

    // extract as block
    Block* mutateToBlock();

    // calculate expected packet size with all header
    uint32_t calcTotalSize() const;

private:
    Block* m_block;
    const uint8_t* m_ptr;
    Address m_address;
};

//--

INLINE const PacketHeader& Packet::header() const
{
    return *(const PacketHeader*) m_ptr;
}

INLINE const DataPacketHeader& Packet::dataHeader() const
{
    ASSERT((PacketType)header().type == PacketType::Data);
    return *(const DataPacketHeader*)(m_ptr + sizeof(PacketHeader));
}

INLINE const DataAcknowledgeHeader& Packet::ackHeader() const
{
    ASSERT((PacketType)header().type == PacketType::DataAcknowledge);
    return *(const DataAcknowledgeHeader*)(m_ptr + sizeof(PacketHeader));
}

INLINE const uint8_t* Packet::data() const
{
    ASSERT((PacketType)header().type == PacketType::Data);
    return m_ptr + sizeof(PacketHeader) + sizeof(DataPacketHeader);
}

//--

END_BOOMER_NAMESPACE_EX(socket::udp)
