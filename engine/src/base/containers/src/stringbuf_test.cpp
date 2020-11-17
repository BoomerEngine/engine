/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "stringBuf.h"

#include "base/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(StringBuf);

namespace testing
{
    template<>
    INLINE ::std::string PrintToString<base::StringBuf>(const base::StringBuf& value)
    {
        return value.c_str();
    }

} // testing

TEST(StringBuf, CreateEmpty)
{
    base::StringBuf x;
    EXPECT_TRUE(x.empty());
}

TEST(StringBuf, EmptyStringHasZeroLength)
{
    base::StringBuf x;
    EXPECT_EQ(0U, x.length());
}

TEST(StringBuf, AllEmptyStringsHasCBuf)
{
    base::StringBuf x, y;
    EXPECT_TRUE(x.c_str());
    EXPECT_EQ(0, x.c_str()[0]);
}

TEST(StringBuf, AllEmptyStringsHaveSameCBuf)
{
    base::StringBuf x, y;
    EXPECT_EQ(x.c_str(), y.c_str());
}

TEST(StringBuf, CreateSimple)
{
    base::StringBuf x("test");
    ASSERT_EQ(4U, x.length());
    EXPECT_EQ('t', x.c_str()[0]);
    EXPECT_EQ('e', x.c_str()[1]);
    EXPECT_EQ('s', x.c_str()[2]);
    EXPECT_EQ('t', x.c_str()[3]);
    EXPECT_EQ(0, x.c_str()[4]);
}

TEST(StringBuf, CopyConstruct)
{
    base::StringBuf x("test");
    base::StringBuf y(x);

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
    base::StringBuf x("test");
    base::StringBuf y(std::move(x));
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
    base::StringBuf x("test");
    base::StringBuf y = x;

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
    base::StringBuf x("test");
    base::StringBuf y = std::move(x);

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
    base::StringBuf x("test");
    base::StringBuf y("test");
    EXPECT_TRUE(x == y);
}

TEST(StringBuf, EqualTestFails)
{
    base::StringBuf x("test");
    base::StringBuf y("dupa");
    EXPECT_FALSE(x == y);
}

TEST(StringBuf, NonEqualTest)
{
    base::StringBuf x("test");
    base::StringBuf y("dupa");
    EXPECT_TRUE(x != y);
}

TEST(StringBuf, NonEqualTestFails)
{
    base::StringBuf x("test");
    base::StringBuf y("test");
    EXPECT_FALSE(x != y);
}

TEST(StringBuf, RawEqualTest)
{
    base::StringBuf x("test");
    auto y  = "test";
    EXPECT_TRUE(x == y);
}

TEST(StringBuf, RawEqualTestFails)
{
    base::StringBuf x("test");
    auto y  = "dupa";
    EXPECT_FALSE(x == y);
}

TEST(StringBuf, RawNonEqualTest)
{
    base::StringBuf x("test");
    auto y  = "dupa";
    EXPECT_TRUE(x != y);
}

TEST(StringBuf, RawNonEqualTestFails)
{
    base::StringBuf x("test");
    auto y  = "test";
    EXPECT_FALSE(x != y);
}

TEST(StringBuf, AppendTest)
{
    base::StringVector x("Ala");
    x.append(" ma ");
    x.append("kota");

    base::StringVector y("Ala ma kota");
    EXPECT_EQ(x, y);
}

TEST(StringBuf, AppendTestRaw)
{
    base::StringVector x("Hello");
    base::StringVector y("World");
    x.append(y);

    base::StringVector z("HelloWorld");
    EXPECT_EQ(z, x);
}

TEST(StringBuf, OrderTest)
{
    base::StringBuf x("Alice");
    base::StringBuf y("Bob");
    EXPECT_TRUE(x < y);
}

TEST(StringBuf, EmptyStringBeforeAnyString)
{
    base::StringBuf x;
    base::StringBuf y(" ");
    EXPECT_TRUE(x < y);
}

TEST(StringBuf, AppendOperator)
{
    base::StringVector x("Hello");
    base::StringVector y("World");
    x += y;
    
    EXPECT_TRUE(x == "HelloWorld");
}

TEST(StringBuf, AppendOperatorRaw)
{
    base::StringVector x("Hello");
    x += "World";

    EXPECT_TRUE(x == "HelloWorld");
}

