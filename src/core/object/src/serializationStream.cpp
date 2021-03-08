/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiProperty.h"
#include "object.h"

#include "serializationStream.h"

BEGIN_BOOMER_NAMESPACE()

//--

const char* SerializationOpcodeName(SerializationOpcode op)
{
    switch (op)
    {
#define SERIALIZATION_OPCODE(x) case SerializationOpcode::##x: return #x; 
#include "serializationStream.inl"
#undef SERIALIZATION_OPCODE
    }
    return "UnknownOpcode";
}

INLINE static uint8_t MoreFlags(uint64_t value)
{
    return value >= 0x80 ? 0x80 : 0x00;
}

uint8_t WriteCompressedUint64(uint8_t* ptr, uint64_t value)
{
    const auto* start = ptr;
    do
    {
        *ptr++ = (value & 0x7F) | MoreFlags(value);
        value >>= 7;
    } while (value);

    return ptr - start;
}

uint64_t ReadCompressedUint64(const uint8_t* ptr, uint8_t& outValueSize)
{
    const auto* start = ptr;
    auto singleByte = *ptr++;
    uint64_t ret = singleByte & 0x7F;
    uint32_t offset = 7;

    while (singleByte & 0x80)
    {
        singleByte = *ptr++;
        ret |= (singleByte & 0x7F) << offset;
        offset += 7;
    }

    outValueSize = ptr - start;
    return ret;
}

uint64_t SerializationOpBase::CalcSize(const SerializationOpBase* op)
{
    if (op->op == SerializationOpcode::DataRaw)
    {
        uint8_t valueSize = 0;
        const auto dataSize = ReadCompressedUint64((const uint8_t*)op + sizeof(SerializationOpDataRaw), valueSize);
        return dataSize + valueSize + sizeof(SerializationOpDataRaw);
    }

    switch (op->op)
    {
#define SERIALIZATION_OPCODE(x) case SerializationOpcode::##x: return sizeof(SerializationOp##x);
#include "serializationStream.inl"
#undef SERIALIZATION_OPCODE
    }

    return 0;
}

//--

uint64_t SerializationOpDataRaw::dataSize() const
{
    const auto* ptr = (const uint8_t*)this + 1;
    uint8_t size = 0;
    return ReadCompressedUint64(ptr, size);
}

const void* SerializationOpDataRaw::data() const
{
    const auto* ptr = (const uint8_t*)this + 1;
    uint8_t size = 0;
    ReadCompressedUint64(ptr, size);
    ptr += size;
    return ptr;
}

//--

ISerializationStreamVisitor::~ISerializationStreamVisitor()
{}

//--

SerializationStreamIterator::SerializationStreamIterator(const SerializationStream* stream /*= nullptr*/)
    : m_stream(stream)
{
    if (m_stream && !m_stream->m_pages.empty())
    {
        m_pos = m_stream->m_pages[0].base;
        m_end = m_stream->m_pages[0].cur;
        m_pageIndex = 0;
    }
}

SerializationStreamIterator::SerializationStreamIterator(const SerializationStream* stream, const SerializationOpBase* firstOpcode, const SerializationOpBase* lastOpcode)
    : m_stream(stream)
    , m_finalOpcode(lastOpcode)
{
    m_pageIndex = stream->findPageForData(firstOpcode);
    ASSERT_EX(m_pageIndex != -1, "Opcode not in stream");

    m_pos = (const uint8_t*)firstOpcode;
    m_end = m_stream->m_pages[m_pageIndex].cur;
}

SerializationStreamIterator::SerializationStreamIterator(const SerializationStreamIterator& other) = default;
SerializationStreamIterator& SerializationStreamIterator::operator=(const SerializationStreamIterator& other) = default;

void SerializationStreamIterator::advance()
{
    if (m_pos)
    {
        const auto* op = (const SerializationOpBase*)m_pos;

        if (op == m_finalOpcode)
        {
            m_pos = nullptr;
            m_end = nullptr;
            m_pageIndex = -1;
            return;
        }

        const auto size = SerializationOpBase::CalcSize(op);
        m_pos += size;

        ASSERT_EX(m_pos <= m_end, "Mallformed opcode stream");

        if (m_pos >= m_end)
        {
            m_pageIndex += 1;

            if (m_pageIndex < (int)m_stream->m_pages.size())
            {
                m_pos = m_stream->m_pages[m_pageIndex].base;
                m_end = m_stream->m_pages[m_pageIndex].cur;
            }
            else
            {
                m_pos = nullptr;
                m_end = nullptr;
                m_pageIndex = -1;
            }
        }
    }
}

//--

SerializationStream::SerializationStream()
{
    m_pages.reserve(4);
}

SerializationStream::~SerializationStream()
{
    freePages();
}

