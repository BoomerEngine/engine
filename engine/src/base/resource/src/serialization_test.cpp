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
#include "base/object/include/memoryWriter.h"
#include "base/object/include/memoryReader.h"
#include "base/object/include/serializationSaver.h"
#include "base/object/include/serializationLoader.h"
#include "base/object/include/streamBinaryVersion.h"
#include "base/reflection/include/reflectionMacros.h"
#include "resourceBinaryFileTables.h"
#include "resourceBinarySaver.h"
#include "resourceBinaryLoader.h"
#include "resourceGeneralTextLoader.h"
#include "resourceGeneralTextSaver.h"

DECLARE_TEST_FILE(Serialization);

using namespace base;

namespace tests
{
    class TestObject : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TestObject, IObject);

    public:
        StringBuf m_text;
        int m_int;
        float m_float;
        bool m_bool;

        RefPtr<TestObject> m_child;

        TestObject()
            : m_int(0)
            , m_float(0)
            , m_bool(false)
        {}
    };

    RTTI_BEGIN_TYPE_CLASS(TestObject);
    RTTI_PROPERTY(m_text);
    RTTI_PROPERTY(m_int);
    RTTI_PROPERTY(m_float);
    RTTI_PROPERTY(m_bool);
    RTTI_PROPERTY(m_child);
    RTTI_END_TYPE();
}

TEST(Serialization, SaveSimple)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();

    // save to memory
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    res::binary::BinarySaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // we should have something
    ASSERT_TRUE(writer.size() > 0) << "No data in buffer";

    // decompile the buffer
    res::binary::FileTables tables;
    stream::MemoryReader reader(writer.data(), writer.size());
    ASSERT_TRUE(tables.load(reader)) << "Failed to load the tables form serialized data";

    // we expect one object
    ASSERT_EQ(1, tables.m_exports.size()) << "Expected exactly one object";

    auto& ex0 = tables.m_exports[0];
    auto name  = &tables.m_strings[tables.m_names[ex0.m_className - 1].m_string];
    ASSERT_TRUE(object->cls()->name() == name) << "Invalid class name: " << name;
}

TEST(Serialization, SaveTwoParented)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();
    RefPtr<tests::TestObject> objectChild = CreateSharedPtr<tests::TestObject>();
    objectChild->parent(object);
    object->m_child = objectChild;

    // save to memory
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    res::binary::BinarySaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // we should have something
    ASSERT_TRUE(writer.size() > 0) << "No data in buffer";

    // decompile the buffer
    res::binary::FileTables tables;
    stream::MemoryReader reader(writer.data(), writer.size());
    ASSERT_TRUE(tables.load(reader)) << "Failed to load the tables form serialized data";

    // we expect one object
    ASSERT_EQ(2, tables.m_exports.size()) << "Expected exactly two objects";

    auto& ex0 = tables.m_exports[0];
    auto& ex1 = tables.m_exports[1];
    ASSERT_EQ(0, ex0.m_parent) << "Object0 should not be parented";
    ASSERT_EQ(1, ex1.m_parent) << "Object1 should be parented to object 0";
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
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    res::binary::BinarySaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // we should have something
    ASSERT_TRUE(writer.size() > 0) << "No data in buffer";

    // load from memory
    stream::MemoryReader reader(writer.data(), writer.size());
    stream::LoadingContext loadContext;
    stream::LoadingResult loadResult;
    res::binary::BinaryLoader loader;
    ASSERT_TRUE(loader.loadObjects(reader, loadContext, loadResult)) << "Loading object back failed";

    // we should have only one root object
    ASSERT_EQ(1, loadResult.m_loadedRootObjects.size()) << "Expected exactly one root object";
    auto rootObject = rtti_cast<tests::TestObject>(loadResult.m_loadedRootObjects[0]);
    ASSERT_TRUE(!!rootObject) << "Root object is not valid";

    // compare data
    EXPECT_EQ(object->m_bool, rootObject->m_bool);
    EXPECT_EQ(object->m_int, rootObject->m_int);
    EXPECT_EQ(object->m_float, rootObject->m_float);
    EXPECT_EQ(object->m_text, rootObject->m_text);
}

TEST(Serialization, SaveSimpleText)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();

    // save to memory
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    res::text::TextSaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // get the text
    auto txt  = (const char*)writer.data();
}

TEST(Serialization, SaveSimpleTextWithData)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();
    object->m_bool = true;
    object->m_float = 0.12345678f;
    object->m_int = 666;
    object->m_text = "Complicated string with problems";

    // save to memory
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    res::text::TextSaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // get the text
    auto txt  = (const char*)writer.data();
}

