/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#pragma once

#include "core/containers/include/hashSet.h"
#include "core/containers/include/inplaceArray.h"
#include "core/system/include/guid.h"

#include "serializationStream.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// reference to a resource
struct CORE_OBJECT_API SerializationWriterResourceReference
{
    ClassType resourceType;
    GUID resourceID;

    INLINE SerializationWriterResourceReference() {};
    INLINE SerializationWriterResourceReference(const SerializationWriterResourceReference& other) = default;
    INLINE SerializationWriterResourceReference(SerializationWriterResourceReference&& other) = default;
    INLINE SerializationWriterResourceReference& operator=(const SerializationWriterResourceReference& other) = default;
    INLINE SerializationWriterResourceReference& operator=(SerializationWriterResourceReference && other) = default;

    INLINE bool operator==(const SerializationWriterResourceReference& other) const
    {
        return (resourceType == other.resourceType) && (resourceID == other.resourceID);
    }

    INLINE static uint32_t CalcHash(SerializationWriterResourceReference& resref)
    {
        CRC32 crc;
        crc << resref.resourceType.name().view();
        crc << resref.resourceID.data()[0];
        crc << resref.resourceID.data()[1];
        crc << resref.resourceID.data()[2];
        crc << resref.resourceID.data()[3];
        return crc;
    }
};

///---

/// all collected references for opcode serialization
struct CORE_OBJECT_API SerializationWriterReferences : public NoCopy
{
    SerializationWriterReferences();
         
    HashSet<StringID> stringIds;
    HashSet<Type> types;
    HashSet<ObjectPtr> objects;
    HashSet<const Property*> properties;
    HashSet<SerializationWriterResourceReference> syncResources;
    HashSet<SerializationWriterResourceReference> asyncResources;
    HashSet<AsyncFileBufferLoaderPtr> asyncBuffers;
};

///---

/// serialization opcode stream builder
class CORE_OBJECT_API SerializationWriter : public NoCopy
{
public:
    SerializationWriter(SerializationStream& stream, SerializationWriterReferences& referenceCollector);
    ~SerializationWriter();

    //--

    //! write raw bytes to stream
    INLINE void writeData(const void* data, uint64_t size);

    //! write typed data
    template< typename T >
    INLINE void writeTypedData(const T& data);

    //--

    INLINE void beginCompound(Type type);
    INLINE void endCompound();

    INLINE void beginArray(uint32_t count);
    INLINE void endArray();

    INLINE void beginSkipBlock();
    INLINE void endSkipBlock();

    INLINE void writeStringID(StringID id);
    INLINE void writeType(Type t);
    INLINE void writeProperty(const Property* rttiProperty);
    INLINE void writePointer(const IObject* object);

    void writeResourceReference(GUID id, ClassType resourceClass, bool async);

    void writeBuffer(Buffer data, CompressionType ct, uint64_t size, uint64_t crc);
    void writeAsyncBuffer(IAsyncFileBufferLoader* loader);

    //--

private:
    SerializationStream& m_stream;
    SerializationWriterReferences& m_references;

    InplaceArray<SerializationOpcode, 20> m_stack;
    InplaceArray<SerializationOpSkipHeader*, 20> m_skips;
    InplaceArray<SerializationOpCompound*, 20> m_compounds;
};
       
///---

template< typename T >
INLINE void SerializationWriter::writeTypedData(const T& data)
{
    writeData(&data, sizeof(T));
}

INLINE void SerializationWriter::writeData(const void* data, uint64_t size)
{
    uint8_t dataSizeValue[10];
    const auto valueSize = WriteCompressedUint64(dataSizeValue, size);

    if (auto op = m_stream.allocOpcode<SerializationOpDataRaw>(valueSize + size))
    {
        auto* writePtr = (uint8_t*)op + sizeof(SerializationOpDataRaw);
        writePtr += WriteCompressedUint64(writePtr, size);
        m_stream.m_totalDataSize += size;
        memcpy(writePtr, data, size);
    }
}

