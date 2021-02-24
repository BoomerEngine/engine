/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: protocol #]
***/

#include "build.h"
#include "block.h"
#include "udpPacket.h"

BEGIN_BOOMER_NAMESPACE(base::socket::udp)

//--

static_assert(sizeof(PacketHeader) == 4, "Size of PacketHeader has to be 4 bytes");
static_assert(sizeof(DataPacketHeader) == 16, "Size of PacketHeader has to be 16 bytes");
static_assert(sizeof(DataAcknowledgeHeader) == 4, "Size of PacketHeader has to be 4 bytes");

Packet::Packet(Block* block, const uint8_t* data, const Address& address)
    : m_block(block)
    , m_ptr(data)
    , m_address(address)
{}

uint32_t Packet::calcTotalSize() const
{
    uint32_t size = sizeof(PacketHeader);

    if ((PacketType)header().type == PacketType::Data)
    {
        size += sizeof(DataPacketHeader);
        size += dataHeader().dataSize;
    }

    return size;
}

void Packet::print(IFormatStream& f) const
{
    bool valid = true;
    auto type = (PacketType) header().type;
    switch (type)
    {
        case PacketType::Connect: f << "Connect"; break;
        case PacketType::Disconnect: f << "Disconnect"; break;
        case PacketType::Acknowledge: f << "Acknowledge"; break;
        case PacketType::Data: f << "Data"; break;
        case PacketType::DataAcknowledge: f << "DataAcknowledge"; break;
        case PacketType::TimeoutProbe: f << "TimeoutProbe"; break;

        default:
            f << "Unknown";
            valid = false;
			break;
    }

    if (valid)
    {
        f.appendf(", checksum={}", Hex(header().checksum));
        f.appendf(", from={}", m_address);

        if (type == PacketType::Data)
        {
            f.appendf(", id={}", dataHeader().id);
            f.appendf(", dataSize={}", dataHeader().dataSize);
            f.appendf(", totalSize={}", dataHeader().totalSize);
            f.appendf(", fragIndex={}", dataHeader().fragmentIndex);
            f.appendf(", seq={}", dataHeader().sequenceNumber);
        }
        else if (type == PacketType::DataAcknowledge)
        {
            f.appendf(", fragIndex={}", ackHeader().fragmentIndex);
            f.appendf(", seq={}", ackHeader().sequenceNumber);
        }
    }
}

Block* Packet::mutateToBlock()
{
    auto dataSize = dataHeader().dataSize;
    m_block->shrink(sizeof(PacketHeader) + sizeof(DataPacketHeader), dataSize);
    return m_block;
}

void Packet::release()
{
    m_block->release();
}

//--

END_BOOMER_NAMESPACE(base::socket::udp)