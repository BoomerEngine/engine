/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationFieldPacking.h"
#include "core/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(FieldPackingTest);

BEGIN_BOOMER_NAMESPACE_EX(replication)

TEST(FieldPacking, ParseEmpty)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("", f));
    ASSERT_EQ(PackingMode::Default, f.m_mode);
}

TEST(FieldPacking, ParseBit)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("b", f));
    ASSERT_EQ(PackingMode::Bit, f.m_mode);
    ASSERT_EQ(1, f.calcBitCount());
}

TEST(FieldPacking, ParseSigned)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("s:10", f));
    ASSERT_EQ(PackingMode::Signed, f.m_mode);
    ASSERT_EQ(10, f.calcBitCount());
}

TEST(FieldPacking, ParseSignedFailsWithNoBitCount)
{
    FieldPacking f;
    ASSERT_FALSE(FieldPacking::Parse("s", f));
}

TEST(FieldPacking, ParseSignedFailsWithToSmallBitCount)
{
    FieldPacking f;
    ASSERT_FALSE(FieldPacking::Parse("s:1", f));
}

TEST(FieldPacking, ParseSignedFailsWithToSmallBitCountAlt)
{
    FieldPacking f;
    ASSERT_FALSE(FieldPacking::Parse("s:1", f));
}

TEST(FieldPacking, ParseSignedFailsWithToLargeBitCount)
{
    FieldPacking f;
    ASSERT_FALSE(FieldPacking::Parse("s:30", f));
}

//--

TEST(FieldPacking, ParseUnsigned)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("u:10", f));
    ASSERT_EQ(PackingMode::Unsigned, f.m_mode);
    ASSERT_EQ(10, f.calcBitCount());
}

TEST(FieldPacking, ParseUnsignedFailsWithNoBitCount)
{
    FieldPacking f;
    ASSERT_FALSE(FieldPacking::Parse("u", f));
}

TEST(FieldPacking, ParseUnsignedFailsWithToLargeBitCount)
{
    FieldPacking f;
    ASSERT_FALSE(FieldPacking::Parse("u:30", f));
}

//--

TEST(FieldPacking, ParseFloat)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("f:10,-1,1", f));
    ASSERT_EQ(PackingMode::RangeFloat, f.m_mode);
    ASSERT_EQ(10, f.calcBitCount());
    ASSERT_EQ(-1.0f, f.m_quantization.m_quantizationMin);
    ASSERT_EQ(1.0f, f.m_quantization.m_quantizationMax);
}

TEST(FieldPacking, ParseFloat_ParsingFail)
{
    FieldPacking f;
    ASSERT_FALSE(FieldPacking::Parse("f", f));
    ASSERT_FALSE(FieldPacking::Parse("f:", f));
    ASSERT_FALSE(FieldPacking::Parse("f:10", f));
    ASSERT_FALSE(FieldPacking::Parse("f:10,", f));
    ASSERT_FALSE(FieldPacking::Parse("f:10,-1", f));
    ASSERT_FALSE(FieldPacking::Parse("f:10,-1,", f));
}

//--

TEST(FieldPacking, ParsePosition)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("pos", f));
    ASSERT_EQ(PackingMode::Position, f.m_mode);
    ASSERT_NE(0, f.calcBitCount());
}

TEST(FieldPacking, ParseDeltaPosition)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("delta,10", f));
    ASSERT_EQ(PackingMode::DeltaPosition, f.m_mode);
    ASSERT_EQ(-10.0f, f.m_quantization.m_quantizationMin);
    ASSERT_EQ(10.0f, f.m_quantization.m_quantizationMax);
    ASSERT_NE(0, f.calcBitCount());
}

TEST(FieldPacking, ParseDeltaPositionFailsWithNoRange)
{
    FieldPacking f;
    ASSERT_FALSE(FieldPacking::Parse("delta", f));
    ASSERT_FALSE(FieldPacking::Parse("delta,", f));
}

TEST(FieldPacking, ParseDeltaPositionFailsWithInvalidRange)
{
    FieldPacking f;
    ASSERT_FALSE(FieldPacking::Parse("delta,0.0", f));
    ASSERT_FALSE(FieldPacking::Parse("delta,-1.0", f));
}

TEST(FieldPacking, ParseNormal)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("normal", f));
    ASSERT_EQ(PackingMode::NormalFull, f.m_mode);
}

TEST(FieldPacking, ParseDir)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("dir", f));
    ASSERT_EQ(PackingMode::NormalRough, f.m_mode);
}

TEST(FieldPacking, ParsePitchYaw)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("pitchYaw", f));
    ASSERT_EQ(PackingMode::PitchYaw, f.m_mode);
}

TEST(FieldPacking, ParseAngles)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("angles", f));
    ASSERT_EQ(PackingMode::AllAngles, f.m_mode);
}

TEST(FieldPacking, ParseArrayCountBit)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("b, maxCount:10", f));
    ASSERT_EQ(PackingMode::Bit, f.m_mode);
    ASSERT_EQ(10, f.m_maxCount);
}

TEST(FieldPacking, ParseArrayDefault)
{
    FieldPacking f;
    ASSERT_TRUE(FieldPacking::Parse("maxCount:10", f));
    ASSERT_EQ(PackingMode::Default, f.m_mode);
    ASSERT_EQ(10, f.m_maxCount);
}

//--

TEST(FieldPacking, CompatibilityBool)
{
    FieldPacking f;
    f.m_mode = PackingMode::Bit;
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<bool>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<uint8_t>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<char>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<float>()));
}

TEST(FieldPacking, CompatibilityFloat)
{
    FieldPacking f;
    f.m_mode = PackingMode::RangeFloat;
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<float>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<uint8_t>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<char>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<bool>()));
}

TEST(FieldPacking, CompatibilitySignedTypes)
{
    FieldPacking f;
    f.m_mode = PackingMode::Signed;
    f.m_quantization = Quantization(3);
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<char>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<short>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<int>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<int64_t>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<uint8_t>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<uint16_t>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<uint32_t>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<uint64_t>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<bool>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<float>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<double>()));
}

TEST(FieldPacking, CompatibilityUnsignedTypes)
{
    FieldPacking f;
    f.m_mode = PackingMode::Unsigned;
    f.m_quantization = Quantization(3);
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<char>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<short>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<int>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<int64_t>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<uint8_t>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<uint16_t>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<uint32_t>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<uint64_t>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<bool>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<float>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<double>()));
}

TEST(FieldPacking, CompatibilitySignedTypeSizeCheck)
{
    FieldPacking f;
    f.m_mode = PackingMode::Signed;

    f.m_quantization = Quantization(10);
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<char>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<short>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<int>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<int64_t>()));

    f.m_quantization = Quantization(20);
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<char>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<short>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<int>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<int64_t>()));
}

TEST(FieldPacking, CompatibilityUnsignedTypeSizeCheck)
{
    FieldPacking f;
    f.m_mode = PackingMode::Unsigned;

    f.m_quantization = Quantization(10);
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<uint8_t>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<uint16_t>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<uint32_t>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<uint64_t>()));

    f.m_quantization = Quantization(20);
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<uint8_t>()));
    ASSERT_FALSE(f.checkTypeCompatibility(GetTypeObject<uint16_t>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<uint32_t>()));
    ASSERT_TRUE(f.checkTypeCompatibility(GetTypeObject<uint64_t>()));
}

//--

TEST(FieldPacking, CheckPackBit)
{
    FieldPacking f;
    f.m_mode = PackingMode::Bit;
}

END_BOOMER_NAMESPACE_EX(replication)






