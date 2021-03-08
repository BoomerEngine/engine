/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#pragma once

#include "core/containers/include/hashSet.h"
#include "core/system/include/guid.h"

BEGIN_BOOMER_NAMESPACE()

///---

class SerializationStream;

typedef uint16_t MappedNameIndex;
typedef uint16_t MappedTypeIndex;
typedef uint16_t MappedPropertyIndex;
typedef uint16_t MappedPathIndex;
typedef uint32_t MappedObjectIndex; // yup, it happened 64K+ objects in one files
typedef uint16_t MappedBufferIndex;

///---

enum class SerializationOpcode : uint8_t
{
#define SERIALIZATION_OPCODE(x) x,
#include "serializationStream.inl"
#undef SERIALIZATION_OPCODE
};

///---

// get opcode printable name
extern CORE_OBJECT_API const char* SerializationOpcodeName(SerializationOpcode op);

// save a 'compressed' uint64_t value with UTF-8 style encoding, returns number of bytes written (max: 10)
extern CORE_OBJECT_API uint8_t WriteCompressedUint64(uint8_t* ptr, uint64_t value);

// load a 'compressed' uint64_t value with UTF-8 style encoding, returns the value and advances the stream pointer
extern CORE_OBJECT_API uint64_t ReadCompressedUint64(const uint8_t* ptr, uint8_t& outValueSize);

///---

#pragma pack(push)
#pragma pack(1)

// NOTE: the destructor of the Op structures are NEVER CALLED, if you store something there that requires destruction you will leak memory
// Don't worry, you will learn about it soon, really soon
struct SerializationOpBase
{
    SerializationOpcode op = SerializationOpcode::Nop;

    CORE_OBJECT_API static uint64_t CalcSize(const SerializationOpBase* op);
};

template< SerializationOpcode opcode, typename SelfT >
struct SerializationOpBaseT : public SerializationOpBase
{
    static const auto OP = opcode;

    INLINE void setup()
    {
        op = OP;
    }
};

#define SERIALIZATION_OPCODE(x) struct SerializationOp##x;
#include "serializationStream.inl"
#undef SERIALIZATION_OPCODE

#define STREAM_OPCODE_DATA(op_) struct SerializationOp##op_ : public SerializationOpBaseT<SerializationOpcode::op_, SerializationOp##op_>

STREAM_OPCODE_DATA(Nop)
{
};

STREAM_OPCODE_DATA(Compound)
{
    Type type;
    uint16_t numProperties = 0;
};

STREAM_OPCODE_DATA(CompoundEnd)
{
};

STREAM_OPCODE_DATA(Array)
{
    uint32_t count = 0;
};

STREAM_OPCODE_DATA(ArrayEnd)
{
};

STREAM_OPCODE_DATA(Property)
{
    const Property* prop = nullptr;
};

STREAM_OPCODE_DATA(SkipHeader)
{
    const SerializationOpSkipLabel* label = nullptr;
};

STREAM_OPCODE_DATA(SkipLabel)
{
};

STREAM_OPCODE_DATA(DataRaw)
{
    // size follows (compressed uint64)
    // data follows

    CORE_OBJECT_API uint64_t dataSize() const;
    CORE_OBJECT_API const void* data() const;
};

STREAM_OPCODE_DATA(DataTypeRef)
{
    Type type;
};

STREAM_OPCODE_DATA(DataName)
{
    StringID name;
};

STREAM_OPCODE_DATA(DataInlineBuffer)
{
    AsyncFileBufferLoaderPtr asyncLoader;

    Buffer data; // destroyed explicitly, always raw data

    CompressionType compressionType = CompressionType::Uncompressed; // may be none
    uint64_t uncompressedSize = 0;
    uint64_t uncompressedCRC = 0;
};

STREAM_OPCODE_DATA(DataObjectPointer)
{
    const IObject* object = nullptr; // NOT add-reffed (reference is kept elsewhere)
};

STREAM_OPCODE_DATA(DataResourceRef)
{
    GUID id;
    ClassType type;
    bool async = false;
};

STREAM_OPCODE_DATA(DataAsyncFileBuffer)
{
    // TODO!
};

#pragma pack(pop)

#define SERIALIZATION_OPCODE(x) static_assert(sizeof(SerializationOp##x) != 0, "Opcode structure not defined");
#include "serializationStream.inl"
#undef SERIALIZATION_OPCODE

///---

