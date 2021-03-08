/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "fileSaver.h"
#include "fileTables.h"
#include "fileLoader.h"

#include "core/object/include/object.h"
#include "core/test/include/gtest/gtest.h"
#include "core/reflection/include/reflectionMacros.h"
#include "core/io/include/fileHandleMemory.h"
#include "core/object/include/serializationStream.h"
#include "core/object/include/serializationWriter.h"
#include "core/object/include/rttiProperty.h"

DECLARE_TEST_FILE(Serialization);

BEGIN_BOOMER_NAMESPACE_EX(test)

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

//--

void HelperSave(const IObject* obj, Buffer& outData)
{
    FileSavingContext context;
    context.rootObjects.pushBack(obj);
    context.format = FileSaveFormat::UnprotectedBinaryStream;

    auto writer = RefNew<MemoryWriterFileHandle>();

    FileSavingResult result;
    ASSERT_TRUE(SaveFile(writer, context, result)) << "Serialization failed";

    outData = writer->extract();

    ASSERT_LT(0, outData.size()) << "No data in buffer";
}

void HelperSave(const Array<ObjectPtr>& roots, Buffer& outData, bool protectedStream)
{
    FileSavingContext context;
    context.rootObjects = *(const Array<const IObject*>*) &roots;
    context.format = protectedStream ? FileSaveFormat::ProtectedBinaryStream : FileSaveFormat::UnprotectedBinaryStream;
    
    auto writer = RefNew<MemoryWriterFileHandle>();

    FileSavingResult result;
    ASSERT_TRUE(SaveFile(writer, context, result)) << "Serialization failed";

    outData = writer->extract();

    ASSERT_LT(0, outData.size()) << "No data in buffer";
}

void HelperLoad(const Buffer& data, ObjectPtr& outRet)
{
    FileLoadingContext context;

    auto reader = RefNew<MemoryAsyncReaderFileHandle>(data);

    FileLoadingResult result;
    ASSERT_TRUE(LoadFile(reader, context, result)) << "Deserialization failed";

    ASSERT_EQ(1, result.roots.size()) << "Nothing loaded";

    outRet = result.roots[0];
}

void HelperLoad(const Buffer& data, Array<ObjectPtr>& outRoots)
{
    FileLoadingContext context;

    auto reader = RefNew<MemoryAsyncReaderFileHandle>(data);

    FileLoadingResult result;
    ASSERT_TRUE(LoadFile(reader, context, result)) << "Deserialization failed";

    ASSERT_NE(0, result.roots.size()) << "Nothing loaded";

    outRoots = result.roots;
}

const char* HelperGetName(const FileTables& tables, uint32_t index)
{
    const auto numNames = tables.chunkCount(FileTables::ChunkType::Names);
    if (index >= numNames)
        return "";

    const auto* stringTable = tables.stringTable();
    const auto* nameTable = tables.nameTable();
    return stringTable + nameTable[index].stringIndex;
}

const char* HelperGetType(const FileTables& tables, uint32_t index)
{
    const auto numTypes = tables.chunkCount(FileTables::ChunkType::Types);
    if (index >= numTypes)
        return "";

    const auto* typeTable = tables.typeTable();
    return HelperGetName(tables, typeTable[index].nameIndex);
}

const char* HelperGetPropertyName(const FileTables& tables, uint32_t index)
{
    const auto numProps = tables.chunkCount(FileTables::ChunkType::Properties);
    if (index >= numProps)
        return "";

    const auto* propTable = tables.propertyTable();
    return HelperGetName(tables, propTable[index].nameIndex);
}

const char* HelperGetPropertyClassType(const FileTables& tables, uint32_t index)
{
    const auto numProps = tables.chunkCount(FileTables::ChunkType::Properties);
    if (index >= numProps)
        return "";

    const auto* propTable = tables.propertyTable();
    return HelperGetType(tables, propTable[index].classTypeIndex);
}

