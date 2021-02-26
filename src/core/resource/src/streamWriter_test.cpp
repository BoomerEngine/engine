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
#include "core/reflection/include/reflectionMacros.h"
#include "core/io/include/ioFileHandleMemory.h"
#include "core/object/include/streamOpcodes.h"
#include "core/object/include/streamOpcodeWriter.h"
#include "core/object/include/rttiProperty.h"

#include "resourceFileSaver.h"
#include "resourceFileTables.h"

DECLARE_TEST_FILE(StreamWriter);

BEGIN_BOOMER_NAMESPACE();

template< typename T >
void HelperWriteType(stream::OpcodeWriter& writer, const T& data)
{
    rtti::TypeSerializationContext type;
    reflection::GetTypeObject<T>()->writeBinary(type, writer, &data, nullptr);
}

TEST(StreamOpcodes, DataRaw)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    HelperWriteType<uint32_t>(writer, 42);

    ASSERT_EQ(1, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);

    const auto* op = (const stream::StreamOpDataRaw*)(*it);
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
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    HelperWriteType<uint32_t>(writer, 1);
    HelperWriteType<uint32_t>(writer, 2);
    HelperWriteType<uint32_t>(writer, 3);

    ASSERT_EQ(3, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(1, *data);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(2, *data);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(3, *data);
        ++it;
    }

    ASSERT_FALSE(it);
}

TEST(StreamOpcodes, DataName)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    HelperWriteType<StringID>(writer, "TEST"_id);

    ASSERT_EQ(1, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    ASSERT_EQ(stream::StreamOpcode::DataName, it->op);

    const auto* op = (const stream::StreamOpDataName*)(*it);
    ASSERT_EQ("TEST"_id, op->name);

    ASSERT_EQ(1, references.stringIds.size());
    ASSERT_EQ("TEST"_id, references.stringIds.keys()[0]);
}

TEST(StreamOpcodes, DataNameMappedOnce)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    HelperWriteType<StringID>(writer, "TEST"_id);
    HelperWriteType<StringID>(writer, "TEST"_id);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::DataName, it->op);
        const auto* op = (const stream::StreamOpDataName*)(*it);
        ASSERT_EQ("TEST"_id, op->name);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataName, it->op);
        const auto* op = (const stream::StreamOpDataName*)(*it);
        ASSERT_EQ("TEST"_id, op->name);
        ++it;
    }
    ASSERT_FALSE(it);

    ASSERT_EQ(1, references.stringIds.size());
    ASSERT_EQ("TEST"_id, references.stringIds.keys()[0]);
}

TEST(StreamOpcodes, DataDifferentNames)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    HelperWriteType<StringID>(writer, "TEST"_id);
    HelperWriteType<StringID>(writer, "TEST2"_id);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::DataName, it->op);
        const auto* op = (const stream::StreamOpDataName*)(*it);
        ASSERT_EQ("TEST"_id, op->name);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataName, it->op);
        const auto* op = (const stream::StreamOpDataName*)(*it);
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
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    const auto intType = reflection::GetTypeObject<int>();
    HelperWriteType<Type>(writer, intType);

    ASSERT_EQ(1, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::DataTypeRef, it->op);
        const auto* op = (const stream::StreamOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
    }

    ASSERT_EQ(1, references.types.size());
    ASSERT_EQ(intType, references.types.keys()[0]);
}

TEST(StreamOpcodes, DataTypeMappedOnce)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    const auto intType = reflection::GetTypeObject<int>();
    HelperWriteType<Type>(writer, intType);
    HelperWriteType<Type>(writer, intType);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::DataTypeRef, it->op);
        const auto* op = (const stream::StreamOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataTypeRef, it->op);
        const auto* op = (const stream::StreamOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
        ++it;
    }
    ASSERT_FALSE(it);

    ASSERT_EQ(1, references.types.size());
    ASSERT_EQ(intType, references.types.keys()[0]);
}

TEST(StreamOpcodes, DataDifferentTypes)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    const auto intType = reflection::GetTypeObject<int>();
    const auto floatType = reflection::GetTypeObject<float>();
    HelperWriteType<Type>(writer, intType);
    HelperWriteType<Type>(writer, floatType);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::DataTypeRef, it->op);
        const auto* op = (const stream::StreamOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataTypeRef, it->op);
        const auto* op = (const stream::StreamOpDataTypeRef*)(*it);
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
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    int ar[3];
    ar[0] = 1;
    ar[1] = 2;
    ar[2] = 3;

    HelperWriteType(writer, ar);

    ASSERT_EQ(5, stream.totalOpcodeCount()); // 3 data + being/end array

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::Array, it->op);
        const auto* op = (const stream::StreamOpArray*)(*it);
        ASSERT_EQ(3, op->count);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(1, *data);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(2, *data);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(3, *data);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::ArrayEnd, it->op);
        ++it;
    }
    ASSERT_FALSE(it);
}

TEST(StreamOpcodes, DynamicArray)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    Array<int> ar;
    ar.pushBack(1);
    ar.pushBack(2);
    ar.pushBack(3);

    HelperWriteType(writer, ar);

    ASSERT_EQ(5, stream.totalOpcodeCount()); // 3 data + being/end array

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::Array, it->op);
        const auto* op = (const stream::StreamOpArray*)(*it);
        ASSERT_EQ(3, op->count);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(1, *data);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(2, *data);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(3, *data);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::ArrayEnd, it->op);
        ++it;
    }
    ASSERT_FALSE(it);
}

