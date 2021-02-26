/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "core/test/include/gtest/gtest.h"
#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

DECLARE_TEST_FILE(ResourcePathTest);

/*
TEST(ResourcePathTest, EmptyPathEmptyString)
{
    ResourcePath path;
    StringBuilder txt;
    txt << path;
    EXPECT_STREQ("", txt.c_str());
}

TEST(ResourcePathTest, PathFileName)
{
    ResourcePath path("dupa.txt");
    EXPECT_STREQ("dupa.txt", StringBuf(path.data()->fileNamePart).c_str());
    EXPECT_STREQ("txt", StringBuf(path.data()->extensionPart).c_str());
    EXPECT_STREQ("dupa", StringBuf(path.data()->fileStemPart).c_str());
}

TEST(ResourcePathTest, PathFileNoExtension)
{
    ResourcePath path("dupa");
    EXPECT_STREQ("dupa", StringBuf(path.data()->fileNamePart).c_str());
    EXPECT_STREQ("", StringBuf(path.data()->extensionPart).c_str());
    EXPECT_STREQ("dupa", StringBuf(path.data()->fileStemPart).c_str());
}

TEST(ResourcePathTest, PathFileMultipleExtensions)
{
    ResourcePath path("dupa.txt.manifest");
    EXPECT_STREQ("dupa.txt.manifest", StringBuf(path.data()->fileNamePart).c_str());
    EXPECT_STREQ("manifest", StringBuf(path.data()->extensionPart).c_str());
    EXPECT_STREQ("dupa.txt", StringBuf(path.data()->fileStemPart).c_str());
}

TEST(ResourcePathTest, PathInitialPathSeparatorIgnored)
{
    ResourcePath path("/dupa.txt");
    EXPECT_STREQ("", StringBuf(path.data()->dirPart).c_str());
    EXPECT_STREQ("dupa.txt", StringBuf(path.data()->fileNamePart).c_str());
    EXPECT_STREQ("txt", StringBuf(path.data()->extensionPart).c_str());
    EXPECT_STREQ("dupa", StringBuf(path.data()->fileStemPart).c_str());
}

TEST(ResourcePathTest, PathDirPathExtracted)
{
    ResourcePath path("/engine/textures/lena.png");
    EXPECT_STREQ("/engine/textures/", StringBuf(path.data()->dirPart).c_str());
    EXPECT_STREQ("lena.png", StringBuf(path.data()->fileNamePart).c_str());
    EXPECT_STREQ("png", StringBuf(path.data()->extensionPart).c_str());
    EXPECT_STREQ("lena", StringBuf(path.data()->fileStemPart).c_str());
}

TEST(ResourcePathTest, PathDirPathSlashesFixed)
{
    ResourcePath path("\\engine\\textures\\lena.png");
    EXPECT_STREQ("/engine/textures/", StringBuf(path.data()->dirPart).c_str());
    EXPECT_STREQ("lena.png", StringBuf(path.data()->fileNamePart).c_str());
    EXPECT_STREQ("png", StringBuf(path.data()->extensionPart).c_str());
    EXPECT_STREQ("lena", StringBuf(path.data()->fileStemPart).c_str());
}
*/

/*TEST(ResourcePathTest, UserPathConformed)
{
    ResourcePath path("\x4d\x6f\x6a\x65\x20\x44\x6f\x6b\x75\x6d\x65\x6e\x74\x79\x5c\x5a\x64\x6a\xc4\x99\x63\x69\x61\x5c\x42\xc5\xbc\x64\xc4\x85\x63\x5c\x6b\x6f\xc5\x82\x79\x73\x6b\x61\x2e\x6a\x70\x67");
    EXPECT_STREQ("moje dokumenty/zdj\xc4\x99\x0acia/b\xc5\xbc\x64\xc4\x85\x63", StringBuf(path.data()->dirPart).c_str());
    EXPECT_STREQ("ko\xc5\x82yska.jpg", StringBuf(path.data()->fileNamePart).c_str());
    EXPECT_STREQ("jpg", StringBuf(path.data()->extensionPart).c_str());
    EXPECT_STREQ("ko\xc5\x82yska", StringBuf(path.data()->fileStemPart).c_str());
}*/


#if 0


TEST(AccessPath, SimplePropertyElement)
{
    AccessPath path;
    path = path["dupa"];
    ASSERT_TRUE(path.isStructureElement());
    ASSERT_EQ("dupa"_id, path.node()->m_name);
    ASSERT_EQ(-1, path.node()->m_index);
    ASSERT_EQ(nullptr, path.node()->m_parent);
}

TEST(AccessPath, SimpleArrayElement)
{
    AccessPath path;
    path = path[42];
    ASSERT_TRUE(path.isArrayElement());
    ASSERT_TRUE(path.node()->m_name.empty());
    ASSERT_EQ(42, path.node()->m_index);
    ASSERT_EQ(nullptr, path.node()->m_parent);
}

TEST(AccessPath, PathBuilding)
{
    AccessPath path = AccessPath()["dupa"][42]["color"]["r"];
    ASSERT_FALSE(path.empty());
    ASSERT_EQ(StringBuf("dupa[42].color.r"), TempString("{}", path).c_str());
}