void SerializationStream::freePages()
{
    for (auto* op : m_inlinedBuffers)
        op->~SerializationOpDataInlineBuffer();

    for (auto* op : m_resourceReferences)
        op->~SerializationOpDataResourceRef();

    for (auto& page : m_pages)
    {
        const auto pageSize = page.cur - page.base;
        FreeSystemMemory(page.base, pageSize);
    }

    m_pages.clear();

    m_totalOpcodeSize = 0;
    m_totalOpcodeCount = 0;
    m_totalDataSize = 0;
    m_writePage = nullptr;
}

int SerializationStream::findPageForData(const void* dataPtr) const
{
    const auto* ptr = (const uint8_t*)dataPtr;

    for (uint32_t i = 0; i < m_pages.size(); ++i)
    {
        const auto& page = m_pages.typedData()[i];
        if (ptr >= page.base && ptr < page.cur)
            return i;
    }

    return -1;
}

bool SerializationStream::allocNewPage(uint64_t requiredSize)
{
    if (m_corrupted)
        return false;

    const auto minPageSize = 1U << 16;

    // open a new one
    const auto pageSize = std::max<uint64_t>(requiredSize, minPageSize);
    const auto largePages = pageSize > (2U << 20);
    void* memory = AllocSystemMemory(pageSize, largePages);
    if (nullptr == memory)
    {
        TRACE_ERROR("OutOfMemory when allocting additional page for serialization stream, page size {}, currently allocated {} in {} pages",
            MemSize(pageSize), MemSize(m_totalOpcodeSize), m_pages.size());
        freePages();
        return false;
    }

    // page allocated
    m_writePage = &m_pages.emplaceBack();
    m_writePage->base = (uint8_t*)memory;
    m_writePage->cur = m_writePage->base;
    m_writePage->end = m_writePage->base + pageSize;
    m_totalOpcodeSize += pageSize;
    return true;
}

void* SerializationStream::allocInternal(uint64_t size)
{
    if (m_corrupted)
        return nullptr;

    if (!m_writePage || m_writePage->cur + size > m_writePage->end)
        if (!allocNewPage(size))
            return nullptr;

    ASSERT(m_writePage->cur + size <= m_writePage->end);
    auto* ptr = m_writePage->cur;
    m_writePage->cur += size;
    m_totalOpcodeCount += 1;
    return ptr;
}

//--

SerializationStreamIterator SerializationStream::opcodes() const
{
    return SerializationStreamIterator(this);
}

void SerializationStream::dispatch(ISerializationStreamVisitor& dispatcher) const
{
    for (SerializationStreamIterator it(this); it; ++it)
        dispatcher.dispatchOpcode(*it);
}

class OpcodeExtraInfoPrinter : public ISerializationStreamVisitor
{
public:
    OpcodeExtraInfoPrinter(IFormatStream& f)
        : m_stream(f)
    {}

    virtual void processOpcode(const SerializationOpCompound& op) override
    {
        m_stream.appendf(" Type='{}'", op.type);
    }

    virtual void processOpcode(const SerializationOpArray& op) override
    {
        m_stream.appendf(" Size={}", op.count);
    }

    virtual void processOpcode(const SerializationOpProperty& op) override
    {
        m_stream.appendf(" Name='{}', Type={}", op.prop->name(), op.prop->type());
    }

    virtual void processOpcode(const SerializationOpDataName& op) override
    {
        m_stream.appendf(" Name='{}'", op.name);
    }

    virtual void processOpcode(const SerializationOpDataTypeRef& op) override
    {
        m_stream.appendf(" Type='{}'", op.type);
    }

    virtual void processOpcode(const SerializationOpDataResourceRef& op) override
    {
        if (op.type && op.id)
        {
            m_stream.appendf(" Type='{}', Id='{}'", op.type, op.id);

            if (op.async)
                m_stream.appendf(" ASYNC");
        }
        else
        {
            m_stream.appendf(" NoResource");
        }
    }

    virtual void processOpcode(const SerializationOpDataObjectPointer& op) override
    {
        if (op.object)
        {
            m_stream.appendf(" Type='{}', Pointer=0x{}", op.object->cls().name(), Hex((uint64_t)op.object));
        }
        else
        {
            m_stream.appendf(" NULL");
        }
    }

    virtual void processOpcode(const SerializationOpDataRaw& op) override
    {
        const auto* ptr = ((const uint8_t*)&op) + 1;

        uint8_t valueSize = 0;
        uint64_t size = ReadCompressedUint64(ptr, valueSize);
        ptr += valueSize;
        m_stream.appendf(" Size='{}' ", size);

        static const uint64_t MAX_PRINT = 32;
        const auto printSize = std::min<uint64_t>(MAX_PRINT, size);
        m_stream.appendHexBlock(ptr, printSize, 1);
    }

private:
    IFormatStream& m_stream;
};

void SerializationStream::print(IFormatStream& f)
{
    OpcodeExtraInfoPrinter printer(f);

    for (SerializationStreamIterator it(this); it; ++it)
    {
        const auto* op = *it;
        f.append(SerializationOpcodeName(op->op));
        printer.dispatchOpcode(op);
        f.append("\n");
    }
}

//--

END_BOOMER_NAMESPACE()
