/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "stringBuf.h"

#include "core/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(StringBuf);

namespace testing
{
    template<>
    INLINE::std::string PrintToString<boomer::StringBuf>(const boomer::StringBuf& value)
    {
        return value.c_str();
    }

} // testing

BEGIN_BOOMER_NAMESPACE()

TEST(StringBuf, CreateEmpty)
{
    StringBuf x;
    EXPECT_TRUE(x.empty());
}

TEST(StringBuf, EmptyStringHasZeroLength)
{
    StringBuf x;
    EXPECT_EQ(0U, x.length());
}

TEST(StringBuf, AllEmptyStringsHasCBuf)
{
    StringBuf x, y;
    EXPECT_TRUE(x.c_str());
    EXPECT_EQ(0, x.c_str()[0]);
}

TEST(StringBuf, AllEmptyStringsHaveSameCBuf)
{
    StringBuf x, y;
    EXPECT_EQ(x.c_str(), y.c_str());
}

TEST(StringBuf, CreateSimple)
{
    StringBuf x("test");
    ASSERT_EQ(4U, x.length());
    EXPECT_EQ('t', x.c_str()[0]);
    EXPECT_EQ('e', x.c_str()[1]);
    EXPECT_EQ('s', x.c_str()[2]);
    EXPECT_EQ('t', x.c_str()[3]);
    EXPECT_EQ(0, x.c_str()[4]);
}

TEST(StringBuf, CopyConstruct)
{
    StringBuf x("test");
    StringBuf y(x);

    ASSERT_EQ(4U, x.length());
    EXPECT_EQ('t', x.c_str()[0]);
    EXPECT_EQ('e', x.c_str()[1]);
    EXPECT_EQ('s', x.c_str()[2]);
    EXPECT_EQ('t', x.c_str()[3]);
    EXPECT_EQ(0, x.c_str()[4]);

    ASSERT_EQ(4U, y.length());
    EXPECT_EQ('t', y.c_str()[0]);
    EXPECT_EQ('e', y.c_str()[1]);
    EXPECT_EQ('s', y.c_str()[2]);
    EXPECT_EQ('t', y.c_str()[3]);
    EXPECT_EQ(0, y.c_str()[4]);
}

TEST(StringBuf, MoveConstruct)
{
    StringBuf x("test");
    StringBuf y(std::move(x));
    EXPECT_TRUE(x.empty());
    ASSERT_EQ(4U, y.length());
    EXPECT_EQ('t', y.c_str()[0]);
    EXPECT_EQ('e', y.c_str()[1]);
    EXPECT_EQ('s', y.c_str()[2]);
    EXPECT_EQ('t', y.c_str()[3]);
    EXPECT_EQ(0, y.c_str()[4]);
}

TEST(StringBuf, CopyAssignment)
{
    StringBuf x("test");
    StringBuf y = x;

    ASSERT_EQ(4U, x.length());
    EXPECT_EQ('t', x.c_str()[0]);
    EXPECT_EQ('e', x.c_str()[1]);
    EXPECT_EQ('s', x.c_str()[2]);
    EXPECT_EQ('t', x.c_str()[3]);
    EXPECT_EQ(0, x.c_str()[4]);

    ASSERT_EQ(4U, y.length());
    EXPECT_EQ('t', y.c_str()[0]);
    EXPECT_EQ('e', y.c_str()[1]);
    EXPECT_EQ('s', y.c_str()[2]);
    EXPECT_EQ('t', y.c_str()[3]);
    EXPECT_EQ(0, y.c_str()[4]);
}

TEST(StringBuf, MoveAssignment)
{
    StringBuf x("test");
    StringBuf y = std::move(x);

    EXPECT_TRUE(x.empty());

    ASSERT_EQ(4U, y.length());
    EXPECT_EQ('t', y.c_str()[0]);
    EXPECT_EQ('e', y.c_str()[1]);
    EXPECT_EQ('s', y.c_str()[2]);
    EXPECT_EQ('t', y.c_str()[3]);
    EXPECT_EQ(0, y.c_str()[4]);
}

TEST(StringBuf, EqualTest)
{
    StringBuf x("test");
    StringBuf y("test");
    EXPECT_TRUE(x == y);
}

TEST(StringBuf, EqualTestFails)
{
    StringBuf x("test");
    StringBuf y("dupa");
    EXPECT_FALSE(x == y);
}

TEST(StringBuf, NonEqualTest)
{
    StringBuf x("test");
    StringBuf y("dupa");
    EXPECT_TRUE(x != y);
}

TEST(StringBuf, NonEqualTestFails)
{
    StringBuf x("test");
    StringBuf y("test");
    EXPECT_FALSE(x != y);
}

TEST(StringBuf, RawEqualTest)
{
    StringBuf x("test");
    auto y  = "test";
    EXPECT_TRUE(x == y);
}

