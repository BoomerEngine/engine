/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#pragma once

#include "serializationStream.h"

#define SUPPORT_PROTECTED_STREAM

BEGIN_BOOMER_NAMESPACE()

///---

struct CORE_OBJECT_API SerializationResolvedResourceReference
{
    GUID id;
    ClassType type;
    ObjectPtr loaded; // only if loaded

    SerializationResolvedResourceReference();
};

class CORE_OBJECT_API ISerializationBufferFactory : public NoCopy
{
public:
    virtual ~ISerializationBufferFactory();

    /// load content of async buffer with given CRC
    virtual Buffer loadAsyncBufferContent(uint64_t crc) const = 0;

    /// create async buffer loader for a buffer with given CRC
    virtual AsyncFileBufferLoaderPtr createAsyncBufferLoader(uint64_t crc) const = 0;
};

struct CORE_OBJECT_API SerializationResolvedReferences : public NoCopy
{
    Array<StringID> stringIds;

    Array<Type> types;
    Array<StringID> typeNames;

    Array<const Property*> properties;
    Array<StringID> propertyNames;

    Array<ObjectPtr> objects;
    Array<SerializationResolvedResourceReference> resources;

    ISerializationBufferFactory* bufferFactory = nullptr;

    SerializationResolvedReferences();
};

///---

class CORE_OBJECT_API SerializationReader : public NoCopy
{
public:
    SerializationReader(const SerializationResolvedReferences& refs, const void* data, uint64_t size, bool safeLayout, uint32_t version);
    ~SerializationReader();

    //--

    // stream version
    INLINE uint32_t version() const { return m_version; }

    //--

    /// get data pointer (reads the OpValue)
    INLINE const void* pointer(uint64_t size);

    /// copy data to another place
    INLINE void readData(void* data, uint64_t size);

    /// load typed data
    template< typename T >
    INLINE void readTypedData(T& data);

    ///--

    /// enter compound block, returns number of properties to read
    INLINE void enterCompound(uint32_t& outNumProperties);

    /// leave compound block
    INLINE void leaveCompound();

    ///--

    /// enter array block, returns number of elements in the array
    INLINE void enterArray(uint32_t& outNumElements);

    /// exit array block
    INLINE void leaveArray();

    //--

    /// enter the skip block
    INLINE void enterSkipBlock();

    /// leave the skip block
    INLINE void leaveSkipBlock();

    /// jump over the skip block
    void discardSkipBlock();

    ///--

    /// read StringID
    INLINE StringID readStringID();

    /// read type reference
    INLINE Type readType(StringID& outTypeName);

    /// read property reference
    INLINE const Property* readProperty(StringID& outPropertyName);

    /// read pointer to other object
    INLINE IObject* readPointer();

    /// read resource reference
    INLINE const SerializationResolvedResourceReference* readResource();

    /// read buffer for use in async buffer
    AsyncFileBufferLoaderPtr readAsyncBuffer();

    /// read as compressed buffer, does not recompress uncompressed buffers!
    Buffer readCompressedBuffer(CompressionType& outCompression, uint64_t& outSize, uint64_t& outCRC);

    /// read as uncompressed buffer, may do on-the-fly decompression
    Buffer readUncompressedBuffer();

    ///---

private:
    const uint8_t* m_cur = nullptr;
    const uint8_t* m_end = nullptr;
    const uint8_t* m_base = nullptr;

    bool m_protectedStream = false;
    uint32_t m_version = 0;

    const SerializationResolvedReferences& m_refs;

    void readBufferInfo(SerializationBufferInfo& outInfo);
    Buffer readBufferData(uint64_t size, bool makeCopy);

    INLINE uint64_t readCompressedNumber();

    INLINE void checkOp(SerializationOpcode op);
    INLINE void checkDataOp(uint64_t size);
};

///---

INLINE const void* SerializationReader::pointer(uint64_t size)
{
#ifdef SUPPORT_PROTECTED_STREAM
    checkDataOp(size);
#endif
    ASSERT_EX(m_cur + size <= m_end, "Read past buffer end");
    auto* ptr = m_cur;
    m_cur += size;
    return ptr;
}