TEST(Serialization, SaveSimple)
{
    // create objects for testings
    auto object = RefNew<TestObject>();

    // save to memory
    Buffer data;
    HelperSave(object, data);

    // get the tables
    const auto& tables = *(const FileTables*)data.data();
    ASSERT_TRUE(tables.validate(data.size())) << "Unable to validate files tables";

    // check names
    ASSERT_STREQ("", HelperGetName(tables, 0));
    ASSERT_STREQ("TestObject", HelperGetName(tables, 1));

    // check types
    ASSERT_STREQ("", HelperGetType(tables, 0));
    ASSERT_STREQ("TestObject", HelperGetType(tables, 1));

    // check exports
    const auto exportCount = tables.chunkCount(FileTables::ChunkType::Exports);
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
    auto object = RefNew<TestObject>();
    object->m_bool = true;
    object->m_float = 42.0f;
    object->m_name = "TEST"_id;
    object->m_text = "Ala ma kota";

    // save to memory
    Buffer data;
    HelperSave(object, data);

    // get the tables
    const auto& tables = *(const FileTables*)data.data();
    ASSERT_TRUE(tables.validate(data.size())) << "Unable to validate files tables";

    // check names
    EXPECT_STREQ("", HelperGetName(tables, 0));
    EXPECT_STREQ("TEST", HelperGetName(tables, 1));
    EXPECT_STREQ("bool", HelperGetName(tables, 2));
    EXPECT_STREQ("float", HelperGetName(tables, 3));
    EXPECT_STREQ("StringID", HelperGetName(tables, 4));
    EXPECT_STREQ("StringBuf", HelperGetName(tables, 5));
    EXPECT_STREQ("TestObject", HelperGetName(tables, 6));

    // check types
    EXPECT_STREQ("", HelperGetType(tables, 0));
    EXPECT_STREQ("bool", HelperGetType(tables, 1));
    EXPECT_STREQ("float", HelperGetType(tables, 2));
    EXPECT_STREQ("StringID", HelperGetType(tables, 3));
    EXPECT_STREQ("StringBuf", HelperGetType(tables, 4));
    EXPECT_STREQ("TestObject", HelperGetType(tables, 5));

    // check properties
    EXPECT_STREQ("bool", HelperGetPropertyName(tables, 0));
    EXPECT_STREQ("float", HelperGetPropertyName(tables, 1));
    EXPECT_STREQ("name", HelperGetPropertyName(tables, 2));
    EXPECT_STREQ("text", HelperGetPropertyName(tables, 3));
    EXPECT_STREQ("TestObject", HelperGetPropertyClassType(tables, 0));
    EXPECT_STREQ("TestObject", HelperGetPropertyClassType(tables, 1));
    EXPECT_STREQ("TestObject", HelperGetPropertyClassType(tables, 2));
    EXPECT_STREQ("TestObject", HelperGetPropertyClassType(tables, 3));

    // check exports
    const auto exportCount = tables.chunkCount(FileTables::ChunkType::Exports);
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
    auto object = RefNew<TestObject>();
    object->m_child = RefNew<TestObject>();
    object->m_child->parent(object);
    object->m_child->m_child = RefNew<TestObject>();
    object->m_child->m_child->parent(object->m_child);

    // save to memory
    Buffer data;
    HelperSave(object, data);

    // get the tables
    const auto& tables = *(const FileTables*)data.data();
    ASSERT_TRUE(tables.validate(data.size())) << "Unable to validate files tables";

    // check exports
    const auto exportCount = tables.chunkCount(FileTables::ChunkType::Exports);
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
    auto object = RefNew<TestObject>();
    object->m_child = RefNew<TestObject>();

    // save to memory
    Buffer data;
    HelperSave(object, data);

    // get the tables
    const auto& tables = *(const FileTables*)data.data();
    ASSERT_TRUE(tables.validate(data.size())) << "Unable to validate files tables";

    // check exports
    const auto exportCount = tables.chunkCount(FileTables::ChunkType::Exports);
    const auto exportData = tables.exportTable();
    ASSERT_EQ(1, exportCount);
    ASSERT_EQ(2, exportData[0].classTypeIndex);
    ASSERT_EQ(0, exportData[0].parentIndex);
}

