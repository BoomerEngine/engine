/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "base/object/include/rttiType.h"
#include "base/test/include/gtest/gtest.h"
#include "base/object/include/object.h"
#include "base/object/include/rttiClassRef.h"

DECLARE_TEST_FILE(ToStringFromString);

namespace base
{

    TEST(RttiToString, SimpleTypes)
    {
        {
            TempString txt;
            PrintToString<char>(txt, 42);
            EXPECT_STREQ("42", txt.c_str());
        }

        {
            TempString txt;
            PrintToString<int64_t>(txt, 1234567890123456);
            EXPECT_STREQ("1234567890123456", txt.c_str());
        }

        {
            TempString txt;
            PrintToString<int64_t>(txt, -1234567890123456);
            EXPECT_STREQ("-1234567890123456", txt.c_str());
        }

        {
            TempString txt;
            PrintToString<bool>(txt, true);
            EXPECT_STREQ("true", txt.c_str());
        }

        {
            TempString txt;
            PrintToString<bool>(txt, false);
            EXPECT_STREQ("false", txt.c_str());
        }
    }

    template<typename T>
    void PrintCString(IFormatStream& f, const T& txt, bool willBeSentInQuotes=false)
    {
        f.appendCEscaped(txt, ARRAY_COUNT(txt) - 1, willBeSentInQuotes);
    }

    TEST(RttiToString, StringCEncodeBasic)
    {
        TempString txt;
        PrintCString(txt, "Ala ma kota");
        EXPECT_STREQ("Ala ma kota", txt.c_str());
    }

    TEST(RttiToString, StringCEncodeSimpleEscapes)
    {
        TempString txt;
        PrintCString(txt, "Ala ma kota\nnext line\r\nWindows line");
        EXPECT_STREQ("Ala ma kota\\nnext line\\r\\nWindows line", txt.c_str());
    }

    TEST(RttiToString, StringCEncodeZeros)
    {
        TempString txt;
        PrintCString(txt, "Ala ma kota\0Hidden KOT\0\0");
        EXPECT_STREQ("Ala ma kota\\0Hidden KOT\\0\\0", txt.c_str());
    }

    TEST(RttiToString, StringCEncodeQuoteNotEncodedByDefault)
    {
        TempString txt;
        PrintCString(txt, "Ala ma \"kota\"");
        EXPECT_STREQ("Ala ma \"kota\"", txt.c_str());
    }

    TEST(RttiToString, StringCEncodeQuoteWhenInQuotes)
    {
        TempString txt;
        PrintCString(txt, "Ala ma \"kota\"", true);
        EXPECT_STREQ("Ala ma \\\"kota\\\"", txt.c_str());
    }

    TEST(RttiToString, StringBufSimple)
    {
        TempString txt;
        PrintToString<StringBuf>(txt, StringBuf("Ala ma kota"));
        EXPECT_STREQ("Ala ma kota", txt.c_str());
    }

    TEST(RttiToString, StringBufWithSpecialChars)
    {
        TempString txt;
        PrintToString<StringBuf>(txt, StringBuf("This\nis\nmulti\nline!\n"));
        EXPECT_STREQ("This\\nis\\nmulti\\nline!\\n", txt.c_str());
    }

    TEST(RttiToString, StringIDSimple)
    {
        TempString txt;
        PrintToString<StringID>(txt, StringID("Ala ma kota"));
        EXPECT_STREQ("Ala ma kota", txt.c_str());
    }

    TEST(RttiToString, StringFromDynamicCharArray)
    {
        Array<char> str;
        str.pushBack('A');
        str.pushBack('l');
        str.pushBack('a');
        str.pushBack(' ');
        str.pushBack('m');
        str.pushBack('a');
        str.pushBack(' ');
        str.pushBack('k');
        str.pushBack('o');
        str.pushBack('t');
        str.pushBack('a');

        TempString txt;
        PrintToString(txt, str);
        EXPECT_STREQ("Ala ma kota", txt.c_str());
    }