///---

TEST(AccessPathParser, EmptyPath)
{
    AccessPath path;
    AccessPathIterator it(path);
    ASSERT_TRUE(it.endOfPath());
}

TEST(AccessPathParser, EmptyPathHasNoArrayElement)
{
    AccessPath path;
    AccessPathIterator it(path);

    uint32_t arrayIndex = 42;
    ASSERT_FALSE(it.enterArray(arrayIndex));
    ASSERT_EQ(42, arrayIndex);
}

TEST(AccessPathParser, EmptyPathHasNoStructElement)
{
    AccessPath path;
    AccessPathIterator it(path);

    StringID propertyName("test");
    ASSERT_FALSE(it.enterProperty(propertyName));
    ASSERT_EQ("test"_id, propertyName);
}

TEST(AccessPathParser, EmptyPathReturnsEmptyPathOnExtraction)
{
    AccessPath path;
    AccessPathIterator it(path);
    ASSERT_TRUE(it.endOfPath());
    auto extractedPath = it.extractRemainingPath();
    ASSERT_TRUE(extractedPath.empty());
}

TEST(AccessPathParser, ArrayElementExtracted)
{
    AccessPath path = AccessPath()[42];
    AccessPathIterator it(path);
    ASSERT_FALSE(it.endOfPath());

    uint32_t arrayIndex = 0;
    ASSERT_TRUE(it.enterArray(arrayIndex));
    ASSERT_EQ(42, arrayIndex);
    ASSERT_TRUE(it.endOfPath());
}

TEST(AccessPathParser, PropertyNameExtracted)
{
    AccessPath path = AccessPath()["test"];
    AccessPathIterator it(path);
    ASSERT_FALSE(it.endOfPath());

    StringID propertyName("test");
    ASSERT_TRUE(it.enterProperty(propertyName));
    ASSERT_EQ("test"_id, propertyName);
    ASSERT_TRUE(it.endOfPath());
}

TEST(AccessPathParser, ComplexPathProperlyReconstructed)
{
    AccessPath path = AccessPath()["a"]["b"]["c"]["d"][42]["f"];
    AccessPathIterator it(path);
    ASSERT_FALSE(it.endOfPath());

    auto extractedPath = it.extractRemainingPath();
    ASSERT_EQ(path, extractedPath);
}

///---

TEST(DataAccessRead, SimpleValue)
{
    uint32_t value = 42;
    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<uint32_t>(), AccessPath(), outValue);
    ASSERT_TRUE(ret);

    uint32_t readValue = 0;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(value, readValue);
}

TEST(DataAccessRead, SimpleValueHasNoNamedComponents)
{
    uint32_t value = 42;
    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<uint32_t>(), AccessPath()["dupa"], outValue);
    ASSERT_FALSE(ret);
}

TEST(DataAccessRead, SimpleValueHasNoIndexComponents)
{
    uint32_t value = 42;
    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<uint32_t>(), AccessPath()[1], outValue);
    ASSERT_FALSE(ret);
}

TEST(DataAccessRead, StructValue)
{
    Vector3 value(1,2,3);
    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<Vector3>(), AccessPath(), outValue);
    ASSERT_TRUE(ret);

    Vector3 readValue;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(value, readValue);
}

TEST(DataAccessRead, StructValueComponents)
{
    Vector3 value(1,2,3);
    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<Vector3>(), AccessPath()["y"], outValue);
    ASSERT_TRUE(ret);

    float readValue;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(value.y, readValue);
}

TEST(DataAccessRead, StructValueAcessingInvalidComponentsFails)
{
    uint32_t value = 42;
    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<uint32_t>(), AccessPath()["crap"], outValue);
    ASSERT_FALSE(ret);
}

TEST(DataAccessRead, ArrayValue)
{
    Array<uint32_t> value;
    value.pushBack(5);
    value.pushBack(10);
    value.pushBack(15);

    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<Array<uint32_t>>(), AccessPath(), outValue);
    ASSERT_TRUE(ret);

    Array<uint32_t> readValue;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(value, readValue);
}

TEST(DataAccessRead, ArrayValueElement)
{
    Array<uint32_t> value;
    value.pushBack(5);
    value.pushBack(10);
    value.pushBack(15);

    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<Array<uint32_t>>(), AccessPath()[1], outValue);
    ASSERT_TRUE(ret);

    uint32_t readValue;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(value[1], readValue);
}


TEST(DataAccessRead, ArrayValueElementOutOfRangeFails)
{
    Array<uint32_t> value;
    value.pushBack(5);
    value.pushBack(10);
    value.pushBack(15);

    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<Array<uint32_t>>(), AccessPath()[10], outValue);
    ASSERT_FALSE(ret);
}

TEST(DataAccessRead, ArraySizeReportedDynArray)
{
    Array<uint32_t> value;
    value.pushBack(5);
    value.pushBack(10);
    value.pushBack(15);

    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath()["size"], outValue);
    ASSERT_TRUE(ret);

    uint32_t readValue;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(3, readValue);
}