INLINE void SerializationWriter::beginCompound(Type type)
{
    auto op = m_stream.allocOpcode<SerializationOpCompound>();
    if (op)
    {
        op->numProperties = 0;
        op->type = type;
    }

    m_compounds.pushBack(op);
    m_stack.pushBack(SerializationOpcode::Compound);
}

INLINE void SerializationWriter::endCompound()
{
    ASSERT_EX(!m_stack.empty(), "Invalid opcode stack - binary steam is mallformed, must be fixed");
    ASSERT_EX(m_stack.back() == SerializationOpcode::Compound, "No inside a compound - binary steam is mallformed, must be fixed");
    m_stack.popBack();
    ASSERT_EX(!m_compounds.empty(), "No compounds on stack");
    m_compounds.popBack();

    m_stream.allocOpcode<SerializationOpCompoundEnd>();
}

INLINE void SerializationWriter::beginArray(uint32_t count)
{
    if (auto op = m_stream.allocOpcode<SerializationOpArray>())
        op->count = count;

    m_stack.pushBack(SerializationOpcode::Array);
}

INLINE void SerializationWriter::endArray()
{
    ASSERT_EX(!m_stack.empty(), "Invalid opcode stack - binary steam is mallformed, must be fixed");
    ASSERT_EX(m_stack.back() == SerializationOpcode::Array, "No inside an array - binary steam is mallformed, must be fixed");
    m_stack.popBack();

    m_stream.allocOpcode<SerializationOpArrayEnd>();
}

INLINE void SerializationWriter::beginSkipBlock()
{
    auto op = m_stream.allocOpcode<SerializationOpSkipHeader>();
    m_stack.pushBack(SerializationOpcode::SkipHeader);
    m_skips.pushBack(op);
}

INLINE void SerializationWriter::endSkipBlock()
{
    ASSERT_EX(!m_stack.empty(), "Invalid opcode stack - binary steam is mallformed, must be fixed");
    ASSERT_EX(m_stack.back() == SerializationOpcode::SkipHeader, "No inside a skip block - binary steam is mallformed, must be fixed");
    m_stack.popBack();

    ASSERT_EX(!m_skips.empty(), "No active skip block");

    auto header = m_skips.back(); // can be NULL in OOM condition
    m_skips.popBack();

    if (auto op = m_stream.allocOpcode<SerializationOpSkipLabel>())
    {
        if (header)
            header->label = op;
    }
}

INLINE void SerializationWriter::writeStringID(StringID id)
{
    if (id)
        m_references.stringIds.insert(id);

    if (auto op = m_stream.allocOpcode<SerializationOpDataName>())
        op->name = id;
}

INLINE void SerializationWriter::writeType(Type t)
{
    if (t)
        m_references.types.insert(t);

    if (auto op = m_stream.allocOpcode<SerializationOpDataTypeRef>())
        op->type = t;
}

INLINE void SerializationWriter::writeProperty(const Property* rttiProperty)
{
    ASSERT_EX(!m_stack.empty(), "Property must be inside a compound");
    ASSERT_EX(m_stack.back() == SerializationOpcode::Compound, "Property must be directly inside a compound");
    ASSERT_EX(!m_compounds.empty(), "Property must be inside a compound");

    if (auto* compound = m_compounds.back())
        compound->numProperties += 1;

    if (rttiProperty)
        m_references.properties.insert(rttiProperty);

    if (auto op = m_stream.allocOpcode<SerializationOpProperty>())
        op->prop = rttiProperty;
}

INLINE void SerializationWriter::writePointer(const IObject* object)
{
    if (object)
        m_references.objects.insert(AddRef(object));

    if (auto op = m_stream.allocOpcode<SerializationOpDataObjectPointer>())
        op->object = object;
}

//--

END_BOOMER_NAMESPACE()
