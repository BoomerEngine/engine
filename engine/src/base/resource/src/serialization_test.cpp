/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "base/object/include/object.h"
#include "base/reflection/include/reflectionMacros.h"
#include "base/io/include/ioFileHandleMemory.h"
#include "base/object/include/streamOpcodes.h"
#include "base/object/include/streamOpcodeWriter.h"
#include "base/object/include/rttiProperty.h"

#include "resourceFileSaver.h"
#include "resourceFileTables.h"
#include "resourceFileLoader.h"
#include "base/memory/include/public.h"

DECLARE_TEST_FILE(Serialization);

using namespace base;

namespace tests
{
    class TestObject : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TestObject, IObject);

    public:
        StringBuf m_text;
        int m_int = 0;
        float m_float = 0.0f;
        bool m_bool = false;
        StringID m_name;
        Vector3 m_struct;

        RefPtr<TestObject> m_child;
    };

    RTTI_BEGIN_TYPE_CLASS(TestObject);
    RTTI_PROPERTY(m_bool);
    RTTI_PROPERTY(m_int);
    RTTI_PROPERTY(m_float);
    RTTI_PROPERTY(m_name);
    RTTI_PROPERTY(m_text);
    RTTI_PROPERTY(m_struct);
    RTTI_PROPERTY(m_child);
    RTTI_END_TYPE();

    class MassTestObject : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MassTestObject, IObject);

    public:
        Array<Array<Box>> m_crap;
        Array<RefPtr<MassTestObject>> m_child;
    };

    RTTI_BEGIN_TYPE_CLASS(MassTestObject);
    RTTI_PROPERTY(m_crap);
    RTTI_PROPERTY(m_child);
    RTTI_END_TYPE();
}

void HelperSave(const base::ObjectPtr& obj, Buffer& outData)
{
    base::res::FileSavingContext context;
    context.rootObject.pushBack(obj);
    context.protectedStream = false;

    auto writer = base::CreateSharedPtr<base::io::MemoryWriterFileHandle>();

    ASSERT_TRUE(base::res::SaveFile(writer, context)) << "Serialization failed";

    outData = writer->extract();

    ASSERT_LT(0, outData.size()) << "No data in buffer";
}

void HelperSave(const Array<base::ObjectPtr>& roots, Buffer& outData, bool protectedStream)
{
    base::res::FileSavingContext context;
    context.rootObject = roots;
    context.protectedStream = protectedStream;

    auto writer = base::CreateSharedPtr<base::io::MemoryWriterFileHandle>();

    ASSERT_TRUE(base::res::SaveFile(writer, context)) << "Serialization failed";

    outData = writer->extract();

    ASSERT_LT(0, outData.size()) << "No data in buffer";
}

void HelperLoad(const Buffer& data, base::ObjectPtr& outRet)
{
    base::res::FileLoadingContext context;

    auto reader = base::CreateSharedPtr<base::io::MemoryAsyncReaderFileHandle>(data);

    ASSERT_TRUE(base::res::LoadFile(reader, context)) << "Deserialization failed";

    ASSERT_EQ(1, context.loadedRoots.size()) << "Nothing loaded";

    outRet = context.loadedRoots[0];
}

void HelperLoad(const Buffer& data, base::Array<ObjectPtr>& outRoots)
{
    base::res::FileLoadingContext context;

    auto reader = base::CreateSharedPtr<base::io::MemoryAsyncReaderFileHandle>(data);

    ASSERT_TRUE(base::res::LoadFile(reader, context)) << "Deserialization failed";

    ASSERT_NE(0, context.loadedRoots.size()) << "Nothing loaded";

    outRoots = context.loadedRoots;
}

const char* HelperGetName(const base::res::FileTables& tables, uint32_t index)
{
    const auto numNames = tables.chunkCount(base::res::FileTables::ChunkType::Names);
    if (index >= numNames)
        return "";

    const auto* stringTable = tables.stringTable();
    const auto* nameTable = tables.nameTable();
    return stringTable + nameTable[index].stringIndex;
}

const char* HelperGetType(const base::res::FileTables& tables, uint32_t index)
{
    const auto numTypes = tables.chunkCount(base::res::FileTables::ChunkType::Types);
    if (index >= numTypes)
        return "";

    const auto* typeTable = tables.typeTable();
    return HelperGetName(tables, typeTable[index].nameIndex);
}

