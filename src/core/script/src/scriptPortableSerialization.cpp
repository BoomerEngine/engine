/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptPortableStubs.h"
#include "scriptPortableData.h"
#include "scriptPortableSerialization.h"

#include "core/script/include/scriptPortableStubs.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//--

ConfigProperty<bool> cvDumpStubStats("Script.Compiler", "DumpStubStats", false);

//--

template< typename T >
static void PrintStubStats(const Array<T>& stubs)
{
    uint32_t counts[(uint32_t)StubType::MAX];
    memset(counts, 0, sizeof(counts));

    for (auto stub  : stubs)
        if (stub)
            counts[(uint8_t)stub->stubType] += 1;

    for (uint32_t i=0; i<ARRAY_COUNT(counts); ++i)
        if (counts[i] != 0)
            TRACE_INFO("  {}: {}", (StubType)i, counts[i]);
}

//--

StubMapper::StubMapper()
{
    m_stubs.reserve(2048);
    m_strings.reserve(2048);
    m_names.reserve(2048);

    m_stubMap.reserve(2048);
    m_stringMap.reserve(2048);
    m_nameMap.reserve(2048);

    m_stubMap[nullptr] = 0;
    m_stubs.pushBack(nullptr);

    m_strings.pushBack(StringBuf::EMPTY());
    m_names.pushBack(StringID());

    m_firstUnprocessedObjects = 0;
}

void StubMapper::writeString(StringView str)
{
    if (!str.empty())
    {
        auto hash = str.calcCRC64();
        if (!m_stringMap.contains(hash))
        {
            auto id = m_strings.size();
            m_strings.emplaceBack(str);
            m_stringMap[hash] = id;
        }
    }
}

void StubMapper::writeName(StringID name)
{
    if (!name.empty())
    {
        if (!m_nameMap.contains(name))
        {
            auto id = m_names.size();
            m_names.pushBack(name);
            m_nameMap[name] = id;
        }
    }
}

void StubMapper::writeRef(const Stub* otherStub)
{
    if (otherStub)
    {
        if (!m_stubMap.contains(otherStub))
        {
            auto id = m_stubs.size();
            m_stubs.pushBack(otherStub);
            m_stubMap[otherStub] = id;

            processObject(otherStub);
        }
    }
}

void StubMapper::processObject(const Stub* stub)
{
    if (stub)
        if (m_processedObjects.insert(stub))
            m_unprocessedObjects.pushBack(stub);
}

bool StubMapper::processRemainingObjects()
{
    if (m_firstUnprocessedObjects == m_unprocessedObjects.size())
        return false;

    while (m_firstUnprocessedObjects < m_unprocessedObjects.size())
    {
        auto obj  = m_unprocessedObjects[m_firstUnprocessedObjects++];
        obj->write(*this);
    }

    return true;
}

//--

StubDataWriter::StubDataWriter(const StubMapper& mapper, PagedBuffer& outMemory)
    : m_mapper(mapper)
    , m_outMemory(outMemory)
{}

void StubDataWriter::writeBool(bool b)
{
    *m_outMemory.allocSmall<uint8_t>() = b ? 1 : 0;
}

void StubDataWriter::writeInt8(char val)
{
    *m_outMemory.allocSmall<char>() = val;
}

void StubDataWriter::writeInt16(short val)
{
	*m_outMemory.allocSmall<short>() = val;
}

void StubDataWriter::writeInt32(int val)
{
	*m_outMemory.allocSmall<int>() = val;
}

void StubDataWriter::writeInt64(int64_t val)
{
	*m_outMemory.allocSmall<int64_t>() = val;
}

void StubDataWriter::writeUint8(uint8_t val)
{
	*m_outMemory.allocSmall<uint8_t>() = val;
}

void StubDataWriter::writeUint16(uint16_t val)
{
	*m_outMemory.allocSmall<uint16_t>() = val;
}