    TEST(RttiToString, StringFromStaticCharArray)
    {
        TempString txt;
        PrintToString(txt, "Ala ma kota");
        EXPECT_STREQ("Ala ma kota", txt.c_str());
    }

    TEST(RttiToString, DynamicArrayEmpty)
    {
        TempString txt;
        Array<int> arr;
        PrintToString(txt, arr);
        EXPECT_STREQ("", txt.c_str());
    }

    TEST(RttiToString, DynamicArraySimple)
    {
        TempString txt;
        Array<int> arr;
        arr.pushBack(1);
        arr.pushBack(2);
        arr.pushBack(3);
        PrintToString(txt, arr);
        EXPECT_STREQ("[1][2][3]", txt.c_str());
    }

    TEST(RttiToString, DynamicArrayRecursive)
    {
        TempString txt;
        Array< Array<int>> arr;
        arr.emplaceBack();
        arr.back().pushBack(1);
        arr.back().pushBack(2);
        arr.back().pushBack(3);
        arr.emplaceBack();
        arr.back().pushBack(4);
        arr.back().pushBack(5);
        arr.back().pushBack(6);
        arr.emplaceBack();
        arr.back().pushBack(7);
        arr.back().pushBack(8);
        arr.back().pushBack(9);
        PrintToString(txt, arr);
        EXPECT_STREQ("[[1][2][3]][[4][5][6]][[7][8][9]]", txt.c_str());
    }

    TEST(RttiToString, DynamicArrayOfStrings)
    {
        TempString txt;
        Array<StringBuf> arr;
        arr.pushBack(StringBuf("Ala"));
        arr.pushBack(StringBuf("ma"));
        arr.pushBack(StringBuf("kota"));
        PrintToString(txt, arr);
        EXPECT_STREQ("[Ala][ma][kota]", txt.c_str());
    }

    TEST(RttiToString, DynamicArrayOfStringsWithEscapement)
    {
        TempString txt;
        Array<StringBuf> arr;
        arr.pushBack(StringBuf("Ala"));
        arr.pushBack(StringBuf("line\nline\n"));
        arr.pushBack(StringBuf("kota"));
        PrintToString(txt, arr);
        EXPECT_STREQ("[Ala][line\\nline\\n][kota]", txt.c_str());
    }

    TEST(RttiToString, DynamicArrayOfStringsWithEscapementNoQuotes)
    {
        TempString txt;
        Array<StringBuf> arr;
        arr.emplaceBack("Ala");
        arr.emplaceBack("\"ma\"");
        arr.emplaceBack("kota");
        PrintToString(txt, arr);
        EXPECT_STREQ("[Ala][\"\\\"ma\\\"\"][kota]", txt.c_str());
    }

    TEST(RttiToString, DynamicArrayOfEmptyStrings)
    {
        TempString txt;
        Array<StringBuf> arr;
        arr.emplaceBack("");
        arr.emplaceBack("");
        arr.emplaceBack("");
        PrintToString(txt, arr);
        EXPECT_STREQ("[][][]", txt.c_str());
    }

    TEST(RttiToString, NativeArraySimple)
    {
        TempString txt;
        int arr[5] = { 1,2,3,4,5 };
        PrintToString(txt, arr);
        EXPECT_STREQ("[1][2][3][4][5]", txt.c_str());
    }

    namespace test
    {
        struct SimpleStruct
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SimpleStruct);