const char* HelperGetPropertyName(const base::res::FileTables& tables, uint32_t index)
{
    const auto numProps = tables.chunkCount(base::res::FileTables::ChunkType::Properties);
    if (index >= numProps)
        return "";

    const auto* propTable = tables.propertyTable();
    return HelperGetName(tables, propTable[index].nameIndex);
}

const char* HelperGetPropertyClassType(const base::res::FileTables& tables, uint32_t index)
{
    const auto numProps = tables.chunkCount(base::res::FileTables::ChunkType::Properties);
    if (index >= numProps)
        return "";

    const auto* propTable = tables.propertyTable();
    return HelperGetType(tables, propTable[index].classTypeIndex);
}

TEST(Serialization, SaveSimple)
{
    // create objects for testings
    auto object = CreateSharedPtr<tests::TestObject>();

    // save to memory
    base::Buffer data;
    HelperSave(object, data);

    // get the tables
    const auto& tables = *(const base::res::FileTables*)data.data();
    ASSERT_TRUE(tables.validate(data.size())) << "Unable to validate files tables";

    // check names
    ASSERT_STREQ("", HelperGetName(tables, 0));
    ASSERT_STREQ("tests::TestObject", HelperGetName(tables, 1));

    // check types
    ASSERT_STREQ("", HelperGetType(tables, 0));
    ASSERT_STREQ("tests::TestObject", HelperGetType(tables, 1));

    // check exports
    const auto exportCount = tables.chunkCount(base::res::FileTables::ChunkType::Exports);
    const auto exportData = tables.exportTable();
    ASSERT_EQ(1, exportCount);
    ASSERT_EQ(1, exportData[0].classTypeIndex);
    ASSERT_NE(0, exportData[0].crc);
    ASSERT_EQ(0, exportData[0].parentIndex);
    ASSERT_NE(0, exportData[0].dataOffset);
    ASSERT_NE(0, exportData[0].dataSize);
}

TEST(Serialization, SaveSimpleWithSimpleData)
{
    // create objects for testings
    auto object = CreateSharedPtr<tests::TestObject>();
    object->m_bool = true;
    object->m_float = 42.0f;
    object->m_name = "TEST"_id;
    object->m_text = "Ala ma kota";

    // save to memory
    base::Buffer data;
    HelperSave(object, data);

    // get the tables
    const auto& tables = *(const base::res::FileTables*)data.data();
    ASSERT_TRUE(tables.validate(data.size())) << "Unable to validate files tables";

    // check names
    EXPECT_STREQ("", HelperGetName(tables, 0));
    EXPECT_STREQ("TEST", HelperGetName(tables, 1));
    EXPECT_STREQ("bool", HelperGetName(tables, 2));
    EXPECT_STREQ("float", HelperGetName(tables, 3));
    EXPECT_STREQ("StringID", HelperGetName(tables, 4));
    EXPECT_STREQ("StringBuf", HelperGetName(tables, 5));
    EXPECT_STREQ("tests::TestObject", HelperGetName(tables, 6));

    // check types
    EXPECT_STREQ("", HelperGetType(tables, 0));
    EXPECT_STREQ("bool", HelperGetType(tables, 1));
    EXPECT_STREQ("float", HelperGetType(tables, 2));
    EXPECT_STREQ("StringID", HelperGetType(tables, 3));
    EXPECT_STREQ("StringBuf", HelperGetType(tables, 4));
    EXPECT_STREQ("tests::TestObject", HelperGetType(tables, 5));

    // check properties
    EXPECT_STREQ("bool", HelperGetPropertyName(tables, 0));
    EXPECT_STREQ("float", HelperGetPropertyName(tables, 1));
    EXPECT_STREQ("name", HelperGetPropertyName(tables, 2));
    EXPECT_STREQ("text", HelperGetPropertyName(tables, 3));
    EXPECT_STREQ("tests::TestObject", HelperGetPropertyClassType(tables, 0));
    EXPECT_STREQ("tests::TestObject", HelperGetPropertyClassType(tables, 1));
    EXPECT_STREQ("tests::TestObject", HelperGetPropertyClassType(tables, 2));
    EXPECT_STREQ("tests::TestObject", HelperGetPropertyClassType(tables, 3));

    // check exports
    const auto exportCount = tables.chunkCount(base::res::FileTables::ChunkType::Exports);
    const auto exportData = tables.exportTable();
    ASSERT_EQ(1, exportCount);
    ASSERT_EQ(5, exportData[0].classTypeIndex);
    ASSERT_NE(0, exportData[0].crc);
    ASSERT_EQ(0, exportData[0].parentIndex);
    ASSERT_NE(0, exportData[0].dataOffset);
    ASSERT_NE(0, exportData[0].dataSize);
}