void StubDataWriter::writeUint32(uint32_t val)
{
	*m_outMemory.allocSmall<uint32_t>() = val;
}

void StubDataWriter::writeUint64(uint64_t val)
{
	*m_outMemory.allocSmall<uint64_t>() = val;
}

void StubDataWriter::writeFloat(float val)
{
	*m_outMemory.allocSmall<float>() = val;
}

void StubDataWriter::writeDouble(double val)
{
	*m_outMemory.allocSmall<double>() = val;
}

void StubDataWriter::writeRawString(StringView txt)
{
    writeUint16(txt.length());
	m_outMemory.writeSmall(txt.data(), txt.length());
}

void StubDataWriter::writeString(StringView str)
{
    uint16_t stringId = 0;
    if (!str.empty())
    {
        auto hash = str.calcCRC64();
        if (!m_mapper.m_stringMap.find(hash, stringId))
        {
            FATAL_ERROR("Unmapped string");
        }
    }

	*m_outMemory.allocSmall<uint16_t>() = stringId;
}

void StubDataWriter::writeName(StringID name)
{
    uint16_t nameId = 0;
    if (!name.empty())
    {
        if (!m_mapper.m_nameMap.find(name, nameId))
        {
            FATAL_ERROR("Unmapped name");
        }
    }

	*m_outMemory.allocSmall<uint16_t>() = nameId;
}

void StubDataWriter::writeRef(const Stub* otherStub)
{
    uint32_t objectId = 0;
    if (otherStub)
    {
        if (!m_mapper.m_stubMap.find(otherStub, objectId))
        {
            FATAL_ERROR("Unmapped object");
        }
    }

	*m_outMemory.allocSmall<uint16_t>() = objectId;
}

void StubDataWriter::writeContainers()
{
    // write names
    writeUint16(m_mapper.m_names.size());
    for (uint32_t i=1; i<m_mapper.m_names.size(); ++i)
        writeRawString(m_mapper.m_names[i].view());

    // write strings
    writeUint16(m_mapper.m_strings.size());
    for (uint32_t i=1; i<m_mapper.m_strings.size(); ++i)
        writeRawString(m_mapper.m_strings[i].view());

    // write object types
    writeUint32(m_mapper.m_stubs.size());
    for (uint32_t i=1; i<m_mapper.m_stubs.size(); ++i)
        //m_outMemory.writeSmall((uint8_t)m_mapper.m_stubs[i]->stubType);

    // write objects
    for (uint32_t i=1; i<m_mapper.m_stubs.size(); ++i)
        m_mapper.m_stubs[i]->write(*this);

    // stats
    if (cvDumpStubStats.get())
    {
        TRACE_INFO("Saved script stubs:");
        PrintStubStats(m_mapper.m_stubs);
    }
}

//--

StubDataReader::StubDataReader(const void* data, uint32_t dataSize, mem::LinearAllocator& unpackedMem)
    : m_mem(unpackedMem)
    , m_hasErrors(false)
{
    m_readPtr = (const uint8_t*)data;
    m_endPtr = m_readPtr + dataSize;
}

bool StubDataReader::validateRead(uint32_t size)
{
    if (m_readPtr + size <= m_endPtr)
        return true;

    if (!m_hasErrors)
    {
        TRACE_ERROR("ScriptDataError: Out of bound read in packed script data");
        m_hasErrors = true;
    }

    return false;
}


bool StubDataReader::readBool()
{
    return readUint8() != 0;
}

char StubDataReader::readInt8()
{
    if (!validateRead(1))
        return 0;

    return read<char>();
}

short StubDataReader::readInt16()
{
    if (!validateRead(2))
        return 0;

    return read<short>();
}

int StubDataReader::readInt32()
{
    if (!validateRead(4))
        return 0;

    return read<int>();
}

int64_t StubDataReader::readInt64()
{
    if (!validateRead(8))
        return 0;

    return read<int64_t>();
}