/// opcode dispatcher, allows to visit every opcode, virtual-based so used mostly for stats/dumping 
class CORE_OBJECT_API ISerializationStreamVisitor : public NoCopy
{
public:
    virtual ~ISerializationStreamVisitor();

#define SERIALIZATION_OPCODE(x) virtual void processOpcode(const SerializationOp##x& op) {};
#include "serializationStream.inl"
#undef SERIALIZATION_OPCODE

    void dispatchOpcode(const SerializationOpBase* ptr)
    {
        switch (ptr->op)
        {
#define SERIALIZATION_OPCODE(x) case SerializationOpcode::##x: processOpcode(*(const SerializationOp##x*)ptr); break;
#include "serializationStream.inl"
#undef SERIALIZATION_OPCODE
        default: ASSERT(!"Invalid stream opcode");
        }
    }
};

///---

/// opcode iterator
class CORE_OBJECT_API SerializationStreamIterator
{
public:
    SerializationStreamIterator(const SerializationStream* stream = nullptr);
    SerializationStreamIterator(const SerializationStream* stream, const SerializationOpBase* firstOpcode, const SerializationOpBase* lastOpcode);
    SerializationStreamIterator(const SerializationStreamIterator& other);
    SerializationStreamIterator& operator=(const SerializationStreamIterator& other);

    INLINE const SerializationOpBase* operator->() const { return (const SerializationOpBase*) m_pos; }
    INLINE const SerializationOpBase* operator*() const { return (const SerializationOpBase*)m_pos; }

    INLINE operator bool() const { return m_pos != nullptr; }

    INLINE void operator++() { advance(); }
    INLINE void operator++(int) { advance(); }

private:
    const SerializationStream* m_stream;

    const uint8_t* m_pos = nullptr;
    const uint8_t* m_end = nullptr;
    int m_pageIndex = -1;

    const SerializationOpBase* m_finalOpcode = nullptr;

    void advance();
};

///---

/// serialization opcode stream
class CORE_OBJECT_API SerializationStream : public NoCopy
{
public:
    SerializationStream();
    ~SerializationStream();

    //--

    // did any serious error condition occurred during stream building ? (out of memory, etc)
    INLINE bool corrupted() const { return m_corrupted; }

    // get total size of the opcode data (may be big...)
    INLINE uint64_t totalOpcodeSize() const { return m_totalOpcodeSize; }

    // get number of opcodes
    INLINE uint64_t totalOpcodeCount() const { return m_totalOpcodeCount; }

    // get total size of the actual data we describe with the opcode stream 
    INLINE uint64_t totalDataSize() const { return m_totalDataSize; }

    //--

    // get opcode stream for iteration
    SerializationStreamIterator opcodes() const;

    // dispatch all opcodes 
    void dispatch(ISerializationStreamVisitor& dispatcher) const;

    /// dump stream to text (debug only)
    void print(IFormatStream& f);

private:
    struct Page
    {
        uint8_t* cur = nullptr;
        uint8_t* end = nullptr;
        uint8_t* base = nullptr;
    };

    Page* m_writePage = nullptr;
    Array<Page> m_pages;

    uint64_t m_totalOpcodeSize = 0;
    uint64_t m_totalOpcodeCount = 0;
    uint64_t m_totalDataSize = 0; // raw data size + 2 bytes per ref on average + all buffer sizes + all inlined buffer sizes
    bool m_corrupted = false;

    Array<SerializationOpDataInlineBuffer*> m_inlinedBuffers; // all stored inlined buffers
    Array<SerializationOpDataResourceRef*> m_resourceReferences; // all stored inlined buffers

    //--

    int findPageForData(const void* dataPtr) const;

    bool allocNewPage(uint64_t requiredSize);
    void* allocInternal(uint64_t size);

    void freePages();

    //--

    //! allocate opcode with extra data
    template< typename T >
    INLINE T* allocOpcode(uint64_t extraDataSize = 0)
    {
        auto* op = (T*)allocInternal(sizeof(T) + extraDataSize);
        op->setup();
        return op;
    }

    friend class SerializationStreamIterator;
    friend class SerializationWriter;
};

///---

struct SerializationBufferInfo
{
    bool externalBuffer = false;
    uint64_t uncompressedSize = 0;
    uint64_t uncompressedCRC = 0;
    uint64_t compressedSize = 0;
    CompressionType compressionType = CompressionType::Uncompressed;
};


END_BOOMER_NAMESPACE()