TEST(Serialization, ObjectChain)
{
    // create objects for testings
    auto object = CreateSharedPtr<tests::TestObject>();
    object->m_child = CreateSharedPtr<tests::TestObject>();
    object->m_child->parent(object);
    object->m_child->m_child = CreateSharedPtr<tests::TestObject>();
    object->m_child->m_child->parent(object->m_child);

    // save to memory
    base::Buffer data;
    HelperSave(object, data);

    // get the tables
    const auto& tables = *(const base::res::FileTables*)data.data();
    ASSERT_TRUE(tables.validate(data.size())) << "Unable to validate files tables";

    // check exports
    const auto exportCount = tables.chunkCount(base::res::FileTables::ChunkType::Exports);
    const auto exportData = tables.exportTable();
    ASSERT_EQ(3, exportCount);
    ASSERT_EQ(2, exportData[0].classTypeIndex);
    ASSERT_EQ(0, exportData[0].parentIndex);
    ASSERT_EQ(2, exportData[1].classTypeIndex);
    ASSERT_EQ(1, exportData[1].parentIndex);
    ASSERT_EQ(2, exportData[2].classTypeIndex);
    ASSERT_EQ(2, exportData[2].parentIndex);
}

TEST(Serialization, ObjectChainBrokenLink)
{
    // create objects for testings
    auto object = CreateSharedPtr<tests::TestObject>();
    object->m_child = CreateSharedPtr<tests::TestObject>();

    // save to memory
    base::Buffer data;
    HelperSave(object, data);

    // get the tables
    const auto& tables = *(const base::res::FileTables*)data.data();
    ASSERT_TRUE(tables.validate(data.size())) << "Unable to validate files tables";

    // check exports
    const auto exportCount = tables.chunkCount(base::res::FileTables::ChunkType::Exports);
    const auto exportData = tables.exportTable();
    ASSERT_EQ(1, exportCount);
    ASSERT_EQ(2, exportData[0].classTypeIndex);
    ASSERT_EQ(0, exportData[0].parentIndex);
}

TEST(Serialization, SaveLoadSimple)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();
    object->m_bool = true;
    object->m_int = 123;
    object->m_float = 666.0f;
    object->m_text = "I want to belive";

    RefPtr<tests::TestObject> objectChild = CreateSharedPtr<tests::TestObject>();
    objectChild->parent(object);
    objectChild->m_bool = false;
    objectChild->m_int = 456;
    objectChild->m_float = -1.0f;
    objectChild->m_text = "crap";
    object->m_child = objectChild;

    // save to memory
    base::Buffer data;
    HelperSave(object, data);

    // loaded object
    base::ObjectPtr loaded;
    HelperLoad(data, loaded);

    auto loadedEx = base::rtti_cast<tests::TestObject>(loaded);
    ASSERT_FALSE(loadedEx.empty());
    auto loadedEx2 = loadedEx->m_child;
    ASSERT_FALSE(loadedEx2.empty());

    // compare data
    EXPECT_EQ(object->m_bool, loadedEx->m_bool);
    EXPECT_EQ(object->m_int, loadedEx->m_int);
    EXPECT_EQ(object->m_float, loadedEx->m_float);
    EXPECT_EQ(object->m_text, loadedEx->m_text);

    EXPECT_EQ(objectChild->m_bool, loadedEx2->m_bool);
    EXPECT_EQ(objectChild->m_int, loadedEx2->m_int);
    EXPECT_EQ(objectChild->m_float, loadedEx2->m_float);
    EXPECT_EQ(objectChild->m_text, loadedEx2->m_text);
}