TEST(StringBuf, ConcatenateOperator)
{
    base::StringVector x("Hello");
    base::StringVector y("World");
    auto z = x + y;

    EXPECT_TRUE(x == "Hello");
    EXPECT_TRUE(y == "World");
    EXPECT_TRUE(z == "HelloWorld");
}

TEST(StringBuf, ConcatenateOperatorRaw)
{
    base::StringVector x("Hello");
    auto z = x + "World";

    EXPECT_TRUE(x == "Hello");
    EXPECT_TRUE(z == "HelloWorld");
}

TEST(StringBuf, Compare)
{
    base::StringBuf x("Bob");

    EXPECT_LT(0, x.compareWith(base::StringBuf::EMPTY()));
    EXPECT_LT(0, x.compareWith(base::StringBuf("Alice")));
    EXPECT_EQ(0, x.compareWith(base::StringBuf("Bob")));
    EXPECT_GT(0, x.compareWith(base::StringBuf("Charles ")));
}

TEST(StringBuf, CompareRaw)
{
    base::StringBuf x("Bob");

    EXPECT_LT(0, x.compareWith(""));
    EXPECT_LT(0, x.compareWith("Alice"));
    EXPECT_EQ(0, x.compareWith("Bob"));
    EXPECT_NE(0, x.compareWith("bob"));
    EXPECT_GT(0, x.compareWith("Charles"));
}

TEST(StringBuf, CompareNoCase)
{
    base::StringBuf x("bob");

    EXPECT_LT(0, x.compareWithNoCase(base::StringBuf::EMPTY()));
    EXPECT_LT(0, x.compareWithNoCase(base::StringBuf("alice")));
    EXPECT_EQ(0, x.compareWithNoCase(base::StringBuf("Bob")));
    EXPECT_EQ(0, x.compareWithNoCase(base::StringBuf("bob")));
    EXPECT_GT(0, x.compareWithNoCase(base::StringBuf("CHARLES")));
}

TEST(StringBuf, CompareNoCaseRaw)
{
    base::StringBuf x("bob");

    EXPECT_LT(0, x.compareWithNoCase(""));
    EXPECT_LT(0, x.compareWithNoCase("Alice"));
    EXPECT_EQ(0, x.compareWithNoCase("Bob"));
    EXPECT_EQ(0, x.compareWithNoCase("bob"));
    EXPECT_GT(0, x.compareWithNoCase("CHARLES"));
}

TEST(StringBuf, CompareN)
{
    base::StringBuf x("testX");

    EXPECT_NE(0, x.compareWith("testY"));
    EXPECT_NE(0, x.compareWith(base::StringView("TESTY", 4)));
    EXPECT_EQ(0, x.leftPart(4).compareWith(base::StringView("testY", 4)));
}

TEST(StringBuf, CompareNoCaseN)
{
    base::StringBuf x("testX");

    EXPECT_NE(0, x.compareWithNoCase("testY"));
    EXPECT_EQ(0, x.leftPart(4).compareWithNoCase(base::StringView("TESTY", 4)));
    EXPECT_EQ(0, x.leftPart(4).compareWithNoCase(base::StringView("testY", 4)));
}

TEST(StringBuf, LeftPartOfEmptyStringIsAlwaysEmpty)
{
    base::StringBuf x;

    EXPECT_EQ(base::StringBuf::EMPTY(), x.leftPart(0));
    EXPECT_EQ(base::StringBuf::EMPTY(), x.leftPart(100));
}

TEST(StringBuf, LeftPartNotBiggerThanString)
{
    base::StringBuf x("test");

    EXPECT_EQ(base::StringBuf::EMPTY(), x.leftPart(0));
    EXPECT_EQ(base::StringBuf("t"), x.leftPart(1));
    EXPECT_EQ(base::StringBuf("te"), x.leftPart(2));
    EXPECT_EQ(base::StringBuf("tes"), x.leftPart(3));
    EXPECT_EQ(base::StringBuf("test"), x.leftPart(4));
    EXPECT_EQ(base::StringBuf("test"), x.leftPart(5));
    EXPECT_EQ(base::StringBuf("test"), x.leftPart(100));
}