TEST(Serialization, SaveLoadSimple)
{
    // create objects for testings
    RefPtr<TestObject> object = RefNew<TestObject>();
    object->m_bool = true;
    object->m_int = 123;
    object->m_float = 666.0f;
    object->m_text = "I want to belive";

    RefPtr<TestObject> objectChild = RefNew<TestObject>();
    objectChild->parent(object);
    objectChild->m_bool = false;
    objectChild->m_int = 456;
    objectChild->m_float = -1.0f;
    objectChild->m_text = "crap";
    object->m_child = objectChild;

    // save to memory
    Buffer data;
    HelperSave(object, data);

    // loaded object
    ObjectPtr loaded;
    HelperLoad(data, loaded);

    auto loadedEx = rtti_cast<TestObject>(loaded);
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

static void GenerateCrapContent(uint32_t objectCount, uint32_t contentSize, Array<RefPtr<MassTestObject>>& outTestObjects, Array<ObjectPtr>& outRoots)
{
    FastRandState rand;

    for (uint32_t i = 0; i < objectCount; ++i)
    {
        auto ptr = RefNew<MassTestObject>();

        auto parentIndex = rand.range(outTestObjects.size()) - 1;
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

        auto numArrays = 1 + rand.range(contentSize);
        ptr->m_crap.reserve(numArrays);
        for (uint32_t j = 0; j < numArrays; ++j)
        {
            auto& ar = ptr->m_crap.emplaceBack();
            auto numArrays2 = 1 + rand.range(contentSize);
            ar.reserve(numArrays2);

            for (uint32_t k = 0; k < numArrays2; ++k)
            {
                auto& entry = ar.emplaceBack();
                entry.min.x = rand.range(-10.0f, 10.0f);
                entry.min.y = rand.range(-10.0f, 10.0f);
                entry.min.z = rand.range(-10.0f, 10.0f);
                entry.max.x = rand.range(-10.0f, 10.0f);
                entry.max.y = rand.range(-10.0f, 10.0f);
                entry.max.z = rand.range(-10.0f, 10.0f);
            }
        }
    }
}

TEST(Serialization, SaveLoadMassiveProtected)
{
    Array<ObjectPtr> roots;
    Array<RefPtr<MassTestObject>> objects;
    GenerateCrapContent(500, 20, objects, roots);

    // save to memory
    Buffer data;
    {
        ScopeTimer timer;
        HelperSave(roots, data, true);
        TRACE_WARNING("Save protected took {}, content {}", timer, MemSize(data.size()));
        auto compressedData = Compress(CompressionType::LZ4HC, data, POOL_SERIALIZATION);
        TRACE_INFO("Compression {} -> {}", MemSize(data.size()), MemSize(compressedData.size()));
    }

    // loaded object
    {
        ScopeTimer timer;
        Array<ObjectPtr> loaded;
        HelperLoad(data, loaded);
        TRACE_WARNING("Load protected took {}", timer);
    }
}

TEST(Serialization, SaveLoadMassiveUnprotected)
{
    Array<ObjectPtr> roots;
    Array<RefPtr<MassTestObject>> objects;
    GenerateCrapContent(500, 20, objects, roots);

    // save to memory
    Buffer data;
    {
        ScopeTimer timer;
        HelperSave(roots, data, false);
        TRACE_WARNING("Save unprotected took {}, content {}", timer, MemSize(data.size()));
        auto compressedData = Compress(CompressionType::LZ4HC, data, POOL_SERIALIZATION);
        TRACE_INFO("Compression {} -> {}", MemSize(data.size()), MemSize(compressedData.size()));
    }

    // loaded object
    {
        ScopeTimer timer;
        Array<ObjectPtr> loaded;
        HelperLoad(data, loaded);
        TRACE_WARNING("Load unprotected took {}", timer);
    }
}

TEST(Serialization, SaveLoadMassiveMostlyObjectsProtected)
{
    Array<ObjectPtr> roots;
    Array<RefPtr<MassTestObject>> objects;
    GenerateCrapContent(5000, 3, objects, roots);

    // save to memory
    Buffer data;
    {
        ScopeTimer timer;
        HelperSave(roots, data, true);
        TRACE_WARNING("Save protected took {}, content {}", timer, MemSize(data.size()));
        auto compressedData = Compress(CompressionType::LZ4HC, data, POOL_SERIALIZATION);
        TRACE_INFO("Compression {} -> {}", MemSize(data.size()), MemSize(compressedData.size()));
    }

    // loaded object
    {
        ScopeTimer timer;
        Array<ObjectPtr> loaded;
        HelperLoad(data, loaded);
        TRACE_WARNING("Load protected took {}", timer);
    }
}

TEST(Serialization, SaveLoadMassiveMostlyObjectsUnprotected)
{
    Array<ObjectPtr> roots;
    Array<RefPtr<MassTestObject>> objects;
    GenerateCrapContent(5000, 3, objects, roots);

    // save to memory
    Buffer data;
    //for (;;)
    {
        ScopeTimer timer;
        HelperSave(roots, data, false);
        TRACE_WARNING("Save protected took {}, content {}", timer, MemSize(data.size()));
        //auto compressedData = Compress(CompressionType::LZ4HC, data);
        //TRACE_INFO("Compression {} -> {}", MemSize(data.size()), MemSize(compressedData.size()));
    }

    // loaded object
    {
        ScopeTimer timer;
        Array<ObjectPtr> loaded;
        HelperLoad(data, loaded);
        TRACE_WARNING("Load protected took {}", timer);
    }
}

END_BOOMER_NAMESPACE_EX(test)
