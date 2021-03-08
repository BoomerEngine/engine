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
#include "core/io/include/fileHandleMemory.h"
#include "core/object/include/serializationStream.h"
#include "core/object/include/serializationWriter.h"
#include "core/object/include/rttiProperty.h"

#include "fileSaver.h"
#include "fileTables.h"

DECLARE_TEST_FILE(StreamWriter);

BEGIN_BOOMER_NAMESPACE();

template< typename T >
void HelperWriteType(SerializationWriter& writer, const T& data)
{
    TypeSerializationContext type;
    GetTypeObject<T>()->writeBinary(type, writer, &data, nullptr);
}

TEST(SerializationOpcodes, DataRaw)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    HelperWriteType<uint32_t>(writer, 42);

    ASSERT_EQ(1, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);
    ASSERT_EQ(SerializationOpcode::DataRaw, it->op);

    const auto* op = (const SerializationOpDataRaw*)(*it);
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

TEST(SerializationOpcodes, DataMultipleValues)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    HelperWriteType<uint32_t>(writer, 1);
    HelperWriteType<uint32_t>(writer, 2);
    HelperWriteType<uint32_t>(writer, 3);

    ASSERT_EQ(3, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
        const auto* op = (const SerializationOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(1, *data);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
        const auto* op = (const SerializationOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(2, *data);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
        const auto* op = (const SerializationOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(3, *data);
        ++it;
    }

    ASSERT_FALSE(it);
}

TEST(SerializationOpcodes, DataName)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    HelperWriteType<StringID>(writer, "TEST"_id);

    ASSERT_EQ(1, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);
    ASSERT_EQ(SerializationOpcode::DataName, it->op);

    const auto* op = (const SerializationOpDataName*)(*it);
    ASSERT_EQ("TEST"_id, op->name);

    ASSERT_EQ(1, references.stringIds.size());
    ASSERT_EQ("TEST"_id, references.stringIds.keys()[0]);
}

TEST(SerializationOpcodes, DataNameMappedOnce)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    HelperWriteType<StringID>(writer, "TEST"_id);
    HelperWriteType<StringID>(writer, "TEST"_id);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::DataName, it->op);
        const auto* op = (const SerializationOpDataName*)(*it);
        ASSERT_EQ("TEST"_id, op->name);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataName, it->op);
        const auto* op = (const SerializationOpDataName*)(*it);
        ASSERT_EQ("TEST"_id, op->name);
        ++it;
    }
    ASSERT_FALSE(it);

    ASSERT_EQ(1, references.stringIds.size());
    ASSERT_EQ("TEST"_id, references.stringIds.keys()[0]);
}

TEST(SerializationOpcodes, DataDifferentNames)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    HelperWriteType<StringID>(writer, "TEST"_id);
    HelperWriteType<StringID>(writer, "TEST2"_id);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::DataName, it->op);
        const auto* op = (const SerializationOpDataName*)(*it);
        ASSERT_EQ("TEST"_id, op->name);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataName, it->op);
        const auto* op = (const SerializationOpDataName*)(*it);
        ASSERT_EQ("TEST2"_id, op->name);
        ++it;
    }
    ASSERT_FALSE(it);

    ASSERT_EQ(2, references.stringIds.size());
    ASSERT_EQ("TEST"_id, references.stringIds.keys()[0]);
    ASSERT_EQ("TEST2"_id, references.stringIds.keys()[1]);
}

TEST(SerializationOpcodes, DataType)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    const auto intType = GetTypeObject<int>();
    HelperWriteType<Type>(writer, intType);

    ASSERT_EQ(1, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::DataTypeRef, it->op);
        const auto* op = (const SerializationOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
    }

    ASSERT_EQ(1, references.types.size());
    ASSERT_EQ(intType, references.types.keys()[0]);
}