TEST(StreamOpcodes, AsyncResourceRef)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    res::AsyncRef<res::IResource> testRef;
    testRef = res::ResourcePath("/test.txt");

    HelperWriteType(writer, testRef);

    ASSERT_EQ(1, stream.totalOpcodeCount()); // 3 data + being/end array

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::DataResourceRef, it->op);
        const auto* op = (const stream::StreamOpDataResourceRef*)(*it);
        ASSERT_STREQ("/test.txt", op->path.c_str());
        ASSERT_EQ(res::IResource::GetStaticClass(), op->type);
        ASSERT_TRUE(op->async);
        ++it;
    }

    ASSERT_FALSE(it);

    ASSERT_EQ(1, references.asyncResources.size());
    ASSERT_EQ(0, references.syncResources.size());

    ASSERT_STREQ("/test.txt", references.asyncResources.keys()[0].resourcePath.c_str());
    ASSERT_EQ(res::IResource::GetStaticClass(), references.asyncResources.keys()[0].resourceType);
}

TEST(StreamOpcodes, SyncResourceRef)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    res::Ref<res::IResource> testRef;
    *((res::BaseReference*)&testRef) = res::BaseReference(res::ResourcePath("/test.txt"));

    HelperWriteType(writer, testRef);

    ASSERT_EQ(2, stream.totalOpcodeCount()); // 3 data + being/end array

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataResourceRef, it->op);
        const auto* op = (const stream::StreamOpDataResourceRef*)(*it);
        ASSERT_STREQ("/test.txt", op->path.c_str());
        ASSERT_EQ(res::IResource::GetStaticClass(), op->type);
        ASSERT_FALSE(op->async);
        ++it;
    }

    ASSERT_FALSE(it);

    ASSERT_EQ(0, references.asyncResources.size());
    ASSERT_EQ(1, references.syncResources.size());

    ASSERT_STREQ("/test.txt", references.syncResources.keys()[0].resourcePath.c_str());
    ASSERT_EQ(res::IResource::GetStaticClass(), references.syncResources.keys()[0].resourceType);
}

TEST(StreamOpcodes, CompoundEmpty)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    Vector3 zero;
    HelperWriteType(writer, zero);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::Compound, it->op);
        const auto* op = (const stream::StreamOpCompound*)(*it);
        ASSERT_EQ(0, op->numProperties);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::CompoundEnd, it->op);
        const auto* op = (const stream::StreamOpCompound*)(*it);
        ASSERT_EQ(0, op->numProperties);
        ++it;
    }

    ASSERT_FALSE(it);
    ASSERT_EQ(0, references.properties.size());
}

TEST(StreamOpcodes, CompoundValues)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    Vector3 zero(1,2,3);
    HelperWriteType(writer, zero);

    ASSERT_EQ(2+3*5, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::Compound, it->op);
        const auto* op = (const stream::StreamOpCompound*)(*it);
        ASSERT_EQ(3, op->numProperties);
        ++it;
    }

    const auto floatType = reflection::GetTypeObject<float>();

    for (int i=1; i<=3; ++i)
    {
        {
            ASSERT_EQ(stream::StreamOpcode::Property, it->op);
            const auto* op = (const stream::StreamOpProperty*)(*it);
            if (i == 1)
                ASSERT_STREQ("x", op->prop->name().c_str());
            else if (i == 2)
                ASSERT_STREQ("y", op->prop->name().c_str());
            else if (i == 3)
                ASSERT_STREQ("z", op->prop->name().c_str());
            ++it;
        }
        {
            ASSERT_EQ(stream::StreamOpcode::DataTypeRef, it->op);
            const auto* op = (const stream::StreamOpDataTypeRef*)(*it);
            ASSERT_EQ(floatType, op->type);
            ++it;
        }
        {
            ASSERT_EQ(stream::StreamOpcode::SkipHeader, it->op);
            ++it;
        }
        {
            ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
            const auto* op = (const stream::StreamOpDataRaw*)(*it);
            ASSERT_EQ(4, op->dataSize());
            const auto* data = (const float*)op->data();
            ASSERT_EQ((float)i, *data);
            ++it;
        }
        {
            ASSERT_EQ(stream::StreamOpcode::SkipLabel, it->op);
            ++it;
        }
    }

    {
        ASSERT_EQ(stream::StreamOpcode::CompoundEnd, it->op);
        const auto* op = (const stream::StreamOpCompound*)(*it);
        ASSERT_EQ(0, op->numProperties);
        ++it;
    }

    ASSERT_FALSE(it);
    ASSERT_EQ(1, references.types.size());
    ASSERT_EQ(3, references.properties.size());
}

TEST(StreamOpcodes, InlinedBuffer)
{
    stream::OpcodeStream stream;
    stream::OpcodeWriterReferences references;
    stream::OpcodeWriter writer(stream, references);

    Buffer buffer = Buffer::Create(POOL_TEMP, 42);
    HelperWriteType(writer, buffer);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    stream::OpcodeIterator it(&stream);
    {
        ASSERT_EQ(stream::StreamOpcode::DataRaw, it->op);
        const auto* op = (const stream::StreamOpDataRaw*)(*it);
        ASSERT_EQ(1, op->dataSize());
        const auto* data = (const uint8_t*)op->data();
        ASSERT_EQ(0, *data);
        ++it;
    }
    {
        ASSERT_EQ(stream::StreamOpcode::DataInlineBuffer, it->op);
        const auto* op = (const stream::StreamOpDataInlineBuffer*)(*it);
        ASSERT_EQ(buffer, op->buffer);
        ++it;
    }

    ASSERT_FALSE(it);
}

END_BOOMER_NAMESPACE();
