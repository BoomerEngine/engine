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

DECLARE_TEST_FILE(StreamWriter);

using namespace base;

template< typename T >
void HelperWriteType(base::stream::OpcodeWriter& writer, const T& data)
{
    base::rtti::TypeSerializationContext type;
    base::reflection::GetTypeObject<T>()->writeBinary(type, writer, &data, nullptr);
}

TEST(StreamOpcodes, DataRaw)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    HelperWriteType<uint32_t>(writer, 42);

    ASSERT_EQ(1, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);

    const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
    ASSERT_EQ(4, op->dataSize());

    const auto* data = (const uint32_t*)op->data();
    ASSERT_EQ(42, *data);
    
    ASSERT_EQ(0, references.asyncResources.size());
    ASSERT_EQ(0, references.syncResources.size());
    ASSERT_EQ(0, references.stringIds.size());
    ASSERT_EQ(0, references.properties.size());
    ASSERT_EQ(0, references.types.size());
    ASSERT_EQ(0, references.objects.size());
}

TEST(StreamOpcodes, DataMultipleValues)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    HelperWriteType<uint32_t>(writer, 1);
    HelperWriteType<uint32_t>(writer, 2);
    HelperWriteType<uint32_t>(writer, 3);

    ASSERT_EQ(3, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(1, *data);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(2, *data);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(3, *data);
        ++it;
    }

    ASSERT_FALSE(it);
}

TEST(StreamOpcodes, DataName)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    HelperWriteType<base::StringID>(writer, "TEST"_id);

    ASSERT_EQ(1, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    ASSERT_EQ(base::stream::StreamOpcode::DataName, it->op);

    const auto* op = (const base::stream::StreamOpDataName*)(*it);
    ASSERT_EQ("TEST"_id, op->name);

    ASSERT_EQ(1, references.stringIds.size());
    ASSERT_EQ("TEST"_id, references.stringIds.keys()[0]);
}

TEST(StreamOpcodes, DataNameMappedOnce)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    HelperWriteType<base::StringID>(writer, "TEST"_id);
    HelperWriteType<base::StringID>(writer, "TEST"_id);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataName, it->op);
        const auto* op = (const base::stream::StreamOpDataName*)(*it);
        ASSERT_EQ("TEST"_id, op->name);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataName, it->op);
        const auto* op = (const base::stream::StreamOpDataName*)(*it);
        ASSERT_EQ("TEST"_id, op->name);
        ++it;
    }
    ASSERT_FALSE(it);

    ASSERT_EQ(1, references.stringIds.size());
    ASSERT_EQ("TEST"_id, references.stringIds.keys()[0]);
}

TEST(StreamOpcodes, DataDifferentNames)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    HelperWriteType<base::StringID>(writer, "TEST"_id);
    HelperWriteType<base::StringID>(writer, "TEST2"_id);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataName, it->op);
        const auto* op = (const base::stream::StreamOpDataName*)(*it);
        ASSERT_EQ("TEST"_id, op->name);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataName, it->op);
        const auto* op = (const base::stream::StreamOpDataName*)(*it);
        ASSERT_EQ("TEST2"_id, op->name);
        ++it;
    }
    ASSERT_FALSE(it);

    ASSERT_EQ(2, references.stringIds.size());
    ASSERT_EQ("TEST"_id, references.stringIds.keys()[0]);
    ASSERT_EQ("TEST2"_id, references.stringIds.keys()[1]);
}

TEST(StreamOpcodes, DataType)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    const auto intType = base::reflection::GetTypeObject<int>();
    HelperWriteType<base::Type>(writer, intType);

    ASSERT_EQ(1, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataTypeRef, it->op);
        const auto* op = (const base::stream::StreamOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
    }

    ASSERT_EQ(1, references.types.size());
    ASSERT_EQ(intType, references.types.keys()[0]);
}