TEST(StringBuf, RightPartOfEmptyStringIsAlwaysEmpty)
{
    base::StringBuf x;

    EXPECT_EQ(base::StringBuf::EMPTY(), x.rightPart(0));
    EXPECT_EQ(base::StringBuf::EMPTY(), x.rightPart(100));

}

TEST(StringBuf, RightPartNotBiggerThanString)
{
    base::StringBuf x("test");

    EXPECT_EQ(base::StringBuf::EMPTY(), x.rightPart(0));
    EXPECT_EQ(base::StringBuf("t"), x.rightPart(1));
    EXPECT_EQ(base::StringBuf("st"), x.rightPart(2));
    EXPECT_EQ(base::StringBuf("est"), x.rightPart(3));
    EXPECT_EQ(base::StringBuf("test"), x.rightPart(4));
    EXPECT_EQ(base::StringBuf("test"), x.rightPart(5));
    EXPECT_EQ(base::StringBuf("test"), x.rightPart(100));
}

TEST(StringBuf, EmptySubString)
{
    base::StringBuf x("test");
    EXPECT_EQ(base::StringBuf(), x.subString(0,0));
}

TEST(StringBuf, SubStringOutOfRangeIsEmpty)
{
    base::StringBuf x("test");
    EXPECT_EQ(base::StringBuf(), x.subString(100));
}

TEST(StringBuf, SubStringRangeIsClamped)
{
    base::StringBuf x("test");
    EXPECT_EQ(base::StringBuf("st"), x.subString(2,100));
}

TEST(StringBuf, SubStringGenericTest)
{
    base::StringBuf x("test");
    EXPECT_EQ(base::StringBuf("es"), x.subString(1, 2));
    EXPECT_EQ(base::StringBuf("e"), x.subString(1, 1));
    EXPECT_EQ(base::StringBuf("t"), x.subString(0, 1));
    EXPECT_EQ(base::StringBuf("t"), x.subString(3, 1));
    EXPECT_EQ(base::StringBuf("test"), x.subString(0, 4));
    EXPECT_EQ(base::StringBuf("test"), x.subString(0));
}

TEST(StringBuf, SplitMiddle)
{
    base::StringBuf x("test");

    base::StringBuf left, right;
    x.split(2, left, right);

    EXPECT_EQ(base::StringBuf("te"), left);
    EXPECT_EQ(base::StringBuf("st"), right);
}

TEST(StringBuf, SplitAtZero)
{
    base::StringBuf x("test");

    base::StringBuf left, right;
    x.split(0, left, right);

    EXPECT_EQ(base::StringBuf(""), left);
    EXPECT_EQ(base::StringBuf("test"), right);
}

TEST(StringBuf, SplitAtEnd)
{
    base::StringBuf x("test");

    base::StringBuf left, right;
    x.split(4, left, right);

    EXPECT_EQ(base::StringBuf("test"), left);
    EXPECT_EQ(base::StringBuf(""), right);
}

TEST(StringBuf, SplitOutOfRange)
{
    base::StringBuf x("test");

    base::StringBuf left, right;
    x.split(100, left, right);

    EXPECT_EQ(base::StringBuf("test"), left);
    EXPECT_EQ(base::StringBuf(""), right);
}

TEST(StringBuf, FindFirstChar)
{
    base::StringBuf x("HelloWorld");

    EXPECT_EQ(2, x.findFirstChar('l'));
    EXPECT_EQ(1, x.findFirstChar('e'));
    EXPECT_EQ(-1, x.findFirstChar('z'));
}

TEST(StringBuf, FindLastChar)
{
    base::StringBuf x("HelloWorld");

    EXPECT_EQ(8, x.findLastChar('l'));
    EXPECT_EQ(1, x.findLastChar('e'));
    EXPECT_EQ(-1, x.findLastChar('z'));
}

TEST(StringBuf, FindString)
{
    base::StringBuf x("HelloWorldhello");

    EXPECT_EQ(5, x.findStr("World"));
}

TEST(StringBuf, FindStringWithOffset)
{
    base::StringBuf x("HelloWorldhello");

    EXPECT_EQ(1, x.findStr("ello", 0));
    EXPECT_EQ(11, x.findStr("ello", 2));
}

TEST(StringBuf, FindStringRev)
{
    base::StringBuf x("HelloWorldhello");

    EXPECT_EQ(11, x.findStrRev("ello"));
}