TEST(SerializationOpcodes, DataTypeMappedOnce)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    const auto intType = GetTypeObject<int>();
    HelperWriteType<Type>(writer, intType);
    HelperWriteType<Type>(writer, intType);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::DataTypeRef, it->op);
        const auto* op = (const SerializationOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataTypeRef, it->op);
        const auto* op = (const SerializationOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
        ++it;
    }
    ASSERT_FALSE(it);

    ASSERT_EQ(1, references.types.size());
    ASSERT_EQ(intType, references.types.keys()[0]);
}

TEST(SerializationOpcodes, DataDifferentTypes)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    const auto intType = GetTypeObject<int>();
    const auto floatType = GetTypeObject<float>();
    HelperWriteType<Type>(writer, intType);
    HelperWriteType<Type>(writer, floatType);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::DataTypeRef, it->op);
        const auto* op = (const SerializationOpDataTypeRef*)(*it);
        ASSERT_EQ(intType, op->type);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataTypeRef, it->op);
        const auto* op = (const SerializationOpDataTypeRef*)(*it);
        ASSERT_EQ(floatType, op->type);
        ++it;
    }
    ASSERT_FALSE(it);

    ASSERT_EQ(2, references.types.size());
    ASSERT_EQ(intType, references.types.keys()[0]);
    ASSERT_EQ(floatType, references.types.keys()[1]);
}

TEST(SerializationOpcodes, NativeArray)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    int ar[3];
    ar[0] = 1;
    ar[1] = 2;
    ar[2] = 3;

    HelperWriteType(writer, ar);

    ASSERT_EQ(5, stream.totalOpcodeCount()); // 3 data + being/end array

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::Array, it->op);
        const auto* op = (const SerializationOpArray*)(*it);
        ASSERT_EQ(3, op->count);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
        const auto* op = (const SerializationOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(1, *data);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
        const auto* op = (const SerializationOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(2, *data);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
        const auto* op = (const SerializationOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(3, *data);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::ArrayEnd, it->op);
        ++it;
    }
    ASSERT_FALSE(it);
}

TEST(SerializationOpcodes, DynamicArray)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    Array<int> ar;
    ar.pushBack(1);
    ar.pushBack(2);
    ar.pushBack(3);

    HelperWriteType(writer, ar);

    ASSERT_EQ(5, stream.totalOpcodeCount()); // 3 data + being/end array

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::Array, it->op);
        const auto* op = (const SerializationOpArray*)(*it);
        ASSERT_EQ(3, op->count);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
        const auto* op = (const SerializationOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(1, *data);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
        const auto* op = (const SerializationOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(2, *data);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
        const auto* op = (const SerializationOpDataRaw*)(*it);
        ASSERT_EQ(4, op->dataSize());
        const auto* data = (const uint32_t*)op->data();
        ASSERT_EQ(3, *data);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::ArrayEnd, it->op);
        ++it;
    }
    ASSERT_FALSE(it);
}

TEST(SerializationOpcodes, ResourceAsyncRef)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    ResourceAsyncRef<IResource> testRef;
    auto id = GUID::Create();
    testRef = ResourceID(id);

    HelperWriteType(writer, testRef);

    ASSERT_EQ(1, stream.totalOpcodeCount()); // 3 data + being/end array

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::DataResourceRef, it->op);
        const auto* op = (const SerializationOpDataResourceRef*)(*it);
        ASSERT_EQ(id, op->id);
        ASSERT_EQ(IResource::GetStaticClass(), op->type);
        ASSERT_TRUE(op->async);
        ++it;
    }

    ASSERT_FALSE(it);

    ASSERT_EQ(1, references.asyncResources.size());
    ASSERT_EQ(0, references.syncResources.size());

    ASSERT_EQ(id, references.asyncResources.keys()[0].resourceID);
    ASSERT_EQ(IResource::GetStaticClass(), references.asyncResources.keys()[0].resourceType);
}

