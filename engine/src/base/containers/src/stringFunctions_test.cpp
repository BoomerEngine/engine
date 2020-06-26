/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "base/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(StringFunctions);

//---

TEST(CompileTimeCRC, EmptyTest)
{
    auto crc = base::prv::CompileTime_CalcCRC32("");
    EXPECT_EQ(0, crc);
}

TEST(CompileTimeCRC, CompileTimeCRCEqualsRuntimeCRC)
{
    auto crc = base::prv::CompileTime_CalcCRC32("Ala ma kota");

    base::CRC32 rt;
    rt << "Ala ma kota";
    auto crc2 = rt.crc();

    EXPECT_EQ(crc2, crc);
}

//---

TEST(MatchInteger, Match_InvalidChars)
{
    char val = 0;
    auto ret = base::StringView<char>("100.00").match(val);
    EXPECT_EQ(base::MatchResult::InvalidCharacter, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, Match_PlusConsumed)
{
    char val = 0;
    auto ret = base::StringView<char>("+5").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 5);
}

TEST(MatchInteger, Match_DoublePlusNotConsumed)
{
    char val = 0;
    auto ret = base::StringView<char>("++5").match(val);
    EXPECT_EQ(base::MatchResult::InvalidCharacter, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, Match_DoubleMinusNotConsumed)
{
    char val = 0;
    auto ret = base::StringView<char>("--5").match(val);
    EXPECT_EQ(base::MatchResult::InvalidCharacter, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, Match_MinusNotConsumedForUnsigned)
{
    uint8_t val = 0;
    auto ret = base::StringView<char>("-5").match(val);
    EXPECT_EQ(base::MatchResult::InvalidCharacter, ret);
    EXPECT_EQ(val, 0);
}

//---

TEST(MatchInteger, MatchInt8_Pos)
{
    char val = 0;
    auto ret = base::StringView<char>("42").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 42);
}

TEST(MatchInteger, MatchInt8_Neg)
{
    char val = 0;
    auto ret = base::StringView<char>("-42").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, -42);
}

TEST(MatchInteger, MatchInt8_Max)
{
    char val = 0;
    auto ret = base::StringView<char>("127").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 127);
}

TEST(MatchInteger, MatchInt8_Min)
{
    char val = 0;
    auto ret = base::StringView<char>("-128").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, -128);
}

TEST(MatchInteger, MatchInt8_Overflow)
{
    char val = 0;
    auto ret = base::StringView<char>("128").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, MatchInt8_OverflowMin)
{
    char val = 0;
    auto ret = base::StringView<char>("-129").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, MatchInt8_OverflowLargeNubmerWithZeros)
{
    char val = 0;
    auto ret = base::StringView<char>("10000").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}


TEST(MatchInteger, MatchInt8_LeadingZerosNoOverlow)
{
    char val = 0;
    auto ret = base::StringView<char>("0000042").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 42);
}

//------

TEST(MatchInteger, MatchInt16_Pos)
{
    short val = 0;
    auto ret = base::StringView<char>("42").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 42);
}

TEST(MatchInteger, MatchInt16_Neg)
{
    short val = 0;
    auto ret = base::StringView<char>("-42").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, -42);
}

TEST(MatchInteger, MatchInt16_Max)
{
    short val = 0;
    auto ret = base::StringView<char>("32767").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 32767);
}

TEST(MatchInteger, MatchInt16_Min)
{
    short val = 0;
    auto ret = base::StringView<char>("-32768").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, -32768);
}

TEST(MatchInteger, MatchInt16_Overflow)
{
    short val = 0;
    auto ret = base::StringView<char>("32768").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, MatchInt16_OverflowMin)
{
    short val = 0;
    auto ret = base::StringView<char>("-32769").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, MatchInt16_OverflowLargeNubmerWithZeros)
{
    short val = 0;
    auto ret = base::StringView<char>("100000000").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}


TEST(MatchInteger, MatchInt16_LeadingZerosNoOverlow)
{
    short val = 0;
    auto ret = base::StringView<char>("00000235").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 235);
}

//------

TEST(MatchInteger, MatchInt32_Pos)
{
    int val = 0;
    auto ret = base::StringView<char>("42").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 42);
}

TEST(MatchInteger, MatchInt32_Neg)
{
    int val = 0;
    auto ret = base::StringView<char>("-42").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, -42);
}

