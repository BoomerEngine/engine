/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationBitReader.h"
#include "replicationBitWriter.h"

#include "build.h"
#include "core/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(BitWriterTest);

BEGIN_BOOMER_NAMESPACE_EX(replication)

TEST(Network, BitWriterTest_Empty)
{
    BitWriter w;
    ASSERT_EQ(0, w.bitSize());
}

TEST(Network, BitWriterTest_SingleBitZero)
{
    BitWriter w;
    w.writeBit(0);
    ASSERT_EQ(1, w.bitSize());
    ASSERT_EQ(0, w.data()[0]);
}

TEST(Network, BitWriterTest_SingleBitOne)
{
    BitWriter w;
    w.writeBit(1);
    ASSERT_EQ(1, w.bitSize());
    ASSERT_EQ(1, w.data()[0]);
}

TEST(Network, BitWriterTest_ByteFill)
{
    BitWriter w;
    w.writeBit(0);
    w.writeBit(1);
    w.writeBit(0);
    w.writeBit(1);
    w.writeBit(0);
    w.writeBit(1);
    w.writeBit(0);
    w.writeBit(1);
    ASSERT_EQ(8, w.bitSize());
    ASSERT_EQ(0xAA, w.data()[0]);
}

struct BitReaderNoFail
{
    BitReader r;

    BitReaderNoFail()
    {}

    void reset(const void* data, uint32_t size)
    {
        r.reset(data, size);
    }

    bool readBit()
    {
        bool ret = false;
        EXPECT_TRUE(r.readBit(ret));
        return ret;
    }

    BitReader::WORD readBits(uint32_t numBits)
    {
        BitReader::WORD ret = 0;
        EXPECT_TRUE(r.readBits(numBits, ret));
        return ret;
    }

    void readBlock(void* data, uint32_t dataSize)
    {
        EXPECT_TRUE(r.readBlock(data, dataSize));
    }

    BitReader::WORD readAdaptiveNumber()
    {
        BitReader::WORD ret = 0;
        EXPECT_TRUE(r.readAdaptiveNumber(ret));
        return ret;
    }
};

template< uint32_t N = 100 >
struct BitTestStorage
{
    uint32_t data[N] = {0};

    BitReaderNoFail r;
    BitWriter w;

    BitTestStorage()
        : w(data, N * 8)
    {
        memzero(data, sizeof(data));
    }

    void startReading()
    {
        r.reset(w.data(), w.bitSize());
    }
};

TEST(Network, BitCopyTest_SingleBit)
{
    BitTestStorage f;

    f.w.writeBit(0);
    f.startReading();

    ASSERT_FALSE(f.r.readBit());
}

TEST(Network, BitCopyTest_SingleBitTrue)
{
    BitTestStorage f;

    f.w.writeBit(1);
    f.startReading();

    ASSERT_TRUE(f.r.readBit());
}

TEST(Network, BitCopyTest_MultiBit)
{
    BitTestStorage f;

    f.w.writeBits(0xAA, 4);
    f.w.writeBits(0xBB, 4);
    f.w.writeBits(0xCC, 4);
    f.w.writeBits(0xDD, 4);
    f.startReading();

    EXPECT_EQ(0xA,f.r.readBits(4));
    EXPECT_EQ(0xB,f.r.readBits(4));
    EXPECT_EQ(0xC,f.r.readBits(4));
    EXPECT_EQ(0xD,f.r.readBits(4));
}

TEST(Network, BitCopyTest_MultiBitWordSpan)
{
    for (uint32_t testWordSize=1; testWordSize < 32; ++testWordSize)
    {
        BitTestStorage f;

        auto wordMask = (testWordSize < 32) ? ((1U << testWordSize) - 1) : ~0U;
        auto numWords = (sizeof(f.data) * 8) / (2 * testWordSize);

        f.w.reserve(2 * numWords * testWordSize);
        for (uint32_t i = 0; i < numWords; ++i)
        {
            f.w.writeBits(i & wordMask, testWordSize);
            f.w.writeBits(~i & wordMask, testWordSize);
        }

        f.startReading();

        for (uint32_t i = 0; i < numWords; ++i)
        {
            auto a = f.r.readBits(testWordSize);
            EXPECT_EQ(i & wordMask, a);
            auto b = f.r.readBits(testWordSize);
            EXPECT_EQ(~i & wordMask, b);
        }
    }
}

TEST(Network, BitCopyTest_AdaptiveNumber)
{
    uint64_t val = 0;
    uint64_t prev = 0;
    auto max = ~0U;
    while (val <= max)
    {
        TRACE_INFO("Value: {}", val);

        BitTestStorage f;
        f.w.writeAdaptiveNumber(val);
        f.startReading();

        auto ret = f.r.readAdaptiveNumber();
        EXPECT_EQ(val, ret);

        auto newVal = val + prev;
        prev = val;
        val = newVal ? newVal : 1;
    }
}

END_BOOMER_NAMESPACE_EX(replication)