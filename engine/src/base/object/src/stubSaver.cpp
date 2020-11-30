/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: stub #]
***/

#include "build.h"
#include "stub.h"
#include "stubSaver.h"

#include "base/containers/include/hashSet.h"
#include "base/containers/include/pagedBuffer.h"

namespace base
{

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

		m_strings.pushBack(base::StringBuf::EMPTY());
		m_names.pushBack(base::StringID());

		m_firstUnprocessedObjects = 0;
	}

	void StubMapper::writeString(base::StringView str)
	{
		if (!str.empty())
		{
			if (!m_stringMap.contains(str))
			{
				auto id = m_strings.size();
				m_strings.emplaceBack(str);
				m_stringMap[base::StringBuf(str)] = id;
			}
		}
	}

	void StubMapper::writeName(base::StringID name)
	{
		if (!name.empty())
		{
			if (!m_nameMap.contains(name))
			{
				auto id = m_names.size();
				m_names.pushBack(name);
				m_nameMap[name] = id;

				writeString(name.view()); // just because string was a name does not mean it is not a string
			}
		}
	}

	void StubMapper::writeRef(const IStub* otherStub)
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

	void StubMapper::writeRefList(const IStub* const* otherStubs, uint32_t numRefs)
	{
		for (uint32_t i = 0; i < numRefs; ++i)
			writeRef(otherStubs[i]);

		m_additionalMemoryNeeded += sizeof(const IStub*) * numRefs;
	}

	void StubMapper::processObject(const IStub* stub)
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
			auto obj = m_unprocessedObjects[m_firstUnprocessedObjects++];
			obj->write(*this);
		}

		return true;
	}

	//--

	StubDataWriter::StubDataWriter(const StubMapper& mapper, Array<uint8_t>& outStorage)
		: m_mapper(mapper)
		, m_outStorage(outStorage)
	{}

	void StubDataWriter::writeBool(bool b)
	{
		writeTyped<uint8_t>(b ? 1 : 0);
	}

	void StubDataWriter::writeInt8(char val)
	{
		writeTyped(val);
	}

	void StubDataWriter::writeInt16(short val)
	{
		writeTyped(val);
	}

	void StubDataWriter::writeInt32(int val)
	{
		writeTyped(val);
	}

	void StubDataWriter::writeInt64(int64_t val)
	{
		writeTyped(val);
	}

	void StubDataWriter::writeUint8(uint8_t val)
	{
		writeTyped(val);
	}

	void StubDataWriter::writeUint16(uint16_t val)
	{
		writeTyped(val);
	}

	void StubDataWriter::writeUint32(uint32_t val)
	{
		writeTyped(val);
	}

	void StubDataWriter::writeUint64(uint64_t val)
	{
		writeTyped(val);
	}

	void StubDataWriter::writeFloat(float val)
	{
		writeTyped(val);
	}

	void StubDataWriter::writeDouble(double val)
	{
		writeTyped(val);
	}

	void StubDataWriter::writeCompressedInt(int val)
	{
		bool sign = 0;

		unsigned int s = val;
		if (val < 0) {
			s = -val;
			sign = true;
		}

		uint8_t buffer[10];
		uint8_t length = 0;

		buffer[length++] = (s & 0x3F) | (s >= 0x40 ? 0x80 : 0x00) | (sign ? 0x40 : 0x00);
		s >>= 6;
		while (s) {
			buffer[length++] = (s & 0x7F) | (s >= 0x80 ? 0x80 : 0x00);
			s >>= 7;
		}

		m_outStorage.pushBack(buffer, length);
	}

	void StubDataWriter::writeData(const void* data, uint32_t size)
	{
		if (size)
		{
			auto* targetData = m_outStorage.allocateUninitialized(size);
			memcpy(targetData, data, size);
		}
	}

	void StubDataWriter::writeString(base::StringView str)
	{
		uint16_t stringId = 0;
		if (!str.empty())
		{
			DEBUG_CHECK_EX(m_mapper.m_stringMap.find(str, stringId), "Saving unmapped string");
		}

		writeCompressedInt(stringId);
	}

	void StubDataWriter::writeName(base::StringID name)
	{
		uint16_t nameId = 0;
		if (!name.empty())
		{
			DEBUG_CHECK_EX(m_mapper.m_nameMap.find(name, nameId), "Saving unmapped name");
		}

		writeCompressedInt(nameId);
	}

	void StubDataWriter::writeRef(const IStub* otherStub)
	{
		uint32_t objectId = 0;
		if (otherStub)
		{
			if (!m_mapper.m_stubMap.find(otherStub, objectId))
			{
				FATAL_ERROR("Unmapped object");
			}
		}

		writeCompressedInt(objectId);
	}

	void StubDataWriter::writeRefList(const IStub* const* otherStubs, uint32_t numRefs)
	{
		writeCompressedInt(numRefs);
		for (uint32_t i = 0; i < numRefs; ++i)
			writeRef(otherStubs[i]);
	};

	//--

	static void WriteCompressedInt(PagedBuffer& outBuffer, int val)
	{
		bool sign = 0;

		unsigned int s = val;
		if (val < 0) {
			s = -val;
			sign = true;
		}

		uint8_t buffer[10];
		uint8_t length = 0;

		buffer[length++] = (s & 0x3F) | (s >= 0x40 ? 0x80 : 0x00) | (sign ? 0x40 : 0x00);
		s >>= 6;
		while (s) {
			buffer[length++] = (s & 0x7F) | (s >= 0x80 ? 0x80 : 0x00);
			s >>= 7;
		}

		outBuffer.writeSmall(buffer, length);
	}

	static void WriteString(PagedBuffer& outBuffer, StringView text)
	{
		WriteCompressedInt(outBuffer, text.length());
		outBuffer.writeSmall(text.data(), text.length());
	}

	//--

	Buffer IStub::pack(uint32_t version) const
	{
		PC_SCOPE_LVL1(PackStubs);

		ScopeTimer timer;

		// map everything visible from the stub
		StubMapper mapper;
		{
			ScopeTimer mapTimer;
			uint32_t mapPasses = 0;

			PC_SCOPE_LVL1(Map);
			mapper.writeRef(this);
			while (mapper.processRemainingObjects())
				mapPasses += 1;

			TRACE_SPAM("Collected {} stubs ({} passes) in {}", mapper.m_stubs.size(), mapPasses, mapTimer);
		}

		// write data
		PagedBuffer uncompressedData(1, POOL_STUBS);
		{
			PC_SCOPE_LVL1(SaveUncompressed);

			ScopeTimer timer;

			// write strings
			for (uint32_t i = 1; i < mapper.m_strings.size(); ++i)
				WriteString(uncompressedData, mapper.m_strings[i]);

			// write markers
			{
				uint32_t MARKER = 'NAMS';
				uncompressedData.writeSmall(&MARKER, sizeof(MARKER));
			}

			// write names
			for (uint32_t i = 1; i < mapper.m_names.size(); ++i)
				WriteString(uncompressedData, mapper.m_names[i].view());

			// write markers
			{
				uint32_t MARKER = 'TYPS';
				uncompressedData.writeSmall(&MARKER, sizeof(MARKER));
			}

			// write stub types
			for (uint32_t i = 1; i < mapper.m_stubs.size(); ++i)
				WriteCompressedInt(uncompressedData, mapper.m_stubs[i]->runtimeType());

			// write stub data
			Array<uint8_t> objectStorage;
			objectStorage.reserve(1024);
			for (uint32_t i = 1; i < mapper.m_stubs.size(); ++i)
			{
				const auto* stub = mapper.m_stubs[i];

				// write marker
				{
					uint32_t MARKER = 'STUB';
					uncompressedData.writeSmall(&MARKER, sizeof(MARKER));
				}

				// write object
				{
					objectStorage.reset();

					StubDataWriter writer(mapper, objectStorage);
					stub->write(writer);

					WriteCompressedInt(uncompressedData, objectStorage.size());
					uncompressedData.writeLarge(objectStorage.data(), objectStorage.dataSize());
				}
			}

			TRACE_SPAM("Saved {} stubs to {} in {}", mapper.m_stubs.size(), MemSize(uncompressedData.dataSize()), timer);
		}

		// compress data
		auto compressedData = base::Compress(CompressionType::LZ4HC, uncompressedData.toBuffer(), POOL_STUBS);
		DEBUG_CHECK_RETURN_EX_V(compressedData, "Failed to compress stub data", nullptr);

		// assemble final buffer
		auto finalData = Buffer::Create(POOL_STUBS, sizeof(StubHeader) + compressedData.size());

		// save header
		auto* header = (StubHeader*)finalData.data();
		header->version = version;
		header->magic = StubHeader::MAGIC;
		header->numNames = mapper.m_names.size();
		header->numStrings = mapper.m_strings.size();
		header->numStubs = mapper.m_stubs.size();
		header->additionalDataSize = mapper.m_additionalMemoryNeeded;
		header->compressedCRC = CRC32().append(compressedData.data(), compressedData.size()).crc();
		header->uncompressedSize = uncompressedData.dataSize();

		// copy data
		memcpy(finalData.data() + sizeof(StubHeader), compressedData.data(), compressedData.size());

		TRACE_SPAM("Packed {} stubs to {} final compressed buffer in {}", mapper.m_stubs.size(), MemSize(finalData.size()), timer);
		return finalData;
	}

	//--

} // base