TEST(StringBuf, FindStringRevWithOffset)
{
    base::StringBuf x("HelloWorldhello");

    EXPECT_EQ(1, x.findStrRev("ello", 5));
}

TEST(StringBuf, BeginsWith)
{
    base::StringBuf x("HelloWorldhello");

    EXPECT_TRUE(x.beginsWith(""));
    EXPECT_TRUE(x.beginsWith("Hello"));
    EXPECT_FALSE(x.beginsWith("ello"));
    EXPECT_FALSE(x.beginsWith("hello"));
    EXPECT_TRUE(x.beginsWith("HelloWorldhello"));
    EXPECT_FALSE(x.beginsWith("HelloWorldhello2"));
}

TEST(StringBuf, BeginsWithNoCase)
{
    base::StringBuf x("HelloWorldhello");

    EXPECT_TRUE(x.beginsWithNoCase(""));
    EXPECT_TRUE(x.beginsWithNoCase("Hello"));
    EXPECT_FALSE(x.beginsWithNoCase("ello"));
    EXPECT_TRUE(x.beginsWithNoCase("hello"));
    EXPECT_TRUE(x.beginsWithNoCase("HELLOWORLDHELLO"));
    EXPECT_FALSE(x.beginsWithNoCase("HelloWorldhello2"));
}

TEST(StringBuf, EndsWith)
{
    base::StringBuf x("HelloWorldhello");

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
    base::StringBuf x("HelloWorldhello");

    EXPECT_TRUE(x.endsWithNoCase(""));
    EXPECT_TRUE(x.endsWithNoCase("Hello"));
    EXPECT_TRUE(x.endsWithNoCase("ELLO"));
}

TEST(StringBuf, StringAfterFirst)
{
    base::StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(base::StringBuf("_kota_Ala_ma"), x.stringAfterFirst("ma"));
    EXPECT_EQ(base::StringBuf("_Ala_ma"), x.stringAfterFirst("kota"));
    EXPECT_TRUE(x.stringAfterFirst("dupa").empty());
    EXPECT_TRUE(x.stringAfterFirst("MA").empty());
}

TEST(StringBuf, StringAfterFirstNoCase)
{
    base::StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(base::StringBuf("_kota_Ala_ma"), x.stringAfterFirstNoCase("MA"));
    EXPECT_EQ(base::StringBuf("_ma_kota_Ala_ma"), x.stringAfterFirstNoCase("ala"));
}

TEST(StringBuf, StringBeforeFirst)
{
    base::StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(base::StringBuf("Ala_"), x.stringBeforeFirst("ma"));
    EXPECT_TRUE(x.stringBeforeFirst("MA").empty());
    EXPECT_EQ(base::StringBuf("Ala_ma_"), x.stringBeforeFirst("kota"));
}

TEST(StringBuf, StringBeforeFirstNoCase)
{
    base::StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(base::StringBuf("Ala_"), x.stringBeforeFirstNoCase("MA"));
}

TEST(StringBuf, StringAfterLast)
{
    base::StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_TRUE(x.stringAfterLast("ma").empty());
    EXPECT_EQ(base::StringBuf("_ma"), x.stringAfterLast("Ala"));
    EXPECT_TRUE(x.stringAfterLast("dupa").empty());
}

TEST(StringBuf, StringAfterLastNoCase)
{
    base::StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(base::StringBuf("_ma"), x.stringAfterLastNoCase("ala"));
    EXPECT_TRUE(x.stringAfterLastNoCase("dupa").empty());
}

TEST(StringBuf, StringBeforeLast)
{
    base::StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_TRUE(x.stringBeforeLast("ala").empty());
    EXPECT_TRUE(x.stringBeforeLast("Ala") == "Ala_ma_kota_");
    EXPECT_TRUE(x.stringBeforeLast("kota") == "Ala_ma_");
    EXPECT_TRUE(x.stringBeforeLast("ma") == "Ala_ma_kota_Ala_");
}


TEST(StringBuf, StringBeforeLastNoCase)
{
    base::StringBuf x("Ala_ma_kota_Ala_ma");
    EXPECT_EQ(base::StringBuf("Ala_ma_kota_"), x.stringBeforeLastNoCase("ala"));
    EXPECT_EQ(base::StringBuf("Ala_ma_"),  x.stringBeforeLastNoCase("KOTA"));
}