TEST(StringBuf, RawEqualTestFails)
{
    StringBuf x("test");
    auto y  = "dupa";
    EXPECT_FALSE(x == y);
}

TEST(StringBuf, RawNonEqualTest)
{
    StringBuf x("test");
    auto y  = "dupa";
    EXPECT_TRUE(x != y);
}

TEST(StringBuf, RawNonEqualTestFails)
{
    StringBuf x("test");
    auto y  = "test";
    EXPECT_FALSE(x != y);
}

TEST(StringBuf, AppendTest)
{
    StringVector x("Ala");
    x.append(" ma ");
    x.append("kota");

    StringVector y("Ala ma kota");
    EXPECT_EQ(x, y);
}

TEST(StringBuf, AppendTestRaw)
{
    StringVector x("Hello");
    StringVector y("World");
    x.append(y);

    StringVector z("HelloWorld");
    EXPECT_EQ(z, x);
}

TEST(StringBuf, OrderTest)
{
    StringBuf x("Alice");
    StringBuf y("Bob");
    EXPECT_TRUE(x < y);
}

TEST(StringBuf, EmptyStringBeforeAnyString)
{
    StringBuf x;
    StringBuf y(" ");
    EXPECT_TRUE(x < y);
}

TEST(StringBuf, AppendOperator)
{
    StringVector x("Hello");
    StringVector y("World");
    x += y;
    
    EXPECT_TRUE(x == "HelloWorld");
}

TEST(StringBuf, AppendOperatorRaw)
{
    StringVector x("Hello");
    x += "World";

    EXPECT_TRUE(x == "HelloWorld");
}

TEST(StringBuf, ConcatenateOperator)
{
    StringVector x("Hello");
    StringVector y("World");
    auto z = x + y;

    EXPECT_TRUE(x == "Hello");
    EXPECT_TRUE(y == "World");
    EXPECT_TRUE(z == "HelloWorld");
}

TEST(StringBuf, ConcatenateOperatorRaw)
{
    StringVector x("Hello");
    auto z = x + "World";

    EXPECT_TRUE(x == "Hello");
    EXPECT_TRUE(z == "HelloWorld");
}

TEST(StringBuf, Compare)
{
    StringBuf x("Bob");

    EXPECT_LT(0, x.compareWith(StringBuf::EMPTY()));
    EXPECT_LT(0, x.compareWith(StringBuf("Alice")));
    EXPECT_EQ(0, x.compareWith(StringBuf("Bob")));
    EXPECT_GT(0, x.compareWith(StringBuf("Charles ")));
}

TEST(StringBuf, CompareRaw)
{
    StringBuf x("Bob");

    EXPECT_LT(0, x.compareWith(""));
    EXPECT_LT(0, x.compareWith("Alice"));
    EXPECT_EQ(0, x.compareWith("Bob"));
    EXPECT_NE(0, x.compareWith("bob"));
    EXPECT_GT(0, x.compareWith("Charles"));
}

TEST(StringBuf, CompareNoCase)
{
    StringBuf x("bob");

    EXPECT_LT(0, x.compareWithNoCase(StringBuf::EMPTY()));
    EXPECT_LT(0, x.compareWithNoCase(StringBuf("alice")));
    EXPECT_EQ(0, x.compareWithNoCase(StringBuf("Bob")));
    EXPECT_EQ(0, x.compareWithNoCase(StringBuf("bob")));
    EXPECT_GT(0, x.compareWithNoCase(StringBuf("CHARLES")));
}

TEST(StringBuf, CompareNoCaseRaw)
{
    StringBuf x("bob");

    EXPECT_LT(0, x.compareWithNoCase(""));
    EXPECT_LT(0, x.compareWithNoCase("Alice"));
    EXPECT_EQ(0, x.compareWithNoCase("Bob"));
    EXPECT_EQ(0, x.compareWithNoCase("bob"));
    EXPECT_GT(0, x.compareWithNoCase("CHARLES"));
}

TEST(StringBuf, CompareN)
{
    StringBuf x("testX");

    EXPECT_NE(0, x.compareWith("testY"));
    EXPECT_NE(0, x.compareWith(StringView("TESTY", 4)));
    EXPECT_EQ(0, x.leftPart(4).compareWith(StringView("testY", 4)));
}

TEST(StringBuf, CompareNoCaseN)
{
    StringBuf x("testX");

    EXPECT_NE(0, x.compareWithNoCase("testY"));
    EXPECT_EQ(0, x.leftPart(4).compareWithNoCase(StringView("TESTY", 4)));
    EXPECT_EQ(0, x.leftPart(4).compareWithNoCase(StringView("testY", 4)));
}