TEST(Serialization, SaveSimpleTextWithTwoRoots)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();
    object->m_bool = true;
    object->m_float = 0.12345678f;
    object->m_int = 666;
    object->m_text = "Complicated string with problems";

    RefPtr<tests::TestObject> object2 = CreateSharedPtr<tests::TestObject>();
    object2->m_float = 3.1415926535f;
    object2->m_int = 42;
    object2->m_text = "Ala ma kota";

    // save to memory
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    saveContext.m_initialExports.pushBack(object2);
    res::text::TextSaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // get the text
    auto txt  = (const char*)writer.data();
}

TEST(Serialization, SaveSimpleTextWithTwoObjectsParented)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();
    object->m_bool = true;
    object->m_float = 0.12345678f;
    object->m_int = 666;
    object->m_text = "Complicated string with problems";

    RefPtr<tests::TestObject> object2 = CreateSharedPtr<tests::TestObject>();
    //object2->parent(object);
    object->m_child = object2;

    object2->m_float = 3.1415926535f;
    object2->m_int = 42;
    object2->m_text = "Ala ma kota";

    // save to memory
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    res::text::TextSaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // get the text
    auto txt  = (const char*)writer.data();
}

TEST(Serialization, SaveSimpleTextWithTwoCrossRefObjects)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();
    object->m_bool = true;
    object->m_float = 0.12345678f;
    object->m_int = 666;
    object->m_text = "Complicated string with problems";

    RefPtr<tests::TestObject> object2 = CreateSharedPtr<tests::TestObject>();
    object2->m_float = 3.1415926535f;
    object2->m_int = 42;
    object2->m_text = "Ala ma kota";

    object->m_child = object2;
    object2->m_child = object;

    // save to memory
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    saveContext.m_initialExports.pushBack(object2);
    res::text::TextSaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // get the text
    auto txt  = (const char*)writer.data();
}

TEST(Serialization, SaveSimpleTextDeepReference)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();
    object->m_bool = true;
    object->m_float = 0.12345678f;
    object->m_int = 666;
    object->m_text = "Complicated string with problems";

    RefPtr<tests::TestObject> object2 = CreateSharedPtr<tests::TestObject>();
    object2->m_float = 3.1415926535f;
    object2->m_int = 42;
    object2->m_text = "Ala ma kota";

    RefPtr<tests::TestObject> object3 = CreateSharedPtr<tests::TestObject>();
    object3->m_float = 2.7172f;
    object3->m_int = 666;
    object3->m_text = "Brown fox jumps over lazy dog";

    object->m_child = object3;

    object2->m_child = object3;
    //object3->parent(object2);

    // save to memory
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    saveContext.m_initialExports.pushBack(object2);
    res::text::TextSaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // get the text
    auto txt  = (const char*)writer.data();
}

TEST(Serialization, SaveSimpleTextDeepNullified)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();
    object->m_bool = true;
    object->m_float = 0.12345678f;
    object->m_int = 666;
    object->m_text = "Complicated string with problems";

    RefPtr<tests::TestObject> object2 = CreateSharedPtr<tests::TestObject>();
    object2->m_float = 3.1415926535f;
    object2->m_int = 42;
    object2->m_text = "Ala ma kota";

    RefPtr<tests::TestObject> object3 = CreateSharedPtr<tests::TestObject>();
    object3->m_float = 2.7172f;
    object3->m_int = 666;
    object3->m_text = "Brown fox jumps over lazy dog";

    object->m_child = object3;

    object2->m_child = object3;
    //object3->parent(object2);

    // save to memory
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    saveContext.m_initialExports.pushBack(object2);
    res::text::TextSaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // get the text
    auto txt  = (const char*)writer.data();
}

TEST(Serialization, SaveSimpleTextDeepNotReachable)
{
    // create objects for testings
    RefPtr<tests::TestObject> object = CreateSharedPtr<tests::TestObject>();
    object->m_bool = true;
    object->m_float = 0.12345678f;
    object->m_int = 666;
    object->m_text = "Complicated string with problems";

    RefPtr<tests::TestObject> object2 = CreateSharedPtr<tests::TestObject>();
    object2->m_float = 3.1415926535f;
    object2->m_int = 42;
    object2->m_text = "Ala ma kota";

    RefPtr<tests::TestObject> object3 = CreateSharedPtr<tests::TestObject>();
    object3->m_float = 2.7172f;
    object3->m_int = 666;
    object3->m_text = "Brown fox jumps over lazy dog";

    object->m_child = object3;

    //object2->m_child = object3;
    //object3->parent(object2);

    // save to memory
    stream::MemoryWriter writer;
    stream::SavingContext saveContext(object);
    saveContext.m_initialExports.pushBack(object2);
    res::text::TextSaver saver;
    ASSERT_TRUE(saver.saveObjects(writer, saveContext)) << "Serialization failed";

    // get the text
    auto txt  = (const char*)writer.data();
}