INLINE void SerializationReader::readData(void* data, uint64_t size)
{
#ifdef SUPPORT_PROTECTED_STREAM
    checkDataOp(size);
#endif
    ASSERT_EX(m_cur + size <= m_end, "Read past buffer end");
    memcpy(data, m_cur, size);
    m_cur += size;
}

template< typename T >
INLINE void SerializationReader::readTypedData(T& data)
{
#ifdef SUPPORT_PROTECTED_STREAM
    checkDataOp(sizeof(T));
#endif
    ASSERT_EX(m_cur + sizeof(T) <= m_end, "Read past buffer end");
    if (alignof(T) < 16)
        data = *(const T*)m_cur;
    else
        memcpy(&data, m_cur, sizeof(T));
    m_cur += sizeof(T);
}

INLINE void SerializationReader::checkOp(SerializationOpcode op)
{
#ifdef SUPPORT_PROTECTED_STREAM
    if (m_protectedStream)
    {
        ASSERT_EX(m_cur < m_end, "Read past buffer end");
        ASSERT_EX((SerializationOpcode)*m_cur == op, "Invalid opcode");
        m_cur += 1;
    }
#endif
}

INLINE void SerializationReader::checkDataOp(uint64_t size)
{
#ifdef SUPPORT_PROTECTED_STREAM
    if (m_protectedStream)
    {
        checkOp(SerializationOpcode::DataRaw);
        const auto savedSize = readCompressedNumber();
        ASSERT_EX(savedSize == size, TempString("Invalid size of saved data {} compared to what is requested ({})", savedSize, size));
    }
#endif
}

INLINE void SerializationReader::enterCompound(uint32_t& outNumProperties)
{
    checkOp(SerializationOpcode::Compound);
    outNumProperties = readCompressedNumber();
}

INLINE void SerializationReader::leaveCompound()
{
    checkOp(SerializationOpcode::CompoundEnd);
}

INLINE void SerializationReader::enterArray(uint32_t& outNumElements)
{
    checkOp(SerializationOpcode::Array);
    outNumElements = readCompressedNumber();
}

INLINE void SerializationReader::leaveArray()
{
    checkOp(SerializationOpcode::ArrayEnd);
}

INLINE void SerializationReader::enterSkipBlock()
{
    checkOp(SerializationOpcode::SkipHeader);
    //readCompressedNumber(); // skip size
}

INLINE void SerializationReader::leaveSkipBlock()
{
    checkOp(SerializationOpcode::SkipLabel);
}
        
INLINE StringID SerializationReader::readStringID()
{
    checkOp(SerializationOpcode::DataName);
    const auto index = readCompressedNumber();
    return m_refs.stringIds[index];
}

INLINE Type SerializationReader::readType(StringID& outTypeName)
{
    checkOp(SerializationOpcode::DataTypeRef);
    const auto index = readCompressedNumber();
    outTypeName = m_refs.typeNames[index];
    return m_refs.types[index];
}

INLINE const Property* SerializationReader::readProperty(StringID& outPropertyName)
{
    checkOp(SerializationOpcode::Property);
    const auto index = readCompressedNumber();
    outPropertyName = m_refs.propertyNames[index];
    return m_refs.properties[index];
}

INLINE IObject* SerializationReader::readPointer()
{
    checkOp(SerializationOpcode::DataObjectPointer);
    const auto index = readCompressedNumber();
    if (index == 0 || index > (int)m_refs.objects.size())
        return nullptr;

    return m_refs.objects[index - 1];
}

INLINE const SerializationResolvedResourceReference* SerializationReader::readResource()
{
    checkOp(SerializationOpcode::DataResourceRef);
    const auto index = readCompressedNumber();
    if (index == 0 || index > (int)m_refs.resources.size())
        return nullptr;

    return &m_refs.resources[index - 1];
}

INLINE uint64_t SerializationReader::readCompressedNumber()
{
    ASSERT_EX(m_cur < m_end, "Reading past the end of the stream");

    auto singleByte = *m_cur++;
    uint64_t ret = singleByte & 0x7F;
    uint32_t offset = 7;

    while (singleByte & 0x80)
    {
        ASSERT_EX(m_cur < m_end, "Reading past the end of the stream");
        singleByte = *m_cur++;
        ret |= (singleByte & 0x7F) << offset;
        offset += 7;
    }

    return ret;
}

//---

END_BOOMER_NAMESPACE()