static void GenerateCrapContent(uint32_t objectCount, uint32_t contentSize, Array<RefPtr<tests::MassTestObject>>& outTestObjects, Array<ObjectPtr>& outRoots)
{
    FastRandState rand;

    for (uint32_t i = 0; i < objectCount; ++i)
    {
        auto ptr = base::CreateSharedPtr<tests::MassTestObject>();

        auto parentIndex = (int)RandMax(rand, outTestObjects.size()) - 1;
        if (parentIndex == -1)
        {
            outRoots.pushBack(ptr);
        }
        else
        {
            ptr->parent(outTestObjects[parentIndex]);
            outTestObjects[parentIndex]->m_child.pushBack(ptr);
        }

        outTestObjects.pushBack(ptr);

        auto numArrays = 1 + RandMax(rand, contentSize);
        ptr->m_crap.reserve(numArrays);
        for (uint32_t j = 0; j < numArrays; ++j)
        {
            auto& ar = ptr->m_crap.emplaceBack();
            auto numArrays2 = 1 + RandMax(rand, contentSize);
            ar.reserve(numArrays2);

            for (uint32_t k = 0; k < numArrays2; ++k)
            {
                auto& entry = ar.emplaceBack();
                entry.min.x = RandRange(rand, -10.0f, 10.0f);
                entry.min.y = RandRange(rand, -10.0f, 10.0f);
                entry.min.z = RandRange(rand, -10.0f, 10.0f);
                entry.max.x = RandRange(rand, -10.0f, 10.0f);
                entry.max.y = RandRange(rand, -10.0f, 10.0f);
                entry.max.z = RandRange(rand, -10.0f, 10.0f);
            }
        }
    }
}

TEST(Serialization, SaveLoadMassiveProtected)
{
    base::Array<ObjectPtr> roots;
    base::Array<RefPtr<tests::MassTestObject>> objects;
    GenerateCrapContent(500, 20, objects, roots);

    // save to memory
    base::Buffer data;
    {
        ScopeTimer timer;
        HelperSave(roots, data, true);
        TRACE_WARNING("Save protected took {}, content {}", timer, MemSize(data.size()));
        auto compressedData = mem::Compress(mem::CompressionType::LZ4HC, data);
        TRACE_INFO("Compression {} -> {}", MemSize(data.size()), MemSize(compressedData.size()));
    }

    // loaded object
    {
        ScopeTimer timer;
        base::Array<base::ObjectPtr> loaded;
        HelperLoad(data, loaded);
        TRACE_WARNING("Load protected took {}", timer);
    }
}

TEST(Serialization, SaveLoadMassiveUnprotected)
{
    base::Array<ObjectPtr> roots;
    base::Array<RefPtr<tests::MassTestObject>> objects;
    GenerateCrapContent(500, 20, objects, roots);

    // save to memory
    base::Buffer data;
    {
        ScopeTimer timer;
        HelperSave(roots, data, false);
        TRACE_WARNING("Save unprotected took {}, content {}", timer, MemSize(data.size()));
        auto compressedData = mem::Compress(mem::CompressionType::LZ4HC, data);
        TRACE_INFO("Compression {} -> {}", MemSize(data.size()), MemSize(compressedData.size()));
    }

    // loaded object
    {
        ScopeTimer timer;
        base::Array<base::ObjectPtr> loaded;
        HelperLoad(data, loaded);
        TRACE_WARNING("Load unprotected took {}", timer);
    }
}

TEST(Serialization, SaveLoadMassiveMostlyObjectsProtected)
{
    base::Array<ObjectPtr> roots;
    base::Array<RefPtr<tests::MassTestObject>> objects;
    GenerateCrapContent(5000, 3, objects, roots);

    // save to memory
    base::Buffer data;
    {
        ScopeTimer timer;
        HelperSave(roots, data, true);
        TRACE_WARNING("Save protected took {}, content {}", timer, MemSize(data.size()));
        auto compressedData = mem::Compress(mem::CompressionType::LZ4HC, data);
        TRACE_INFO("Compression {} -> {}", MemSize(data.size()), MemSize(compressedData.size()));
    }

    // loaded object
    {
        ScopeTimer timer;
        base::Array<base::ObjectPtr> loaded;
        HelperLoad(data, loaded);
        TRACE_WARNING("Load protected took {}", timer);
    }
}

TEST(Serialization, SaveLoadMassiveMostlyObjectsUnprotected)
{
    base::Array<ObjectPtr> roots;
    base::Array<RefPtr<tests::MassTestObject>> objects;
    GenerateCrapContent(5000, 3, objects, roots);

    // save to memory
    base::Buffer data;
    for (;;)
    {
        ScopeTimer timer;
        HelperSave(roots, data, false);
        TRACE_WARNING("Save protected took {}, content {}", timer, MemSize(data.size()));
        //auto compressedData = mem::Compress(mem::CompressionType::LZ4HC, data);
        //TRACE_INFO("Compression {} -> {}", MemSize(data.size()), MemSize(compressedData.size()));
    }

    // loaded object
    /*{
        ScopeTimer timer;
        base::Array<base::ObjectPtr> loaded;
        HelperLoad(data, loaded);
        TRACE_WARNING("Load protected took {}", timer);
    }*/
}