        public:
            int x = 0, y = 0, z=0;
        };

        RTTI_BEGIN_TYPE_CLASS(SimpleStruct);
        RTTI_PROPERTY(x);
        RTTI_PROPERTY(y);
        RTTI_PROPERTY(z);
        RTTI_END_TYPE();

        struct NestedStruct
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(NestedStruct);

        public:
            SimpleStruct min, max;
        };

        RTTI_BEGIN_TYPE_CLASS(NestedStruct);
        RTTI_PROPERTY(min);
        RTTI_PROPERTY(max);
        RTTI_END_TYPE();

        struct StructWithStrings
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(StructWithStrings);

        public:
            StringBuf a, b, c;

            StructWithStrings()
            {
                b = StringBuf("non-empty");
            }
        };

        RTTI_BEGIN_TYPE_CLASS(StructWithStrings);
        RTTI_PROPERTY(a);
        RTTI_PROPERTY(b);
        RTTI_PROPERTY(c);
        RTTI_END_TYPE();

        struct NestedNode
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(NestedNode);

        public:
            int payload;
            Array<NestedNode> inners;
        };

        RTTI_BEGIN_TYPE_CLASS(NestedNode);
        RTTI_PROPERTY(payload);
        RTTI_PROPERTY(inners);
        RTTI_END_TYPE();

    } // test

    TEST(RttiToString, SimpleStruct)
    {
        test::SimpleStruct data;
        data.x = 1.0f;
        data.y = 2.0f;
        data.z = 3.0f;

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("(x=1)(y=2)(z=3)", txt.c_str());
    }

    TEST(RttiToString, SimpleStructDefaultsNotSaved)
    {
        test::SimpleStruct data;
        data.z = -3.0f;

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("(z=-3)", txt.c_str());
    }

    TEST(RttiToString, SimpleStructEmptyNotSavedAtAll)
    {
        test::SimpleStruct data;

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("", txt.c_str());
    }

    TEST(RttiToString, NestedStruct)
    {
        test::NestedStruct data;
        data.min.x = -1;
        data.min.y = -2;
        data.min.z = -3;
        data.max.x = 1;
        data.max.y = 2;
        data.max.z = 3;

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("(min=(x=-1)(y=-2)(z=-3))(max=(x=1)(y=2)(z=3))", txt.c_str());
    }

    TEST(RttiToString, NestedStructWithDefaults)
    {
        test::NestedStruct data;
        data.min.z = -3;
        data.max.y = 2;

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("(min=(z=-3))(max=(y=2))", txt.c_str());
    }

    TEST(RttiToString, NestedStructWithEmptyMembers)
    {
        test::NestedStruct data;

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("", txt.c_str());
    }

    TEST(RttiToString, StructWithStrings)
    {
        test::StructWithStrings data;
        data.a = StringBuf("Ala");
        data.b = StringBuf("ma");
        data.c = StringBuf("kota");

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("(a=Ala)(b=ma)(c=kota)", txt.c_str());
    }

    TEST(RttiToString, StructWithStringsEscaped)
    {
        test::StructWithStrings data;
        data.a = StringBuf("Ala");
        data.b = StringBuf("ma");
        data.c = StringBuf("(((kota)))");

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("(a=Ala)(b=ma)(c=\"(((kota)))\")", txt.c_str());
    }

    TEST(RttiToString, StructWithStringsEscapedQuotes)
    {
        test::StructWithStrings data;
        data.a = StringBuf("Ala");
        data.b = StringBuf("\"ma\"");
        data.c = StringBuf("kota");

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("(a=Ala)(b=\"\\\"ma\\\"\")(c=kota)", txt.c_str());
    }

    TEST(RttiToString, StructWithEmptyStrings)
    {
        test::StructWithStrings data;
        data.b = StringBuf("");

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("(b=)", txt.c_str());
    }

    TEST(RttiToString, NestedTree)
    {
        TempString txt;

        test::NestedNode n;
        n.payload = 1.0f;
        n.inners.emplaceBack().payload = 2.0f;
        n.inners.emplaceBack().payload = 3.0f;
        n.inners.back().inners.emplaceBack().payload = -1.0f;
        n.inners.back().inners.emplaceBack().payload = -2.0f;
        n.inners.emplaceBack().payload = 4.0f;
        PrintToString(txt, n);
        EXPECT_STREQ("(payload=1)(inners=[(payload=2)][(payload=3)(inners=[(payload=-1)][(payload=-2)])][(payload=4)])", txt.c_str());
    }

    TEST(RttiToString, ClassRef)
    {
        TempString txt;

        SpecificClassType<IObject> c;
        c = IObject::GetStaticClass();

        PrintToString(txt, c);
        EXPECT_STREQ("IObject", txt.c_str());
    }

    TEST(RttiToString, ClassRefEmpty)
    {
        TempString txt;

        SpecificClassType<IObject> c;

        PrintToString(txt, c);
        EXPECT_STREQ("null", txt.c_str());
    }

    //--

    TEST(RttiFromString, SimpleTypes)
    {
        {
            char val = 0;
            EXPECT_TRUE(ParseFromString("42", val));
            EXPECT_EQ(42, val);
        }

        {
            int64_t val = 0;
            EXPECT_TRUE(ParseFromString("1234567890123456", val));
            EXPECT_EQ(1234567890123456, val);
        }

        {
            int val = 0;
            EXPECT_FALSE(ParseFromString("1234567890123456", val));
            EXPECT_EQ(0, val);
        }

        {
            uint32_t val = 0;
            EXPECT_TRUE(ParseFromString("4123123123", val));
            EXPECT_EQ(4123123123, val);
        }

        {
            uint32_t val = 0;
            EXPECT_FALSE(ParseFromString("-100", val));
            EXPECT_EQ(0, val);
        }

        {
            bool val = true;
            EXPECT_TRUE(ParseFromString("false", val));
            EXPECT_EQ(false, val);
        }

        {
            bool val = false;
            EXPECT_TRUE(ParseFromString("true", val));
            EXPECT_EQ(true, val);
        }
    }

    TEST(RttiFromString, IntoStaticStringBuffer)
    {
        char buffer[50];
        memset(buffer, 0xCC, sizeof(buffer));

        EXPECT_TRUE(ParseFromString("Ala ma kota", buffer));
        EXPECT_STREQ("Ala ma kota", buffer);
        EXPECT_EQ(0, buffer[30]); // rest should be filled to zeros
    }

    TEST(RttiFromString, IntoStaticStringBufferSmallSize)
    {
        char buffer[3];
        memset(buffer, 0xCC, sizeof(buffer));

        EXPECT_TRUE(ParseFromString("Ala ma kota", buffer));
        EXPECT_EQ('A', buffer[0]);
        EXPECT_EQ('l', buffer[1]);
        EXPECT_EQ(0, buffer[2]);
    }

    TEST(RttiFromString, StringBufSimple)
    {
        StringBuf data;
        EXPECT_TRUE(ParseFromString("Ala ma kota", data));
        EXPECT_STREQ("Ala ma kota", data.c_str());
    }

    TEST(RttiFromString, StringBufWithSpecialChars)
    {
        StringBuf data;
        EXPECT_TRUE(ParseFromString("This\\nis\\nmulti\\nline!\\n", data));
        EXPECT_STREQ("This\nis\nmulti\nline!\n", data.c_str());
    }

    TEST(RttiFromString, StringIDSimple)
    {
        StringID data;
        EXPECT_TRUE(ParseFromString("Ala ma kota", data));
        EXPECT_STREQ("Ala ma kota", data.c_str());
    }

    TEST(RttiFromString, StringFromDynamicCharArray)
    {
        Array<char> data;
        EXPECT_TRUE(ParseFromString("Ala ma kota", data));
        ASSERT_EQ(11, data.size());
        EXPECT_EQ('A', data[0]);
        EXPECT_EQ('l', data[1]);
        EXPECT_EQ('a', data[2]);
        EXPECT_EQ(' ', data[3]);
        EXPECT_EQ('m', data[4]);
        EXPECT_EQ('a', data[5]);
        EXPECT_EQ(' ', data[6]);
        EXPECT_EQ('k', data[7]);
        EXPECT_EQ('o', data[8]);
        EXPECT_EQ('t', data[9]);
        EXPECT_EQ('a', data[10]);
    }

    TEST(RttiFromString, DynamicArrayEmpty)
    {
        Array<int> arr;
        EXPECT_TRUE(ParseFromString("", arr));
        EXPECT_EQ(0, arr.size());
    }

    TEST(RttiFromString, DynamicArrayClearedWhenEmpty)
    {
        Array<int> arr;
        arr.pushBack(1);
        EXPECT_TRUE(ParseFromString("", arr));
        EXPECT_EQ(0, arr.size());
    }

    TEST(RttiFromString, DynamicArraySimple)
    {
        Array<int> arr;
        EXPECT_TRUE(ParseFromString("[1][2][3]", arr));
        ASSERT_EQ(3, arr.size());
        EXPECT_EQ(1, arr[0]);
        EXPECT_EQ(2, arr[1]);
        EXPECT_EQ(3, arr[2]);        
    }

    TEST(RttiFromString, DynamicArrayRecursive)
    {
        Array< Array<int>> arr;
        EXPECT_TRUE(ParseFromString("[[1][2][3]][[4][5][6]][[7][8][9]]", arr));
        ASSERT_EQ(3, arr.size());
        ASSERT_EQ(3, arr[0].size());
        ASSERT_EQ(3, arr[0].size());
        ASSERT_EQ(3, arr[0].size());
        EXPECT_EQ(1, arr[0][0]);
        EXPECT_EQ(2, arr[0][1]);
        EXPECT_EQ(3, arr[0][2]);
        EXPECT_EQ(4, arr[1][0]);
        EXPECT_EQ(5, arr[1][1]);
        EXPECT_EQ(6, arr[1][2]);
        EXPECT_EQ(7, arr[2][0]);
        EXPECT_EQ(8, arr[2][1]);
        EXPECT_EQ(9, arr[2][2]);
    }

    TEST(RttiFromString, DynamicArrayOfStrings)
    {
        Array<StringBuf> arr;
        EXPECT_TRUE(ParseFromString("[Ala][ma][kota]", arr));
        ASSERT_EQ(3, arr.size());
        EXPECT_STREQ("Ala", arr[0].c_str());
        EXPECT_STREQ("ma", arr[1].c_str());
        EXPECT_STREQ("kota", arr[2].c_str());
    }

    TEST(RttiFromString, DynamicArrayOfStringsWithEscapement)
    {
        Array<StringBuf> arr;
        EXPECT_TRUE(ParseFromString("[Ala][line\\nline\\n][kota]", arr));
        ASSERT_EQ(3, arr.size());
        EXPECT_STREQ("Ala", arr[0].c_str());
        EXPECT_STREQ("line\nline\n", arr[1].c_str());
        EXPECT_STREQ("kota", arr[2].c_str());
    }

    TEST(RttiFromString, DynamicArrayOfStringsWithEscapementNoQuotes)
    {
        Array<StringBuf> arr;
        EXPECT_TRUE(ParseFromString("[Ala][\"\\\"ma\\\"\"][kota]", arr));
        ASSERT_EQ(3, arr.size());
        EXPECT_STREQ("Ala", arr[0].c_str());
        EXPECT_STREQ("\"ma\"", arr[1].c_str());
        EXPECT_STREQ("kota", arr[2].c_str());
    }

    TEST(RttiFromString, DynamicArrayOfEmptyStrings)
    {
        Array<StringBuf> arr;
        EXPECT_TRUE(ParseFromString("[][][]", arr));
        ASSERT_EQ(3, arr.size());
        EXPECT_STREQ("", arr[0].c_str());
        EXPECT_STREQ("", arr[1].c_str());
        EXPECT_STREQ("", arr[2].c_str());
    }

    TEST(RttiFromString, DynamicArrayOfEmptyValuesDoesNotParse)
    {
        Array<int> arr;
        EXPECT_FALSE(ParseFromString("[][][]", arr));
    }

    TEST(RttiFromString, DynamicArrayOfInvalidValuesDoesNotParse)
    {
        Array<uint32_t> arr;
        EXPECT_FALSE(ParseFromString("[1][2][-1]", arr));
    }

    TEST(RttiFromString, DynamicArrayFailedParseDoesNotResize)
    {
        Array<int> arr;
        arr.resizeWith(5, -1);
        EXPECT_FALSE(ParseFromString("[][][", arr));
        EXPECT_EQ(5, arr.size());
    }

    TEST(RttiFromString, NativeArraySimple)
    {
        int arr[5] = { 0,0,0,0,0 };
        EXPECT_TRUE(ParseFromString("[1][2][3][4][5]", arr));
        EXPECT_EQ(1, arr[0]);
        EXPECT_EQ(2, arr[1]);
        EXPECT_EQ(3, arr[2]);
        EXPECT_EQ(4, arr[3]);
        EXPECT_EQ(5, arr[4]);
    }

    TEST(RttiFromString, NativeArraySimpleNotEnoughElements)
    {
        int arr[5] = { -1,-1,-1,-1,-1 };
        EXPECT_TRUE(ParseFromString("[1][2][3]", arr));
        EXPECT_EQ(1, arr[0]);
        EXPECT_EQ(2, arr[1]);
        EXPECT_EQ(3, arr[2]);
        EXPECT_EQ(0, arr[3]);
        EXPECT_EQ(0, arr[4]);
    }

    TEST(RttiFromString, NativeArraySimpleToManyElements)
    {
        int arr[5] = { -1,-1,-1,-1,-1 };
        EXPECT_TRUE(ParseFromString("[1][2][3][4][5][6][7][8][9]", arr));
        EXPECT_EQ(1, arr[0]);
        EXPECT_EQ(2, arr[1]);
        EXPECT_EQ(3, arr[2]);
        EXPECT_EQ(4, arr[3]);
        EXPECT_EQ(5, arr[4]);
    }

    TEST(RttiFromString, DynamicArrayGracefullFailure)
    {
        Array<int> arr;
        EXPECT_FALSE(ParseFromString("[5][", arr));
    }

    TEST(RttiFromString, SimpleStruct)
    {
        test::SimpleStruct data;
        EXPECT_TRUE(ParseFromString("(x=1)(y=2)(z=3)", data));
        EXPECT_EQ(1, data.x);
        EXPECT_EQ(2, data.y);
        EXPECT_EQ(3, data.z);
    }

    TEST(RttiFromString, SimpleStructUnparsedPropsUnchanged)
    {
        test::SimpleStruct data;
        data.y = 555;
        EXPECT_TRUE(ParseFromString("(x=1)(z=3)", data));
        EXPECT_EQ(1, data.x);
        EXPECT_EQ(555, data.y);
        EXPECT_EQ(3, data.z);
    }

    TEST(RttiFromString, SimpleStructHandlesMissingProps)
    {
        test::SimpleStruct data;
        EXPECT_TRUE(ParseFromString("(x=1)(w=100)(z=3)", data));
        EXPECT_EQ(1, data.x);
        EXPECT_EQ(3, data.z);
    }

    TEST(RttiFromString, SimpleStructHandlesEmpty)
    {
        test::SimpleStruct data;
        EXPECT_TRUE(ParseFromString("", data));
        EXPECT_EQ(0, data.x);
        EXPECT_EQ(0, data.y);
        EXPECT_EQ(0, data.z);
    }

    TEST(RttiFromString, SimpleStructDefaultsNotSaved)
    {
        test::SimpleStruct data;
        data.z = -3.0f;

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("(z=-3)", txt.c_str());
    }

    TEST(RttiFromString, SimpleStructEmptyNotSavedAtAll)
    {
        test::SimpleStruct data;

        TempString txt;
        PrintToString(txt, data);
        EXPECT_STREQ("", txt.c_str());
    }

    TEST(RttiFromString, NestedStruct)
    {
        test::NestedStruct data;
        EXPECT_TRUE(ParseFromString("(min=(x=-1)(y=-2)(z=-3))(max=(x=1)(y=2)(z=3))", data));
        EXPECT_EQ(-1, data.min.x);
        EXPECT_EQ(-2, data.min.y);
        EXPECT_EQ(-3, data.min.z);
        EXPECT_EQ(1, data.max.x);
        EXPECT_EQ(2, data.max.y);
        EXPECT_EQ(3, data.max.z);
    }

    TEST(RttiFromString, NestedStructEmptySubStruct)
    {
        test::NestedStruct data;
        data.min.x = 1;
        data.min.y = 1;
        data.min.z = 1;
        data.max.x = 2;
        data.max.y = 2;
        data.max.z = 2;
        EXPECT_TRUE(ParseFromString("(min=)(max=(y=0))", data));
        EXPECT_EQ(1, data.min.x);
        EXPECT_EQ(1, data.min.y);
        EXPECT_EQ(1, data.min.z);
        EXPECT_EQ(2, data.max.x);
        EXPECT_EQ(0, data.max.y);
        EXPECT_EQ(2, data.max.z);
    }

    TEST(RttiFromString, StructWithStrings)
    {
        test::StructWithStrings data;
        EXPECT_TRUE(ParseFromString("(a=Ala)(b=ma)(c=kota)", data));
        EXPECT_STREQ("Ala", data.a.c_str());
        EXPECT_STREQ("ma", data.b.c_str());
        EXPECT_STREQ("kota", data.c.c_str());
    }

    TEST(RttiFromString, StructWithStringsEscaped)
    {
        test::StructWithStrings data;
        EXPECT_TRUE(ParseFromString("(a=Ala)(b=ma)(c=\"(((kota)))\")", data));
        EXPECT_STREQ("Ala", data.a.c_str());
        EXPECT_STREQ("ma", data.b.c_str());
        EXPECT_STREQ("(((kota)))", data.c.c_str());
    }

    TEST(RttiFromString, StructWithStringsEscapedQuotes)
    {
        test::StructWithStrings data;
        EXPECT_TRUE(ParseFromString("(a=Ala)(b=\"\\\"ma\\\"\")(c=kota)", data));
        EXPECT_STREQ("Ala", data.a.c_str());
        EXPECT_STREQ("\"ma\"", data.b.c_str());
        EXPECT_STREQ("kota", data.c.c_str());
    }

    TEST(RttiFromString, StructWithEmptyStrings)
    {
        test::StructWithStrings data;
        EXPECT_TRUE(ParseFromString("(b=)", data));
        EXPECT_STREQ("", data.b.c_str());
    }

    TEST(RttiFromString, NestedTree)
    {
        test::NestedNode n;
        EXPECT_TRUE(ParseFromString("(payload=1)(inners=[(payload=2)][(payload=3)(inners=[(payload=-1)][(payload=-2)])][(payload=4)])", n));

        EXPECT_EQ(1, n.payload);
        ASSERT_EQ(3, n.inners.size());
        EXPECT_EQ(2, n.inners[0].payload);
        EXPECT_EQ(3, n.inners[1].payload);
        ASSERT_EQ(2, n.inners[1].inners.size());
        EXPECT_EQ(-1, n.inners[1].inners[0].payload);
        EXPECT_EQ(-2, n.inners[1].inners[1].payload);
        EXPECT_EQ(4, n.inners[2].payload);
    }

    TEST(RttiFromString, ClassRef)
    {
        SpecificClassType<IObject> c;
        EXPECT_TRUE(ParseFromString("base::IObject", c));
        EXPECT_EQ(IObject::GetStaticClass(), c.ptr());
    }

    TEST(RttiFromString, ClassRefEmpty)
    {
        SpecificClassType<IObject> c;
        EXPECT_TRUE(ParseFromString("null", c));
        EXPECT_EQ(nullptr, c.ptr());
    }

    TEST(RttiFromString, ClassRefEmptyDoesNotParseFromEmpty)
    {
        SpecificClassType<IObject> c;
        EXPECT_FALSE(ParseFromString("", c));
    }

    TEST(RttiFromString, ClassRefEmptyDoesNotParseFromInvalidClass)
    {
        SpecificClassType<IObject> c;
        EXPECT_FALSE(ParseFromString("dupa", c));
    }

    TEST(RttiFromString, ClassRefEmptyDoesNotChangeWhenNotParsed)
    {
        SpecificClassType<IObject> c;
        c = IObject::GetStaticClass();
        EXPECT_FALSE(ParseFromString("dupa", c));
        EXPECT_EQ(IObject::GetStaticClass(), c.ptr());
    }


    //--

} // base