TEST(StringBuf, LeftPartOfEmptyStringIsAlwaysEmpty)
{
    StringBuf x;

    EXPECT_EQ(StringBuf::EMPTY(), x.leftPart(0));
    EXPECT_EQ(StringBuf::EMPTY(), x.leftPart(100));
}

TEST(StringBuf, LeftPartNotBiggerThanString)
{
    StringBuf x("test");

    EXPECT_EQ(StringBuf::EMPTY(), x.leftPart(0));
    EXPECT_EQ(StringBuf("t"), x.leftPart(1));
    EXPECT_EQ(StringBuf("te"), x.leftPart(2));
    EXPECT_EQ(StringBuf("tes"), x.leftPart(3));
    EXPECT_EQ(StringBuf("test"), x.leftPart(4));
    EXPECT_EQ(StringBuf("test"), x.leftPart(5));
    EXPECT_EQ(StringBuf("test"), x.leftPart(100));
}

TEST(StringBuf, RightPartOfEmptyStringIsAlwaysEmpty)
{
    StringBuf x;

    EXPECT_EQ(StringBuf::EMPTY(), x.rightPart(0));
    EXPECT_EQ(StringBuf::EMPTY(), x.rightPart(100));

}

TEST(StringBuf, RightPartNotBiggerThanString)
{
    StringBuf x("test");

    EXPECT_EQ(StringBuf::EMPTY(), x.rightPart(0));
    EXPECT_EQ(StringBuf("t"), x.rightPart(1));
    EXPECT_EQ(StringBuf("st"), x.rightPart(2));
    EXPECT_EQ(StringBuf("est"), x.rightPart(3));
    EXPECT_EQ(StringBuf("test"), x.rightPart(4));
    EXPECT_EQ(StringBuf("test"), x.rightPart(5));
    EXPECT_EQ(StringBuf("test"), x.rightPart(100));
}

TEST(StringBuf, EmptySubString)
{
    StringBuf x("test");
    EXPECT_EQ(StringBuf(), x.subString(0,0));
}

TEST(StringBuf, SubStringOutOfRangeIsEmpty)
{
    StringBuf x("test");
    EXPECT_EQ(StringBuf(), x.subString(100));
}

TEST(StringBuf, SubStringRangeIsClamped)
{
    StringBuf x("test");
    EXPECT_EQ(StringBuf("st"), x.subString(2,100));
}

TEST(StringBuf, SubStringGenericTest)
{
    StringBuf x("test");
    EXPECT_EQ(StringBuf("es"), x.subString(1, 2));
    EXPECT_EQ(StringBuf("e"), x.subString(1, 1));
    EXPECT_EQ(StringBuf("t"), x.subString(0, 1));
    EXPECT_EQ(StringBuf("t"), x.subString(3, 1));
    EXPECT_EQ(StringBuf("test"), x.subString(0, 4));
    EXPECT_EQ(StringBuf("test"), x.subString(0));
}

TEST(StringBuf, SplitMiddle)
{
    StringBuf x("test");

    StringBuf left, right;
    x.split(2, left, right);

    EXPECT_EQ(StringBuf("te"), left);
    EXPECT_EQ(StringBuf("st"), right);
}

TEST(StringBuf, SplitAtZero)
{
    StringBuf x("test");

    StringBuf left, right;
    x.split(0, left, right);

    EXPECT_EQ(StringBuf(""), left);
    EXPECT_EQ(StringBuf("test"), right);
}

TEST(StringBuf, SplitAtEnd)
{
    StringBuf x("test");

    StringBuf left, right;
    x.split(4, left, right);

    EXPECT_EQ(StringBuf("test"), left);
    EXPECT_EQ(StringBuf(""), right);
}

TEST(StringBuf, SplitOutOfRange)
{
    StringBuf x("test");

    StringBuf left, right;
    x.split(100, left, right);

    EXPECT_EQ(StringBuf("test"), left);
    EXPECT_EQ(StringBuf(""), right);
}

TEST(StringBuf, FindFirstChar)
{
    StringBuf x("HelloWorld");

    EXPECT_EQ(2, x.findFirstChar('l'));
    EXPECT_EQ(1, x.findFirstChar('e'));
    EXPECT_EQ(-1, x.findFirstChar('z'));
}

TEST(StringBuf, FindLastChar)
{
    StringBuf x("HelloWorld");

    EXPECT_EQ(8, x.findLastChar('l'));
    EXPECT_EQ(1, x.findLastChar('e'));
    EXPECT_EQ(-1, x.findLastChar('z'));
}

TEST(StringBuf, FindString)
{
    StringBuf x("HelloWorldhello");

    EXPECT_EQ(5, x.findStr("World"));
}

TEST(StringBuf, FindStringWithOffset)
{
    StringBuf x("HelloWorldhello");

    EXPECT_EQ(1, x.findStr("ello", 0));
    EXPECT_EQ(11, x.findStr("ello", 2));
}

