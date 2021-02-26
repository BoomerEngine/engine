/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(net)

/// return codes used by reassembler
enum class ReassemblerResult : uint8_t
{
    NeedsMore, // we had some data, but not all of it
    Valid, // we have a valid data (header, message, etc)
    Corruption, // corruption of the channel, we should probably close the connection
};

/// reassembler inspection interface
class CORE_NET_API IMessageReassemblerInspector : public NoCopy
{
public:
    virtual ~IMessageReassemblerInspector();

    // try to parse header from the provided data, if we parse header we are expected to know the data size of the WHOLE THING that we still need to receive
    // NOTE: the need to know the data size limits the messages we can reassemble to ones that have size in the header but that's all that we use in the engine (so far)
    virtual ReassemblerResult tryParseHeader(const uint8_t* currentData, uint32_t currentDataSize, uint32_t& outTotalMessageSize) const = 0;

    // try to accept the message, called only after we gathered all the data as indicated by the previously parsed header,
    // NOTE: specified size passed here always matches the size reported by header
    // NOTE: main thing to do is to validate CRC (if we had one)
    virtual ReassemblerResult tryParseMessage(const uint8_t* messageData, uint32_t messageDataSize) const = 0;
};

/// helper class for reassembling data coming from the network stream (usually TCP) into packets/messages
class CORE_NET_API MessageReassembler : public NoCopy
{
public:
    MessageReassembler(IMessageReassemblerInspector* inspector, uint32_t initialStorageSize = 1024, uint32_t maxStorageSize = 100U << 20, uint32_t maxHeaderSize = 10 << 10);
    ~MessageReassembler();

    /// push new data into storage
    /// NOTE: pushing to much data without any active header will cause a channel corruption situation
    /// NOTE: returns false on serious errors
    bool pushData(const void* data, uint32_t dataSize);

    /// process the currently stored data, try to assemble messages of of them
    /// If we have a valid message a ReassemblerResult::Valid will be returned with the message data passed via outMessageData/outMessageSize
    /// ReassemblerResult::NeedsMore is returned if we need more data to do something
    /// Watch out for ReassemblerResult::Corruption that will be called if we have ANY problems with the connection
    ReassemblerResult reassemble(const uint8_t*& outMessageData, uint32_t& outMessageSize);

private:
    IMessageReassemblerInspector* m_inspector;

    // temporary storage for message data while they are being reassembled
    // NOTE: due to the nature of TCP or a PIPE (that is usually sourcing data here) we may have multiple messages added here
    uint8_t* m_storagePtr;
    uint32_t m_storageCapacity;
    uint32_t m_storagePos;

    // if known this is the expected size of the message we are waiting for
    uint32_t m_expectedMessageSize;

    // reading position for the message parsing
    uint32_t m_readPos;

    // did we get corrupted ?
    bool m_corrupted;

    // setup
    const uint32_t m_maxStorageSize;
    const uint32_t m_maxHeaderSize;

    // put us in the error state
    void fatalError(StringView reason);
};

END_BOOMER_NAMESPACE_EX(net)
