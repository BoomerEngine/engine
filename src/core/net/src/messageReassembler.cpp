/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages #]
***/

#include "build.h"
#include "messageReassembler.h"

BEGIN_BOOMER_NAMESPACE_EX(net)

//--

IMessageReassemblerInspector::~IMessageReassemblerInspector()
{}

//--

static const uint32_t MIN_CAPACITY = 1024;

MessageReassembler::MessageReassembler(IMessageReassemblerInspector* inspector, uint32_t initialStorageSize /*= 1024*/, uint32_t maxStorageSize /*= 100U << 20*/, uint32_t maxHeaderSize /*= 10 << 10*/)
    : m_inspector(inspector)
    , m_maxStorageSize(maxStorageSize)
    , m_maxHeaderSize(maxHeaderSize)
    , m_expectedMessageSize(0)
    , m_readPos(0)
    , m_corrupted(false)
{
    // create initial storage
    m_storageCapacity = std::max<uint32_t>(MIN_CAPACITY, initialStorageSize);
    m_storagePos = 0;
    m_storagePtr = mem::GlobalPool<POOL_NET_REASSEMBLER, uint8_t>::AllocN(m_storageCapacity);
}

MessageReassembler::~MessageReassembler()
{
    if (m_storagePtr)
    {
        mem::GlobalPool<POOL_NET_REASSEMBLER>::Free(m_storagePtr);
        m_storagePtr = nullptr;
    }
}

void MessageReassembler::fatalError(StringView reason)
{
    if (!m_corrupted)
    {
        TRACE_ERROR("NetCorruption: {}", reason);
        m_corrupted = true;

        if (m_storagePtr)
        {
            mem::GlobalPool<POOL_NET_REASSEMBLER>::Free(m_storagePtr);
            m_storagePtr = nullptr;
        }
    }
}

bool MessageReassembler::pushData(const void* data, uint32_t dataSize)
{
    // oh well
    if (m_corrupted)
        return false;

    // we won't ever fit
    if (m_storagePos + dataSize > m_maxStorageSize)
    {
        fatalError(TempString("To much data in the unprocessed buffer {} (limit is {})", MemSize(m_storagePos + dataSize), MemSize(m_maxStorageSize)));
        return false;
    }

    // do we have enough storage space ?
    if (m_storagePos + dataSize > m_storageCapacity)
    {
        // try to GC space that was already freed
        if (m_readPos > 0)
        {
            auto validRemainingDataSize  = m_storagePos - m_readPos;
            memmove(m_storagePtr, m_storagePtr + m_readPos, validRemainingDataSize);
            m_storagePos = validRemainingDataSize;
            m_readPos = 0;
        }

        // calculate required size, in proper steps
        uint32_t newCapacity = m_storageCapacity * 2;
        while (m_storagePos + dataSize > newCapacity)
            newCapacity *= 2;

        // allocate new buffer, may fail (we can handle really large data here)
        auto newBuffer  = mem::GlobalPool<POOL_NET_REASSEMBLER, uint8_t>::Resize(m_storagePtr, newCapacity);
        if (!newBuffer)
        {
            fatalError(TempString("Out of memory while trying to resize storage to {}", MemSize(newCapacity)));

            mem::GlobalPool<POOL_NET_REASSEMBLER>::Free(m_storagePtr);
            m_storagePtr = nullptr;

            m_corrupted = true;
            return false;
        }
        else
        {
            m_storageCapacity = newCapacity;
            m_storagePtr = (uint8_t*)newBuffer;
        }
    }

    // append data
    memcpy(m_storagePtr + m_storagePos, data, dataSize);
    m_storagePos += dataSize;
    return true;
}

ReassemblerResult MessageReassembler::reassemble(const uint8_t*& outMessageData, uint32_t& outMessageSize)
{
    // oh well
    if (m_corrupted)
        return ReassemblerResult::Corruption;

    // if we don't have a header try to establish one
    if (0 == m_expectedMessageSize)
    {
        uint32_t messageSize = 0;
        switch (m_inspector->tryParseHeader(m_storagePtr + m_readPos, m_storagePos - m_readPos, messageSize))
        {
            case ReassemblerResult::Valid:
            {
                ASSERT_EX(messageSize != 0, "Message can't be empty");

                // we got a header in the data
                m_expectedMessageSize = messageSize;
                break;
            }

            case ReassemblerResult::NeedsMore:
            {
                // hold on a minute, check if the header size even makes sense
                const auto outstandingDataSize = m_storagePos - m_readPos;
                if (outstandingDataSize > m_maxHeaderSize)
                {
                    fatalError("Header not found");
                    return ReassemblerResult::Corruption;
                }

                return ReassemblerResult::NeedsMore;
            }

            case ReassemblerResult::Corruption:
            default:
            {
                // we got a header in the data
                fatalError("Header corruption detected");
                return ReassemblerResult::Corruption;
            }
        }
    }

    // if we have enough data for a message than process it
    ASSERT(m_expectedMessageSize > 0);
    if (m_storagePos >= (m_readPos + m_expectedMessageSize))
    {
        switch (m_inspector->tryParseMessage(m_storagePtr + m_readPos, m_expectedMessageSize))
        {
            case ReassemblerResult::Valid:
            {
                outMessageData = m_storagePtr + m_readPos;
                outMessageSize = m_expectedMessageSize;
                m_readPos += m_expectedMessageSize;
                m_expectedMessageSize = 0;
                return ReassemblerResult::Valid;
            }

            case ReassemblerResult::NeedsMore:
            {
                fatalError("Message size reported in header was invalid");
                return ReassemblerResult::NeedsMore;
            }

            case ReassemblerResult::Corruption:
            default:
            {
                // we got a header in the data
                fatalError("Message corruption detected");
                return ReassemblerResult::Corruption;
            }
        }
    }

    // we need more data
    return ReassemblerResult::NeedsMore;
}

//--

END_BOOMER_NAMESPACE_EX(net)