TEST(StreamOpcodes, DataTypeMappedOnce)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    const auto intType = base::reflection::GetTypeObject<int>();
    HelperWriteType<base::Type>(writer, intType);
    HelperWriteType<base::Type>(writer, intType);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataTypeRef, it->op);
        const auto* op = (const base::stream::StreamOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataTypeRef, it->op);
        const auto* op = (const base::stream::StreamOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
        ++it;
    }
    ASSERT_FALSE(it);

    ASSERT_EQ(1, references.types.size());
    ASSERT_EQ(intType, references.types.keys()[0]);
}

TEST(StreamOpcodes, DataDifferentTypes)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    const auto intType = base::reflection::GetTypeObject<int>();
    const auto floatType = base::reflection::GetTypeObject<float>();
    HelperWriteType<base::Type>(writer, intType);
    HelperWriteType<base::Type>(writer, floatType);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataTypeRef, it->op);
        const auto* op = (const base::stream::StreamOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataTypeRef, it->op);
        const auto* op = (const base::stream::StreamOpDataTypeRef*)(*it);
        ASSERT_EQ(floatType, op->type);
        ++it;
    }
    ASSERT_FALSE(it);

    ASSERT_EQ(2, references.types.size());
    ASSERT_EQ(intType, references.types.keys()[0]);
    ASSERT_EQ(floatType, references.types.keys()[1]);
}

TEST(StreamOpcodes, NativeArray)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    int ar[3];
    ar[0] = 1;
    ar[1] = 2;
    ar[2] = 3;

    HelperWriteType(writer, ar);

    ASSERT_EQ(5, stream.totalOpcodeCount()); // 3 data + being/end array

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::Array, it->op);
        const auto* op = (const base::stream::StreamOpArray*)(*it);
        ASSERT_EQ(3, op->count);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(1, *data);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(2, *data);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(3, *data);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::ArrayEnd, it->op);
        ++it;
    }
    ASSERT_FALSE(it);
}

TEST(StreamOpcodes, DynamicArray)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    base::Array<int> ar;
    ar.pushBack(1);
    ar.pushBack(2);
    ar.pushBack(3);

    HelperWriteType(writer, ar);

    ASSERT_EQ(5, stream.totalOpcodeCount()); // 3 data + being/end array

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::Array, it->op);
        const auto* op = (const base::stream::StreamOpArray*)(*it);
        ASSERT_EQ(3, op->count);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(1, *data);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(2, *data);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(3, *data);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::ArrayEnd, it->op);
        ++it;
    }
    ASSERT_FALSE(it);
}

TEST(StreamOpcodes, AsyncResourceRef)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    res::AsyncRef<res::IResource> testRef;
    testRef = base::res::ResourceKey("/test.txt", base::res::IResource::GetStaticClass());

    HelperWriteType(writer, testRef);

    ASSERT_EQ(1, stream.totalOpcodeCount()); // 3 data + being/end array

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataResourceRef, it->op);
        const auto* op = (const base::stream::StreamOpDataResourceRef*)(*it);
        ASSERT_STREQ("/test.txt", op->path.c_str());
        ASSERT_EQ(base::res::IResource::GetStaticClass(), op->type);
        ASSERT_TRUE(op->async);
        ++it;
    }

    ASSERT_FALSE(it);

    ASSERT_EQ(1, references.asyncResources.size());
    ASSERT_EQ(0, references.syncResources.size());

    ASSERT_STREQ("/test.txt", references.asyncResources.keys()[0].resourcePath.c_str());
    ASSERT_EQ(base::res::IResource::GetStaticClass(), references.asyncResources.keys()[0].resourceType);
}