TEST(MatchInteger, MatchInt32_Max)
{
    int val = 0;
    auto ret = base::StringView<char>("2147483647").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 2147483647);
}

TEST(MatchInteger, MatchInt32_Min)
{
    int val = 0;
    auto ret = base::StringView<char>("-2147483648").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, -(int64_t)2147483648);
}

TEST(MatchInteger, MatchInt32_Overflow)
{
    int val = 0;
    auto ret = base::StringView<char>("2147483648").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, MatchInt32_OverflowMin)
{
    int val = 0;
    auto ret = base::StringView<char>("-2147483649").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, MatchInt32_OverflowLargeNubmerWithZeros)
{
    int val = 0;
    auto ret = base::StringView<char>("100000000000").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}


TEST(MatchInteger, MatchInt32_LeadingZerosNoOverlow)
{
    int val = 0;
    auto ret = base::StringView<char>("0000234234").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 234234);
}

//------

TEST(MatchInteger, MatchInt64_Pos)
{
    int64_t val = 0;
    auto ret = base::StringView<char>("42").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 42);
}

TEST(MatchInteger, MatchInt64_Neg)
{
    int64_t val = 0;
    auto ret = base::StringView<char>("-42").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, -42);
}

TEST(MatchInteger, MatchInt64_Max)
{
    int64_t val = 0;
    auto ret = base::StringView<char>("9223372036854775807").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 9223372036854775807);
}

TEST(MatchInteger, MatchInt64_Min)
{
    int64_t val = 0;
    auto ret = base::StringView<char>("-9223372036854775808").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, std::numeric_limits<int64_t>::min());
}

TEST(MatchInteger, MatchInt64_Overflow)
{
    int64_t val = 0;
    auto ret = base::StringView<char>("9223372036854775808").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, MatchInt64_OverflowMin)
{
    int64_t val = 0;
    auto ret = base::StringView<char>("-9223372036854775809").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}

TEST(MatchInteger, MatchInt64_OverflowLargeNubmerWithZeros)
{
    int64_t val = 0;
    auto ret = base::StringView<char>("1000000000000000000000").match(val);
    EXPECT_EQ(base::MatchResult::Overflow, ret);
    EXPECT_EQ(val, 0);
}


TEST(MatchInteger, MatchInt64_LeadingZerosNoOverlow)
{
    int64_t val = 0;
    auto ret = base::StringView<char>("0000234234").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 234234);
}

//------

TEST(MatchInteger, MatchFloat_Pos)
{
    double val = 0;
    auto ret = base::StringView<char>("42").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, 42.0);
}

TEST(MatchInteger, MatchFloat_Neg)
{
    double val = 0;
    auto ret = base::StringView<char>("-42").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_EQ(val, -42.0);
}

TEST(MatchInteger, MatchFloat_SimpleFrac)
{
    double val = 0;
    auto ret = base::StringView<char>("3.14").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_NEAR(val, 3.14, 0.001);
}

TEST(MatchInteger, MatchFloat_OnlyFrac)
{
    double val = 0;
    auto ret = base::StringView<char>(".14").match(val);
    EXPECT_EQ(base::MatchResult::OK, ret);
    EXPECT_NEAR(val, 0.14, 0.001);
}

TEST(MatchInteger, MatchFloat_InvalidChars)
{
    double val = 0;
    auto ret = base::StringView<char>("124x").match(val);
    EXPECT_EQ(base::MatchResult::InvalidCharacter, ret);
    EXPECT_EQ(val, 0);
}

//---

TEST(StringMatch, WildcardMatches)
{
    EXPECT_TRUE(base::StringView<char>("geeks").matchString("g*ks"));
    EXPECT_TRUE(base::StringView<char>("geeksforgeeks").matchString("ge?ks*"));
    EXPECT_FALSE(base::StringView<char>("gee").matchString("g*k"));
    EXPECT_FALSE(base::StringView<char>("pqrst").matchString("*pqrs"));
    EXPECT_TRUE(base::StringView<char>("abcdhghgbcd").matchString("abc*bcd"));
    EXPECT_FALSE(base::StringView<char>("abc*c?d").matchString("abcd"));
    EXPECT_TRUE(base::StringView<char>("abcd").matchString("*c*d"));
    EXPECT_TRUE(base::StringView<char>("abcd").matchString("*?c*d"));
}