TEST(StringBuf, FindStringRev)
{
    StringBuf x("HelloWorldhello");

    EXPECT_EQ(11, x.findStrRev("ello"));
}

TEST(StringBuf, FindStringRevWithOffset)
{
    StringBuf x("HelloWorldhello");

    EXPECT_EQ(1, x.findStrRev("ello", 5));
}

TEST(StringBuf, BeginsWith)
{
    StringBuf x("HelloWorldhello");

    EXPECT_TRUE(x.beginsWith(""));
    EXPECT_TRUE(x.beginsWith("Hello"));
    EXPECT_FALSE(x.beginsWith("ello"));
    EXPECT_FALSE(x.beginsWith("hello"));
    EXPECT_TRUE(x.beginsWith("HelloWorldhello"));
    EXPECT_FALSE(x.beginsWith("HelloWorldhello2"));
}

TEST(StringBuf, BeginsWithNoCase)
{
    StringBuf x("HelloWorldhello");

    EXPECT_TRUE(x.beginsWithNoCase(""));
    EXPECT_TRUE(x.beginsWithNoCase("Hello"));
    EXPECT_FALSE(x.beginsWithNoCase("ello"));
    EXPECT_TRUE(x.beginsWithNoCase("hello"));
    EXPECT_TRUE(x.beginsWithNoCase("HELLOWORLDHELLO"));
    EXPECT_FALSE(x.beginsWithNoCase("HelloWorldhello2"));
}

TEST(StringBuf, EndsWith)
{
    StringBuf x("HelloWorldhello");

    EXPECT_TRUE(x.endsWith(""));
    EXPECT_TRUE(x.endsWith("hello"));
    EXPECT_TRUE(x.endsWith("ello"));
    EXPECT_FALSE(x.endsWith("ell"));
    EXPECT_FALSE(x.endsWith("Hello"));
    EXPECT_TRUE(x.endsWith("HelloWorldhello"));
    EXPECT_FALSE(x.endsWith("2HelloWorldhello"));
}

TEST(StringBuf, EndsWithNoCase)
{
    StringBuf x("HelloWorldhello");

    EXPECT_TRUE(x.endsWithNoCase(""));
    EXPECT_TRUE(x.endsWithNoCase("Hello"));
    EXPECT_TRUE(x.endsWithNoCase("ELLO"));
}

TEST(StringBuf, StringAfterFirst)
{
    StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(StringBuf("_kota_Ala_ma"), x.stringAfterFirst("ma"));
    EXPECT_EQ(StringBuf("_Ala_ma"), x.stringAfterFirst("kota"));
    EXPECT_TRUE(x.stringAfterFirst("dupa").empty());
    EXPECT_TRUE(x.stringAfterFirst("MA").empty());
}

TEST(StringBuf, StringAfterFirstNoCase)
{
    StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(StringBuf("_kota_Ala_ma"), x.stringAfterFirstNoCase("MA"));
    EXPECT_EQ(StringBuf("_ma_kota_Ala_ma"), x.stringAfterFirstNoCase("ala"));
}

TEST(StringBuf, StringBeforeFirst)
{
    StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(StringBuf("Ala_"), x.stringBeforeFirst("ma"));
    EXPECT_TRUE(x.stringBeforeFirst("MA").empty());
    EXPECT_EQ(StringBuf("Ala_ma_"), x.stringBeforeFirst("kota"));
}

TEST(StringBuf, StringBeforeFirstNoCase)
{
    StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(StringBuf("Ala_"), x.stringBeforeFirstNoCase("MA"));
}

TEST(StringBuf, StringAfterLast)
{
    StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_TRUE(x.stringAfterLast("ma").empty());
    EXPECT_EQ(StringBuf("_ma"), x.stringAfterLast("Ala"));
    EXPECT_TRUE(x.stringAfterLast("dupa").empty());
}

TEST(StringBuf, StringAfterLastNoCase)
{
    StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(StringBuf("_ma"), x.stringAfterLastNoCase("ala"));
    EXPECT_TRUE(x.stringAfterLastNoCase("dupa").empty());
}

TEST(StringBuf, StringBeforeLast)
{
    StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_TRUE(x.stringBeforeLast("ala").empty());
    EXPECT_TRUE(x.stringBeforeLast("Ala") == "Ala_ma_kota_");
    EXPECT_TRUE(x.stringBeforeLast("kota") == "Ala_ma_");
    EXPECT_TRUE(x.stringBeforeLast("ma") == "Ala_ma_kota_Ala_");
}


TEST(StringBuf, StringBeforeLastNoCase)
{
    StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(StringBuf("Ala_ma_kota_"), x.stringBeforeLastNoCase("ala"));
    EXPECT_EQ(StringBuf("Ala_ma_"),  x.stringBeforeLastNoCase("KOTA"));
}

END_BOOMER_NAMESPACE()
