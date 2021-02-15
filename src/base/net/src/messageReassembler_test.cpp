﻿/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages #]
***/

#include "build.h"
#include "base/test/include/gtest/gtest.h"

#include "messageReassembler.h"

using namespace base;
using namespace base::net;

DECLARE_TEST_FILE(MessageReassembler);

namespace test
{

    class SimpleHeaderParser : public IMessageReassemblerInspector
    {
    public:
        uint32_t m_requiredHeaderSize = 8;
        uint32_t m_reportedMesssageSize = 64;
        bool m_failHeader = false;
        bool m_failMessage = false;
        mutable uint32_t m_numMessagesParser = 0;

        virtual ReassemblerResult tryParseHeader(const uint8_t* currentData, uint32_t currentDataSize, uint32_t& outTotalMessageSize) const override final
        {
            if (currentDataSize < m_requiredHeaderSize)
                return ReassemblerResult::NeedsMore;

            if (m_failHeader)
                return ReassemblerResult::Corruption;

            outTotalMessageSize = m_reportedMesssageSize;
            return ReassemblerResult::Valid;
        }

        virtual ReassemblerResult tryParseMessage(const uint8_t* messageData, uint32_t messageDataSize) const override final
        {
            if (m_failMessage)
                return ReassemblerResult::Corruption;
            if (messageDataSize != m_reportedMesssageSize)
                return ReassemblerResult::Corruption;

            m_numMessagesParser += 1;
            return ReassemblerResult::Valid;
        }
    };


} // test

TEST(MessageReassembler, OneMessage)
{
    test::SimpleHeaderParser parser;
    MessageReassembler dut(&parser);

    for (uint32_t i=1; i<=parser.m_reportedMesssageSize; ++i)
    {
        uint8_t data = i;
        dut.pushData(&data, 1);

        const uint8_t* messagePtr = nullptr;
        uint32_t messageSize;
        auto ret = dut.reassemble(messagePtr, messageSize);
        if (i < parser.m_reportedMesssageSize)
        {
            ASSERT_EQ(ReassemblerResult::NeedsMore, ret);
        }
        else if (i == parser.m_reportedMesssageSize)
        {
            ASSERT_EQ(ReassemblerResult::Valid, ret);
            ASSERT_EQ(parser.m_reportedMesssageSize, messageSize);

            for (uint32_t j=0; j<messageSize; ++j)
            {
                ASSERT_EQ(j+1, messagePtr[j]);
            }
        }
    }

    ASSERT_EQ(1, parser.m_numMessagesParser);
}

TEST(MessageReassembler, HeaderCanFail)
{
    test::SimpleHeaderParser parser;
    MessageReassembler dut(&parser);

    parser.m_failHeader = true;

    for (uint32_t i=1; i<=parser.m_reportedMesssageSize; ++i)
    {
        uint8_t data = i;
        dut.pushData(&data, 1);

        const uint8_t* messagePtr = nullptr;
        uint32_t messageSize;
        auto ret = dut.reassemble(messagePtr, messageSize);
        if (i < parser.m_requiredHeaderSize)
        {
            ASSERT_EQ(ReassemblerResult::NeedsMore, ret);
        }
        else
        {
            ASSERT_EQ(ReassemblerResult::Corruption, ret);
        }
    }

    ASSERT_EQ(0, parser.m_numMessagesParser);
}

TEST(MessageReassembler, MessageCanFail)
{
    test::SimpleHeaderParser parser;
    MessageReassembler dut(&parser);

    parser.m_failMessage = true;

    for (uint32_t i=1; i<=parser.m_reportedMesssageSize; ++i)
    {
        uint8_t data = i;
        dut.pushData(&data, 1);

        const uint8_t* messagePtr = nullptr;
        uint32_t messageSize;
        auto ret = dut.reassemble(messagePtr, messageSize);
        if (i < parser.m_reportedMesssageSize)
        {
            ASSERT_EQ(ReassemblerResult::NeedsMore, ret);
        }
        else if (i == parser.m_reportedMesssageSize)
        {
            ASSERT_EQ(ReassemblerResult::Corruption, ret);
        }
    }

    ASSERT_EQ(0, parser.m_numMessagesParser);
}

TEST(MessageReassembler, ManyMessagesFromSmallData)
{
    test::SimpleHeaderParser parser;
    MessageReassembler dut(&parser);

    auto numMessages = 10;
    for (uint32_t i = 1; i <= numMessages * parser.m_reportedMesssageSize; ++i)
    {
        uint8_t data = i;
        dut.pushData(&data, 1);

        const uint8_t *messagePtr = nullptr;
        uint32_t messageSize;
        auto ret = dut.reassemble(messagePtr, messageSize);
        if (i < parser.m_reportedMesssageSize)
        {
            ASSERT_EQ(ReassemblerResult::NeedsMore, ret);
        }
        else if (i == parser.m_reportedMesssageSize)
        {
            ASSERT_EQ(ReassemblerResult::Valid, ret);
            ASSERT_EQ(parser.m_reportedMesssageSize, messageSize);

            for (uint32_t j = 0; j < messageSize; ++j)
            {
                ASSERT_EQ(j, (messagePtr[j] - 1) % parser.m_reportedMesssageSize);
            }
        }
    }

    ASSERT_EQ(numMessages, parser.m_numMessagesParser);
}

TEST(MessageReassembler, ManyMessagesFromLargeData)
{
    test::SimpleHeaderParser parser;
    MessageReassembler dut(&parser);

    auto numMessages = 10;

    Array<uint8_t> data;
    data.resize(parser.m_reportedMesssageSize * numMessages);
    for (uint32_t j=0; j<parser.m_reportedMesssageSize * numMessages; ++j)
        data[j] = j;

    dut.pushData(data.data(), data.dataSize());

    const uint8_t *messagePtr = nullptr;
    uint32_t messageSize;

    for (uint32_t i=0; i<numMessages; ++i)
    {
        auto ret = dut.reassemble(messagePtr, messageSize);
        ASSERT_EQ(ReassemblerResult::Valid, ret);
        ASSERT_EQ(parser.m_reportedMesssageSize, messageSize);
    }

    auto ret = dut.reassemble(messagePtr, messageSize);
    ASSERT_EQ(ReassemblerResult::NeedsMore, ret);
    ASSERT_EQ(numMessages, parser.m_numMessagesParser);
}