TEST(DataAccessRead, ArraySizeReportedStaticArray)
{
    InplaceArray<uint32_t,5> value;
    value.pushBack(5);
    value.pushBack(10);
    value.pushBack(15);

    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath()["size"], outValue);
    ASSERT_TRUE(ret);

    uint32_t readValue;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(3, readValue);
}

TEST(DataAccessRead, ArraySizeReportedNativeArray)
{
    uint32_t value[3];
    value[0] = 5;
    value[1] = 10;
    value[2] = 15;

    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath()["size"], outValue);
    ASSERT_TRUE(ret);

    uint32_t readValue;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(3, readValue);
}

TEST(DataAccessRead, ArrayCapacityReportedDynArray)
{
    Array<uint32_t> value;
    value.pushBack(5);
    value.pushBack(10);
    value.pushBack(15);

    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath()["capacity"], outValue);
    ASSERT_TRUE(ret);

    uint32_t readValue;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(value.capacity(), readValue);
}

TEST(DataAccessRead, ArrayCapacityReportedStaticArray)
{
     InplaceArray<uint32_t,4> value;
    value.pushBack(5);
    value.pushBack(10);
    value.pushBack(15);

    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath()["capacity"], outValue);
    ASSERT_TRUE(ret);

    uint32_t readValue;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(4, readValue);
}

TEST(DataAccessRead, ArrayCapacityReportedNativeArray)
{
    uint32_t value[3];
    value[0] = 5;
    value[1] = 10;
    value[2] = 15;

    Variant outValue;
    auto ret = ReadNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath()["capacity"], outValue);
    ASSERT_TRUE(ret);

    uint32_t readValue;
    auto ret2 = outValue.safeRead(readValue);
    ASSERT_TRUE(ret2);
    ASSERT_EQ(3, readValue);
}

TEST(DataAccessRead, ComplexStructureRead)
{
    Box box(Vector3(1,2,3), Vector3(4,5,6));

    Variant outValue;
    auto ret = ReadNativeData(&box, reflection::GetTypeObject<decltype(box)>(), AccessPath()["min"]["x"], outValue);
    ASSERT_TRUE(ret);
    ASSERT_EQ(1.0f, outValue.get<float>());
    ASSERT_EQ(1, outValue.get<int>());
}

//---

TEST(DataAccessWrite, SimpleValue)
{
    uint32_t value = 0;
    auto valueToWrite = rtti::CreateVariant<uint32_t>(500);
    auto ret = WriteNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath(), valueToWrite);
    ASSERT_TRUE(ret);
    ASSERT_EQ(value, 500);
}

TEST(DataAccessWrite, StructValue)
{
    Vector3 value(0,0,0);
    auto valueToWrite = rtti::CreateVariant(Vector3(1,2,3));
    auto ret = WriteNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath(), valueToWrite);
    ASSERT_TRUE(ret);
    ASSERT_EQ(1.0f, value.x);
    ASSERT_EQ(2.0f, value.y);
    ASSERT_EQ(3.0f, value.z);
}

TEST(DataAccessWrite, StructValueElement)
{
    Vector3 value(0,0,0);
    auto valueToWrite = rtti::CreateVariant<float>(42.0f);
    auto ret = WriteNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath()["y"], valueToWrite);
    ASSERT_TRUE(ret);
    ASSERT_EQ(0.0f, value.x);
    ASSERT_EQ(42.0f, value.y);
    ASSERT_EQ(0.0f, value.z);
}

TEST(DataAccessWrite, StructValueElementInvalidElementFails)
{
    Vector3 value(0,0,0);
    auto valueToWrite = rtti::CreateVariant<float>(42.0f);
    auto ret = WriteNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath()["w"], valueToWrite);
    ASSERT_FALSE(ret);
    ASSERT_EQ(0.0f, value.x);
    ASSERT_EQ(0.0f, value.y);
    ASSERT_EQ(0.0f, value.z);
}

TEST(DataAccessWrite, ArrayElement)
{
    Array<uint32_t> value;
    value.pushBack(5);
    value.pushBack(10);
    value.pushBack(15);

    auto valueToWrite = rtti::CreateVariant<uint32_t>(42);
    auto ret = WriteNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath()[1], valueToWrite);
    ASSERT_TRUE(ret);
    ASSERT_EQ(5, value[0]);
    ASSERT_EQ(42, value[1]);
    ASSERT_EQ(15, value[2]);
}


TEST(DataAccessWrite, ArrayElementInvalidIndexFails)
{
    Array<uint32_t> value;
    value.pushBack(5);
    value.pushBack(10);
    value.pushBack(15);

    auto valueToWrite = rtti::CreateVariant<uint32_t>(42);
    auto ret = WriteNativeData(&value, reflection::GetTypeObject<decltype(value)>(), AccessPath()[5], valueToWrite);
    ASSERT_FALSE(ret);
    ASSERT_EQ(5, value[0]);
    ASSERT_EQ(10, value[1]);
    ASSERT_EQ(15, value[2]);
}


#endif

END_BOOMER_NAMESPACE_EX(res)