TEST(StreamOpcodes, SyncResourceRef)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    res::Ref<res::IResource> testRef;
    (*(res::BaseReference*) & testRef) = res::BaseReference(base::res::ResourceKey("/test.txt", base::res::IResource::GetStaticClass()));

    HelperWriteType(writer, testRef);

    ASSERT_EQ(2, stream.totalOpcodeCount()); // 3 data + being/end array

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataResourceRef, it->op);
        const auto* op = (const base::stream::StreamOpDataResourceRef*)(*it);
        ASSERT_STREQ("/test.txt", op->path.c_str());
        ASSERT_EQ(base::res::IResource::GetStaticClass(), op->type);
        ASSERT_FALSE(op->async);
        ++it;
    }

    ASSERT_FALSE(it);

    ASSERT_EQ(0, references.asyncResources.size());
    ASSERT_EQ(1, references.syncResources.size());

    ASSERT_STREQ("/test.txt", references.syncResources.keys()[0].resourcePath.c_str());
    ASSERT_EQ(base::res::IResource::GetStaticClass(), references.syncResources.keys()[0].resourceType);
}

TEST(StreamOpcodes, CompoundEmpty)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    base::Vector3 zero;
    HelperWriteType(writer, zero);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::Compound, it->op);
        const auto* op = (const base::stream::StreamOpCompound*)(*it);
        ASSERT_EQ(0, op->numProperties);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::CompoundEnd, it->op);
        const auto* op = (const base::stream::StreamOpCompound*)(*it);
        ASSERT_EQ(0, op->numProperties);
        ++it;
    }

    ASSERT_FALSE(it);
    ASSERT_EQ(0, references.properties.size());
}

TEST(StreamOpcodes, CompoundValues)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    base::Vector3 zero(1,2,3);
    HelperWriteType(writer, zero);

    ASSERT_EQ(2+3*5, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::Compound, it->op);
        const auto* op = (const base::stream::StreamOpCompound*)(*it);
        ASSERT_EQ(3, op->numProperties);
        ++it;
    }

    const auto floatType = base::reflection::GetTypeObject<float>();

    for (int i=1; i<=3; ++i)
    {
        {
            ASSERT_EQ(base::stream::StreamOpcode::Property, it->op);
            const auto* op = (const base::stream::StreamOpProperty*)(*it);
            if (i == 1)
                ASSERT_STREQ("x", op->prop->name().c_str());
            else if (i == 2)
                ASSERT_STREQ("y", op->prop->name().c_str());
            else if (i == 3)
                ASSERT_STREQ("z", op->prop->name().c_str());
            ++it;
        }
        {
            ASSERT_EQ(base::stream::StreamOpcode::DataTypeRef, it->op);
            const auto* op = (const base::stream::StreamOpDataTypeRef*)(*it);
            ASSERT_EQ(floatType, op->type);
            ++it;
        }
        {
            ASSERT_EQ(base::stream::StreamOpcode::SkipHeader, it->op);
            ++it;
        }
        {
            ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
            const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
            ASSERT_EQ(4, op->dataSize());
            const auto* data = (const float*)op->data();
            ASSERT_EQ((float)i, *data);
            ++it;
        }
        {
            ASSERT_EQ(base::stream::StreamOpcode::SkipLabel, it->op);
            ++it;
        }
    }

    {
        ASSERT_EQ(base::stream::StreamOpcode::CompoundEnd, it->op);
        const auto* op = (const base::stream::StreamOpCompound*)(*it);
        ASSERT_EQ(0, op->numProperties);
        ++it;
    }

    ASSERT_FALSE(it);
    ASSERT_EQ(1, references.types.size());
    ASSERT_EQ(3, references.properties.size());
}

TEST(StreamOpcodes, InlinedBuffer)
{
    base::stream::OpcodeStream stream;
    base::stream::OpcodeWriterReferences references;
    base::stream::OpcodeWriter writer(stream, references);

    base::Buffer buffer = base::Buffer::Create(POOL_TEMP, 42);
    HelperWriteType(writer, buffer);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    base::stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const base::stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(1, op->dataSize());
        const auto* data = (const uint8_t*)op->data();
        ASSERT_EQ(0, *data);
        ++it;
    }
    {
        ASSERT_EQ(base::stream::StreamOpcode::DataInlineBuffer, it->op);
        const auto* op = (const base::stream::StreamOpDataInlineBuffer*)(*it);
        ASSERT_EQ(buffer, op->buffer);
        ++it;
    }

    ASSERT_FALSE(it);
}