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

#include "streamOpcodes.h"

BEGIN_BOOMER_NAMESPACE_EX(stream)

///---

/// reference to a resource
struct CORE_OBJECT_API OpcodeWriterResourceReference
{
    ClassType resourceType;
    StringBuf resourcePath;

    INLINE OpcodeWriterResourceReference() {};
    INLINE OpcodeWriterResourceReference(const OpcodeWriterResourceReference& other) = default;
    INLINE OpcodeWriterResourceReference(OpcodeWriterResourceReference&& other) = default;
    INLINE OpcodeWriterResourceReference& operator=(const OpcodeWriterResourceReference& other) = default;
    INLINE OpcodeWriterResourceReference& operator=(OpcodeWriterResourceReference && other) = default;

    INLINE bool operator==(const OpcodeWriterResourceReference& other) const
    {
        return (resourceType == other.resourceType) && (resourcePath == other.resourcePath);
    }

    INLINE static uint32_t CalcHash(OpcodeWriterResourceReference& resref)
    {
        CRC32 crc;
        crc << resref.resourceType.name().view();
        crc << resref.resourcePath.view();
        return crc;
    }
};

///---

/// all collected references for opcode serialization
struct CORE_OBJECT_API OpcodeWriterReferences : public NoCopy
{
    OpcodeWriterReferences();
         
    HashSet<StringID> stringIds;
    HashSet<Type> types;
    HashSet<ObjectPtr> objects;
    HashSet<const Property*> properties;
    HashSet<OpcodeWriterResourceReference> syncResources;
    HashSet<OpcodeWriterResourceReference> asyncResources;
};

///---

/// serialization opcode stream builder
class CORE_OBJECT_API OpcodeWriter : public NoCopy
{
public:
    OpcodeWriter(OpcodeStream& stream, OpcodeWriterReferences& referenceCollector);
    ~OpcodeWriter();

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

    void writeResourceReference(StringView path, ClassType resourceClass, bool async);
    void writeBuffer(const Buffer& buffer);

    //--

private:
    OpcodeStream& m_stream;
    OpcodeWriterReferences& m_references;

    InplaceArray<StreamOpcode, 20> m_stack;
    InplaceArray<StreamOpSkipHeader*, 20> m_skips;
    InplaceArray<StreamOpCompound*, 20> m_compounds;
};
       
///---

template< typename T >
INLINE void OpcodeWriter::writeTypedData(const T& data)
{
    writeData(&data, sizeof(T));
}

INLINE void OpcodeWriter::writeData(const void* data, uint64_t size)
{
    uint8_t dataSizeValue[10];
    const auto valueSize = WriteCompressedUint64(dataSizeValue, size);

    if (auto op = m_stream.allocOpcode<StreamOpDataRaw>(valueSize + size))
    {
        auto* writePtr = (uint8_t*)op + sizeof(StreamOpDataRaw);
        writePtr += WriteCompressedUint64(writePtr, size);
        m_stream.m_totalDataSize += size;
        memcpy(writePtr, data, size);
    }
}

INLINE void OpcodeWriter::beginCompound(Type type)
{
    auto op = m_stream.allocOpcode<StreamOpCompound>();
    if (op)
    {
        op->numProperties = 0;
        op->type = type;
    }

    m_compounds.pushBack(op);
    m_stack.pushBack(StreamOpcode::Compound);
}

INLINE void OpcodeWriter::endCompound()
{
    ASSERT_EX(!m_stack.empty(), "Invalid opcode stack - binary steam is mallformed, must be fixed");
    ASSERT_EX(m_stack.back() == StreamOpcode::Compound, "No inside a compound - binary steam is mallformed, must be fixed");
    m_stack.popBack();
    ASSERT_EX(!m_compounds.empty(), "No compounds on stack");
    m_compounds.popBack();

    m_stream.allocOpcode<StreamOpCompoundEnd>();
}

INLINE void OpcodeWriter::beginArray(uint32_t count)
{
    if (auto op = m_stream.allocOpcode<StreamOpArray>())
        op->count = count;

    m_stack.pushBack(StreamOpcode::Array);
}

INLINE void OpcodeWriter::endArray()
{
    ASSERT_EX(!m_stack.empty(), "Invalid opcode stack - binary steam is mallformed, must be fixed");
    ASSERT_EX(m_stack.back() == StreamOpcode::Array, "No inside an array - binary steam is mallformed, must be fixed");
    m_stack.popBack();

    m_stream.allocOpcode<StreamOpArrayEnd>();
}

INLINE void OpcodeWriter::beginSkipBlock()
{
    auto op = m_stream.allocOpcode<StreamOpSkipHeader>();
    m_stack.pushBack(StreamOpcode::SkipHeader);
    m_skips.pushBack(op);
}

INLINE void OpcodeWriter::endSkipBlock()
{
    ASSERT_EX(!m_stack.empty(), "Invalid opcode stack - binary steam is mallformed, must be fixed");
    ASSERT_EX(m_stack.back() == StreamOpcode::SkipHeader, "No inside a skip block - binary steam is mallformed, must be fixed");
    m_stack.popBack();

    ASSERT_EX(!m_skips.empty(), "No active skip block");

    auto header = m_skips.back(); // can be NULL in OOM condition
    m_skips.popBack();

    if (auto op = m_stream.allocOpcode<StreamOpSkipLabel>())
    {
        if (header)
            header->label = op;
    }
}

INLINE void OpcodeWriter::writeStringID(StringID id)
{
    if (id)
        m_references.stringIds.insert(id);

    if (auto op = m_stream.allocOpcode<StreamOpDataName>())
        op->name = id;
}

INLINE void OpcodeWriter::writeType(Type t)
{
    if (t)
        m_references.types.insert(t);

    if (auto op = m_stream.allocOpcode<StreamOpDataTypeRef>())
        op->type = t;
}

INLINE void OpcodeWriter::writeProperty(const Property* rttiProperty)
{
    ASSERT_EX(!m_stack.empty(), "Property must be inside a compound");
    ASSERT_EX(m_stack.back() == StreamOpcode::Compound, "Property must be directly inside a compound");
    ASSERT_EX(!m_compounds.empty(), "Property must be inside a compound");

    if (auto* compound = m_compounds.back())
        compound->numProperties += 1;

    if (rttiProperty)
        m_references.properties.insert(rttiProperty);

    if (auto op = m_stream.allocOpcode<StreamOpProperty>())
        op->prop = rttiProperty;
}

INLINE void OpcodeWriter::writePointer(const IObject* object)
{
    if (object)
        m_references.objects.insert(AddRef(object));

    if (auto op = m_stream.allocOpcode<StreamOpDataObjectPointer>())
        op->object = object;
}

//--

END_BOOMER_NAMESPACE_EX(stream)