TEST(SerializationOpcodes, SyncResourceRef)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    auto id = GUID::Create();

    ResourceRef<IResource> testRef;
    *((BaseReference*)&testRef) = BaseReference(id);

    HelperWriteType(writer, testRef);

    ASSERT_EQ(2, stream.totalOpcodeCount()); // 3 data + being/end array

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::DataResourceRef, it->op);
        const auto* op = (const SerializationOpDataResourceRef*)(*it);
        ASSERT_EQ(id, op->id);
        ASSERT_EQ(IResource::GetStaticClass(), op->type);
        ASSERT_FALSE(op->async);
        ++it;
    }

    ASSERT_FALSE(it);

    ASSERT_EQ(0, references.asyncResources.size());
    ASSERT_EQ(1, references.syncResources.size());

    ASSERT_EQ(id, references.syncResources.keys()[0].resourceID);
    ASSERT_EQ(IResource::GetStaticClass(), references.syncResources.keys()[0].resourceType);
}

TEST(SerializationOpcodes, CompoundEmpty)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    Vector3 zero;
    HelperWriteType(writer, zero);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::Compound, it->op);
        const auto* op = (const SerializationOpCompound*)(*it);
        ASSERT_EQ(0, op->numProperties);
        ++it;
    }
    {
        ASSERT_EQ(SerializationOpcode::CompoundEnd, it->op);
        const auto* op = (const SerializationOpCompound*)(*it);
        ASSERT_EQ(0, op->numProperties);
        ++it;
    }

    ASSERT_FALSE(it);
    ASSERT_EQ(0, references.properties.size());
}

TEST(SerializationOpcodes, CompoundValues)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    Vector3 zero(1,2,3);
    HelperWriteType(writer, zero);

    ASSERT_EQ(2+3*5, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);
    {
        ASSERT_EQ(SerializationOpcode::Compound, it->op);
        const auto* op = (const SerializationOpCompound*)(*it);
        ASSERT_EQ(3, op->numProperties);
        ++it;
    }

    const auto floatType = GetTypeObject<float>();

    for (int i=1; i<=3; ++i)
    {
        {
            ASSERT_EQ(SerializationOpcode::Property, it->op);
            const auto* op = (const SerializationOpProperty*)(*it);
            if (i == 1)
                ASSERT_STREQ("x", op->prop->name().c_str());
            else if (i == 2)
                ASSERT_STREQ("y", op->prop->name().c_str());
            else if (i == 3)
                ASSERT_STREQ("z", op->prop->name().c_str());
            ++it;
        }
        {
            ASSERT_EQ(SerializationOpcode::DataTypeRef, it->op);
            const auto* op = (const SerializationOpDataTypeRef*)(*it);
            ASSERT_EQ(floatType, op->type);
            ++it;
        }
        {
            ASSERT_EQ(SerializationOpcode::SkipHeader, it->op);
            ++it;
        }
        {
            ASSERT_EQ(SerializationOpcode::DataRaw, it->op);
            const auto* op = (const SerializationOpDataRaw*)(*it);
            ASSERT_EQ(4, op->dataSize());
            const auto* data = (const float*)op->data();
            ASSERT_EQ((float)i, *data);
            ++it;
        }
        {
            ASSERT_EQ(SerializationOpcode::SkipLabel, it->op);
            ++it;
        }
    }

    {
        ASSERT_EQ(SerializationOpcode::CompoundEnd, it->op);
        const auto* op = (const SerializationOpCompound*)(*it);
        ASSERT_EQ(0, op->numProperties);
        ++it;
    }

    ASSERT_FALSE(it);
    ASSERT_EQ(1, references.types.size());
    ASSERT_EQ(3, references.properties.size());
}

TEST(SerializationOpcodes, UncompressedBufferStores)
{
    SerializationStream stream;
    SerializationWriterReferences references;
    SerializationWriter writer(stream, references);

    Buffer buffer = Buffer::Create(POOL_TEMP, 42);
    HelperWriteType(writer, buffer);

    ASSERT_EQ(2, stream.totalOpcodeCount());

    SerializationStreamIterator it(&stream);

    {
        ASSERT_EQ(SerializationOpcode::DataInlineBuffer, it->op);
        const auto* op = (const SerializationOpDataInlineBuffer*)(*it);
        ASSERT_EQ(buffer, op->data);
        ++it;
    }

    ASSERT_FALSE(it);
}

END_BOOMER_NAMESPACE();
