/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationQuantization.h"

#include "build.h"
#include "base/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(QuantizationTest);

using namespace base;
using namespace base::replication;

//--

TEST(Quantization, BitCountEmptyIsMax)
{
    BitCount bitCount;
    EXPECT_EQ(0, bitCount.m_bitCount);
    EXPECT_EQ(std::numeric_limits<uint32_t>::max(), bitCount.maxUnsigned());
    EXPECT_EQ(std::numeric_limits<int>::max(), bitCount.maxSigned());
    EXPECT_EQ(std::numeric_limits<int>::min(), bitCount.minSigned());
}

TEST(Quantization, BitCountRanges1)
{
    BitCount bitCount(1);
    EXPECT_EQ(1, bitCount.maxUnsigned());
}

TEST(Quantization, BitCountRanges2)
{
    BitCount bitCount(2);
    EXPECT_EQ(3, bitCount.maxUnsigned());
    EXPECT_EQ(-2, bitCount.minSigned());
    EXPECT_EQ(1, bitCount.maxSigned());
}

TEST(Quantization, BitCountRanges8)
{
    BitCount bitCount(8);
    EXPECT_EQ(std::numeric_limits<uint8_t>::max(), bitCount.maxUnsigned());
    EXPECT_EQ(std::numeric_limits<char>::min(), bitCount.minSigned());
    EXPECT_EQ(std::numeric_limits<char>::max(), bitCount.maxSigned());
}

TEST(Quantization, BitCountRanges16)
{
    BitCount bitCount(16);
    EXPECT_EQ(std::numeric_limits<uint16_t>::max(), bitCount.maxUnsigned());
    EXPECT_EQ(std::numeric_limits<short>::min(), bitCount.minSigned());
    EXPECT_EQ(std::numeric_limits<short>::max(), bitCount.maxSigned());
}

TEST(Quantization, BitCountRanges32)
{
    BitCount bitCount(32);
    EXPECT_EQ(std::numeric_limits<uint32_t>::max(), bitCount.maxUnsigned());
    EXPECT_EQ(std::numeric_limits<int>::min(), bitCount.minSigned());
    EXPECT_EQ(std::numeric_limits<int>::max(), bitCount.maxSigned());
}

//--

TEST(Quantization, QuantizationEmpty)
{
    Quantization q;
    EXPECT_EQ(0, q.m_bitCount.m_bitCount);
}

TEST(Quantization, QuantizationUnsignedInRange)
{
    Quantization q(8);
    EXPECT_EQ(0, q.quantizeUnsigned(0));
    EXPECT_EQ(100, q.quantizeUnsigned(100));
    EXPECT_EQ(255, q.quantizeUnsigned(300));
    EXPECT_EQ(255, q.quantizeUnsigned(std::numeric_limits<uint32_t>::max()));
}

TEST(Quantization, QuantizationUnsignedFull)
{
    Quantization q;
    EXPECT_EQ(0, q.quantizeUnsigned(0));
    EXPECT_EQ(100, q.quantizeUnsigned(100));
    EXPECT_EQ(300, q.quantizeUnsigned(300));
    EXPECT_EQ(std::numeric_limits<uint32_t>::max(), q.quantizeUnsigned(std::numeric_limits<uint32_t>::max()));
}

TEST(Quantization, QuantizationSignedInRange)
{
    Quantization q(8);
    EXPECT_EQ(0, q.quantizeSigned(0));
    EXPECT_EQ(100, q.quantizeSigned(100));
    EXPECT_EQ((uint8_t)-1, q.quantizeSigned(-1));
    EXPECT_EQ((uint8_t)-100, q.quantizeSigned(-100));
    EXPECT_EQ(127, q.quantizeSigned(200));
    EXPECT_EQ((uint8_t)-128, q.quantizeSigned(-200));
    EXPECT_EQ(127, q.quantizeSigned(std::numeric_limits<int>::max()));
    EXPECT_EQ((uint8_t)-128, q.quantizeSigned(std::numeric_limits<int>::min()));
}

TEST(Quantization, QuantizationSignedFull)
{
    Quantization q;
    EXPECT_EQ(0, q.quantizeSigned(0));
    EXPECT_EQ(100, q.quantizeSigned(100));
    EXPECT_EQ(-1, q.quantizeSigned(-1));
    EXPECT_EQ(-100, q.quantizeSigned(-100));
    EXPECT_EQ(200, q.quantizeSigned(200));
    EXPECT_EQ(-200, q.quantizeSigned(-200));
    EXPECT_EQ(std::numeric_limits<int>::max(), q.quantizeSigned(std::numeric_limits<int>::max()));
    EXPECT_EQ(std::numeric_limits<int>::min(), q.quantizeSigned(std::numeric_limits<int>::min()));
}

//--

TEST(Quantization, QuantizationFloat)
{
    Quantization q(8, 0.0f, 1.0f);

    EXPECT_EQ(0, q.quantizeFloat(0.0f));
    EXPECT_EQ(0, q.quantizeFloat(-1.0f));
    EXPECT_EQ(255, q.quantizeFloat(1.0f));
    EXPECT_EQ(255, q.quantizeFloat(2.0f));
    EXPECT_EQ(128, q.quantizeFloat(0.5f));
}

TEST(Quantization, QuantizationFloatSigned)
{
    Quantization q(8, -1.0f, 1.0f);

    EXPECT_EQ(0, q.quantizeFloat(-2.0f));
    EXPECT_EQ(0, q.quantizeFloat(-1.0f));
    EXPECT_EQ(255, q.quantizeFloat(1.0f));
    EXPECT_EQ(255, q.quantizeFloat(2.0f));
    EXPECT_EQ(128, q.quantizeFloat(0.0f));
}

TEST(Quantization, QuantizationFloatInfinities)
{
    Quantization q(8, 0.0f, 1.0f);

    auto inf = std::numeric_limits<float>::max();

    EXPECT_EQ(0, q.quantizeFloat(-inf));
    EXPECT_EQ(255, q.quantizeFloat(inf));
}

TEST(Quantization, QuantizationFloatNans)
{
    Quantization q(8, 0.0f, 1.0f);

    auto _nan = NAN;

    EXPECT_EQ(0, q.quantizeFloat(_nan));
    EXPECT_EQ(0, q.quantizeFloat(_nan));
}

//--

TEST(Quantization, RestoreUnsigned)
{
    Quantization q(8);
    EXPECT_EQ(0, q.unquantizeUnsigned(0));
    EXPECT_EQ(0, q.unquantizeUnsigned(256));
    EXPECT_EQ(100, q.unquantizeUnsigned(100));
}

TEST(Quantization, RestoreSigned)
{
    Quantization q(8);
    EXPECT_EQ(0, q.unquantizeSigned(0));
    EXPECT_EQ(100, q.unquantizeSigned(100));
    EXPECT_EQ(-1, q.unquantizeSigned(255));
}

TEST(Quantization, RestoreFloat)
{
    Quantization q(8, 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(0, q.unquantizeFloat(0));
    EXPECT_NEAR(0.5f, q.unquantizeFloat(128), q.quantizationError());
    EXPECT_NEAR(1.0f, q.unquantizeFloat(255), q.quantizationError());
}

TEST(Quantization, RestoreFloatSigned)
{
    Quantization q(8, -1.0f, 1.0f);
    EXPECT_FLOAT_EQ(-1.0f, q.unquantizeFloat(0));
    EXPECT_NEAR(0.0f, q.unquantizeFloat(128), q.quantizationError());
    EXPECT_NEAR(1.0f, q.unquantizeFloat(255), q.quantizationError());
}

//--
