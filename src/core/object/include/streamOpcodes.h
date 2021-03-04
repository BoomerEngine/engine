/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#pragma once
#include "core/containers/include/hashSet.h"

BEGIN_BOOMER_NAMESPACE_EX(stream)

///---

enum class StreamOpcode : uint8_t
{
#define STREAM_COMMAND_OPCODE(x) x,
#include "streamOpcodes.inl"
#undef STREAM_COMMAND_OPCODE
};

///---

// get opcode printable name
extern CORE_OBJECT_API const char* OpcodeName(StreamOpcode op);

// save a 'compressed' uint64_t value with UTF-8 style encoding, returns number of bytes written (max: 10)
extern CORE_OBJECT_API uint8_t WriteCompressedUint64(uint8_t* ptr, uint64_t value);

// load a 'compressed' uint64_t value with UTF-8 style encoding, returns the value and advances the stream pointer
extern CORE_OBJECT_API uint64_t ReadCompressedUint64(const uint8_t* ptr, uint8_t& outValueSize);

///---

#pragma pack(push)
#pragma pack(1)

// NOTE: the destructor of the Op structures are NEVER CALLED, if you store something there that requires destruction you will leak memory
// Don't worry, you will learn about it soon, really soon
struct StreamOpBase
{
    StreamOpcode op = StreamOpcode::Nop;

    CORE_OBJECT_API static uint64_t CalcSize(const StreamOpBase* op);
};

template< StreamOpcode opcode, typename SelfT >
struct StreamOpBaseT : public StreamOpBase
{
    static const auto OP = opcode;

    INLINE void setup()
    {
        op = OP;
    }
};

#define STREAM_COMMAND_OPCODE(x) struct StreamOp##x;
#include "streamOpcodes.inl"
#undef STREAM_COMMAND_OPCODE

#define STREAM_OPCODE_DATA(op_) struct StreamOp##op_ : public StreamOpBaseT<StreamOpcode::op_, StreamOp##op_>

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
    const StreamOpSkipLabel* label = nullptr;
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
    Buffer buffer; // destroyed explicitly
};

STREAM_OPCODE_DATA(DataObjectPointer)
{
    const IObject* object = nullptr; // NOT add-reffed (reference is kept elsewhere)
};

STREAM_OPCODE_DATA(DataResourceRef)
{
    StringBuf path; // destroyed explicitly
    ClassType type;
    bool async = false;
};

STREAM_OPCODE_DATA(DataAsyncBuffer)
{
    // TODO!
};

#pragma pack(pop)

#define STREAM_COMMAND_OPCODE(x) static_assert(sizeof(StreamOp##x) != 0, "Opcode structure not defined");
#include "streamOpcodes.inl"
#undef STREAM_COMMAND_OPCODE

///---

/// opcode dispatcher, allows to visit every opcode, virtual-based so used mostly for stats/dumping 
class CORE_OBJECT_API IOpcodeDispatcher : public NoCopy
{
public:
    virtual ~IOpcodeDispatcher();

#define STREAM_COMMAND_OPCODE(x) virtual void processOpcode(const StreamOp##x& op) {};
#include "streamOpcodes.inl"
#undef STREAM_COMMAND_OPCODE

    void dispatchOpcode(const StreamOpBase* ptr)
    {
        switch (ptr->op)
        {
#define STREAM_COMMAND_OPCODE(x) case StreamOpcode::##x: processOpcode(*(const StreamOp##x*)ptr); break;
#include "streamOpcodes.inl"
#undef STREAM_COMMAND_OPCODE
        default: ASSERT(!"Invalid stream opcode");
        }
    }
};

///---

/// opcode iterator
class CORE_OBJECT_API OpcodeIterator
{
public:
    OpcodeIterator(const OpcodeStream* stream = nullptr);
    OpcodeIterator(const OpcodeStream* stream, const StreamOpBase* firstOpcode, const StreamOpBase* lastOpcode);
    OpcodeIterator(const OpcodeIterator& other);
    OpcodeIterator& operator=(const OpcodeIterator& other);

    INLINE const StreamOpBase* operator->() const { return (const StreamOpBase*) m_pos; }
    INLINE const StreamOpBase* operator*() const { return (const StreamOpBase*)m_pos; }

    INLINE operator bool() const { return m_pos != nullptr; }

    INLINE void operator++() { advance(); }
    INLINE void operator++(int) { advance(); }

private:
    const OpcodeStream* m_stream;

    const uint8_t* m_pos = nullptr;
    const uint8_t* m_end = nullptr;
    int m_pageIndex = -1;

    const StreamOpBase* m_finalOpcode = nullptr;

    void advance();
};

///---

/// serialization opcode stream
class CORE_OBJECT_API OpcodeStream : public NoCopy
{
public:
    OpcodeStream();
    ~OpcodeStream();

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
    OpcodeIterator opcodes() const;

    // dispatch all opcodes 
    void dispatch(IOpcodeDispatcher& dispatcher) const;

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

    Array<StreamOpDataInlineBuffer*> m_inlinedBuffers; // all stored inlined buffers
    Array<StreamOpDataResourceRef*> m_resourceReferences; // all stored inlined buffers

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

    friend class OpcodeIterator;
    friend class OpcodeWriter;
};

///---

END_BOOMER_NAMESPACE_EX(stream)