uint8_t StubDataReader::readUint8()
{
    if (!validateRead(1))
        return 0;

    return read<uint8_t>();
}

uint16_t StubDataReader::readUint16()
{
    if (!validateRead(2))
        return 0;

    return read<uint16_t>();
}

uint32_t StubDataReader::readUint32()
{
    if (!validateRead(4))
        return 0;

    return read<uint32_t>();
}

uint64_t StubDataReader::readUint64()
{
    if (!validateRead(8))
        return 0;

    return read<uint64_t>();
}

float StubDataReader::readFloat()
{
    if (!validateRead(4))
        return 0;

    return read<float>();
}

double StubDataReader::readDouble()
{
    if (!validateRead(8))
        return 0;

    return read<double>();
}

StringBuf StubDataReader::readString()
{
    auto id = readUint16();
    if (id == 0)
        return StringBuf();

    if (id >= m_strings.size())
    {
        TRACE_ERROR("ScriptDataError: Invalid string ID {}", id);
        return StringBuf();
    }

    return m_strings[id];
}

StringID StubDataReader::readName()
{
    auto id = readUint16();
    if (id == 0)
        return StringID();

    if (id >= m_names.size())
    {
        TRACE_ERROR("ScriptDataError: Invalid name ID {}", id);
        return StringID();
    }

    return m_names[id];
}

const Stub* StubDataReader::readRef()
{
    auto id = readUint32();
    if (id == 0)
        return nullptr;

    if (id >= m_stubs.size())
    {
        TRACE_ERROR("ScriptDataError: Invalid object ID {}", id);
        return nullptr;
    }

    return m_stubs[id ];
}

StringView StubDataReader::readRawString()
{
    auto len = readUint16();
    if (len == 0)
        return StringView();

    if (!validateRead(len))
        return StringView();

    auto chars  = (char*)m_mem.alloc(len+1, 1);
    memcpy(chars, m_readPtr, len);
    chars[len] = 0;
    m_readPtr += len;

    return StringView(chars, len);
}

const StubModule* StubDataReader::exportedModule() const
{
    for (auto stub  : m_stubs)
        if (stub && stub->stubType == StubType::Module && !stub->flags.test(StubFlag::Import))
            return stub->asModule();

    return nullptr;
}

void StubDataReader::allStubs(Array<const Stub*>& outAllStubs) const
{
    outAllStubs.reserve(m_stubs.size());

    for (auto stub  : m_stubs)
        if (stub && stub->stubType != StubType::Opcode)
            outAllStubs.pushBack(stub);
}

bool StubDataReader::readContainers()
{
    // reset
    m_names.clear();
    m_strings.clear();
    m_stubs.clear();
    m_mem.clear();
    m_hasErrors = false;

    // load names
    m_names.resize(readUint16());
    for (uint32_t i=1; i<m_names.size(); ++i)
        m_names[i] = StringID(readRawString());
    if (m_hasErrors)
        return false;

    // load strings
    m_strings.resize(readUint16());
    for (uint32_t i=1; i<m_strings.size(); ++i)
        m_strings[i] = StringBuf(readRawString());
    if (m_hasErrors)
        return false;

    // create objects
    m_stubs.resizeWith(readUint32(), nullptr);
    for (uint32_t i=1; i<m_stubs.size(); ++i)
    {
        auto type = (StubType)readUint8();
        m_stubs[i] = Stub::Create(m_mem, type);
        if (!m_stubs[i])
            return false;
    }

    // load objects
    for (uint32_t i=1; i<m_stubs.size(); ++i)
    {
        m_stubs[i]->read(*this);

        if (m_hasErrors)
            return false;
    }

    // post load object
    for (auto stub  : m_stubs)
        if (stub)
            stub->postLoad();

    // loaded
    if (cvDumpStubStats.get())
    {
        TRACE_INFO("Loaded script stubs:");
        PrintStubStats(m_stubs);
    }
    return true;
}

END_BOOMER_NAMESPACE_EX(script)
