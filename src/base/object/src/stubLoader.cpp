/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: stubs #]
***/

#include "build.h"
#include "stub.h"
#include "stubFactory.h"
#include "stubLoader.h"
#include "base/containers/include/crc.h"

BEGIN_BOOMER_NAMESPACE(base)

//--

struct StubDataReaderTables
{
	base::Array<base::StringID> names;
	base::Array<base::StringView> strings;
	base::Array<IStub*> stubs;
	base::Array<StubTypeValue> stubTypes;

	uint32_t totalStubMemory = 0;
};

//--

template< typename T >
INLINE bool TryRead(const uint8_t*& ptr, const uint8_t* endPtr, T& outVal)
{
	if (ptr + sizeof(T) <= endPtr)
	{
		outVal = *(const T*)ptr;
		ptr += sizeof(T);
		return true;
	}

	return false;
}

static bool TryReadCompressedInt(const uint8_t*& ptr, const uint8_t* endPtr, int& outValue)
{
	if (ptr < endPtr)
	{
		int ret = 0;

		unsigned char byte = *ptr++;
		bool sign = (byte & 0x40) != 0;
		int ofs = 6;

		ret = byte & 0x3F;
		while (byte & 0x80)
		{
			if (ptr < endPtr)
			{
				byte = *ptr++;
				ret |= ((byte & 0x7F) << ofs);
				ofs += 7;
			}
			else
			{
				break;
			}
		}

		outValue = sign ? -ret : ret;
		return true;
	}

	return false;
}

static bool TryReadString(const uint8_t*& ptr, const uint8_t* endPtr, StringView& outView)
{
	int length = 0;
	if (TryReadCompressedInt(ptr, endPtr, length))
	{
		if (ptr + length <= endPtr)
		{
			outView = StringView((const char*)ptr, length);
			ptr += length;
			return true;
		}
	}

	return false;
}

//--

// in-memory reader
class StubDataReader : public IStubReader
{
public:
	StubDataReader(uint32_t version, const StubDataReaderTables& tables, const uint8_t*& objectMemPtr, const uint8_t* objectMemEndPtr, uint8_t*& additionalAllocationBlock, uint8_t* additionalAllocationBlockEndPtr)
		: IStubReader(version)
		, m_tables(tables)
		, m_objectMemPtr(objectMemPtr)
		, m_objectMemEndPtr(objectMemEndPtr)
		, m_additionalAllocationBlockPtr(additionalAllocationBlock)
		, m_additionalAllocationBlockEndPtr(additionalAllocationBlockEndPtr)
	{}

	//--

	INLINE bool hasErrors() const { return m_hasErrors; }
		
	//--

	virtual bool readBool() override final { return read<bool>(); }
	virtual char readInt8() override final { return read<char>(); }
	virtual short readInt16() override final { return read<short>(); }
	virtual int readInt32() override final { return read<int>(); }
	virtual int64_t readInt64() override final { return read<int64_t>(); }
	virtual uint8_t readUint8() override final { return read<uint8_t>(); }
	virtual uint16_t readUint16() override final { return read<uint16_t>(); }
	virtual uint32_t readUint32() override final { return read<uint32_t>(); }
	virtual uint64_t readUint64() override final { return read<uint64_t>(); }
	virtual float readFloat() override final { return read<float>(); }
	virtual double readDouble() override final { return read<double>(); }

	//--

	virtual int readCompressedInt() override final
	{
		int ret = 0;
		if (!m_hasErrors && TryReadCompressedInt(m_objectMemPtr, m_objectMemEndPtr, ret))
			return ret;

		if (!m_hasErrors)
		{
			TRACE_ERROR("Failed to read compressed int");
			m_hasErrors = true;
		}

		return ret;
	}

	virtual StringView readString() override final
	{
		int index = 0;
		if (!m_hasErrors && TryReadCompressedInt(m_objectMemPtr, m_objectMemEndPtr, index))
			if (index >= 0 && index <= m_tables.strings.lastValidIndex())
				return m_tables.strings[index];

		if (!m_hasErrors)
		{
			TRACE_ERROR("Failed to read string");
			m_hasErrors = true;
		}

		return "";
	}

	virtual StringID readName() override final
	{
		int index = 0;
		if (!m_hasErrors && TryReadCompressedInt(m_objectMemPtr, m_objectMemEndPtr, index))
			if (index >= 0 && index <= m_tables.names.lastValidIndex())
				return m_tables.names[index];

		if (!m_hasErrors)
		{
			TRACE_ERROR("Failed to read name");
			m_hasErrors = true;
		}

		return StringID();
	}

	virtual const IStub* readRef(StubTypeValue expectedType) override final
	{
		int index = 0;
		if (!m_hasErrors && TryReadCompressedInt(m_objectMemPtr, m_objectMemEndPtr, index))
		{
			if (index == 0)
				return nullptr;

			if (index > 0 && index <= m_tables.stubs.lastValidIndex())
			{
				const auto* stub = m_tables.stubs[index];
				if (stub)
				{
					//TRACE_INFO("Loaded reference {}: {}", index, stub->debugName());
					DEBUG_CHECK_EX(!expectedType || stub->runtimeType() == expectedType, base::TempString("Expected stub of type {}, got {} ({})", expectedType, stub->runtimeType(), stub->debugName()));
					return stub;
				}
			}
		}

		if (!m_hasErrors)
		{
			TRACE_ERROR("Failed to read reference");
			m_hasErrors = true;
		}

		return nullptr;
	}

	virtual const IStub** readRefList(uint32_t count) override final
	{
		const IStub** list = nullptr;

		if (count)
		{
			const auto additionalDataSize = sizeof(IStub*) * count;
			DEBUG_CHECK_RETURN_EX_V(tryAllocateAdditionalData(additionalDataSize, (void*&)list), "OOM on allocating additional data", nullptr);

			for (uint32_t i = 0; i < count; ++i)
				list[i] = readRef(0);
		}

		return list;
	}

	virtual const void* readData(uint32_t size) override final
	{
		const void* ret = nullptr;

		if (size)
		{
			DEBUG_CHECK_RETURN_EX_V(tryReadData(size, (void*&)ret), "Data block goes outside object boundary", nullptr);
		}

		return ret;
	}

private:
	const uint8_t*& m_objectMemPtr;
	const uint8_t* m_objectMemEndPtr = nullptr;
		
	uint8_t*& m_additionalAllocationBlockPtr;
	uint8_t* m_additionalAllocationBlockEndPtr = nullptr;

	const StubDataReaderTables& m_tables;

	template< typename T >
	INLINE T read(T defaultValue = T(0))
	{
		if (!m_hasErrors && m_objectMemPtr + sizeof(T) <= m_objectMemEndPtr)
		{
			auto* ptr = (const T*)m_objectMemPtr;
			m_objectMemPtr += sizeof(T);
			return *ptr;
		}

		if (!m_hasErrors)
		{
			TRACE_ERROR("Read of {} bytes out of intended memory range", sizeof(T));
			m_hasErrors = true;
		}

		return defaultValue;
	}

	INLINE bool tryAllocateAdditionalData(uint32_t size, void*& outData)
	{
		if (!m_hasErrors && (m_additionalAllocationBlockPtr + size <= m_additionalAllocationBlockEndPtr))
		{
			outData = m_additionalAllocationBlockPtr;
			m_additionalAllocationBlockPtr += size;
			return true;
		}

		if (!m_hasErrors)
		{
			TRACE_ERROR("Out of additional memory when allocating {} bytes", size);
			m_hasErrors = true;
		}
		return false;
	}

	INLINE bool tryReadData(uint32_t size, void*& outData)
	{
		if (!m_hasErrors && (m_objectMemPtr + size <= m_objectMemEndPtr))
		{
			outData = (void*) m_objectMemPtr;
			m_objectMemPtr += size;
			return true;
		}

		if (!m_hasErrors)
		{
			TRACE_ERROR("Read of {} bytes of data outside of intended memory range", size);
			m_hasErrors = true;
		}

		return false;
	}


	bool m_hasErrors = false;
};

//--

StubLoader::StubLoader(const StubFactory& factory, uint32_t minVersion, PoolTag tag /*= POOL_STUBS*/)
	: m_factory(factory)
	, m_minVersion(minVersion)
	, m_tag(tag)
{}

StubLoader::~StubLoader()
{
	clear();
}

void StubLoader::clear()
{
	for (auto* stub : m_stubsThatNeedDestruction)
		if (const auto* classInfo = m_factory.classInfo(stub->runtimeType()))
			if (classInfo->cleanupFunc)
				classInfo->cleanupFunc(stub);

	base::mem::FreeBlock(m_stubMem);
	m_stubMem = nullptr;

	base::mem::FreeBlock(m_unpackedMem);
	m_unpackedMem = nullptr;
}

struct StubLoaderCleanupHelper : public NoCopy
{
public:
	StubLoaderCleanupHelper(uint8_t* memory, const StubDataReaderTables& tables, const StubFactory& factory)
		: m_memory(memory)
		, m_tables(tables)
		, m_factory(factory)
	{}

	~StubLoaderCleanupHelper()
	{
		if (m_memory)
		{
			for (auto* stub : m_tables.stubs)
			{
				if (stub)
				{
					const auto* classInfo = m_factory.classInfo(stub->runtimeType());
					if (classInfo && classInfo->cleanupFunc)
						classInfo->cleanupFunc(stub);
				}
			}

			mem::FreeBlock(m_memory);
		}
	}

	void disable()
	{
		m_memory = nullptr;
	}

private:
	const StubDataReaderTables& m_tables;
	const StubFactory& m_factory;

	uint8_t* m_memory = nullptr;
};

const IStub* StubLoader::unpack(const Buffer& buffer)
{
	return unpack(buffer.data(), buffer.size());
}

const IStub* StubLoader::unpack(const void* packedData, uint32_t packedDataSize)
{
	PC_SCOPE_LVL1(UnpackStubs);

	clear();

	if (!packedData || !packedDataSize)
		return nullptr;

	ScopeTimer timer;

	static const auto headerSize = sizeof(StubHeader);
	DEBUG_CHECK_RETURN_EX_V(packedDataSize > headerSize, "Not enough data for even a header", nullptr);

	const auto* header = (const StubHeader*) packedData;
	DEBUG_CHECK_RETURN_EX_V(header->version >= m_minVersion, base::TempString("Shader blob is in version {} but minimal supported version is {}", header->version, m_minVersion), nullptr);

	const auto crc = CRC32().append((uint8_t*)packedData + headerSize, packedDataSize - headerSize).crc();
	DEBUG_CHECK_RETURN_EX_V(header->compressedCRC == crc, "Shader blob has invalid CRC than in the header. Data is possible corrupted. Loading would be unsafe.", nullptr);

	mem::AutoFreePtr unpackedMem(mem::AllocateBlock(m_tag, header->uncompressedSize, 4));
	DEBUG_CHECK_RETURN_EX_V(unpackedMem, "Failed to allocate buffer for decompress shader data.", nullptr);

	DEBUG_CHECK_RETURN_EX_V(base::Decompress(base::CompressionType::LZ4HC, header + 1, packedDataSize - headerSize, unpackedMem.ptr(), header->uncompressedSize), "Decompression failed", nullptr);
	TRACE_SPAM("Unpacked stub blob {} -> {}", MemSize(packedDataSize), MemSize(header->uncompressedSize));

	DEBUG_CHECK_RETURN_EX_V(header->numNames >= 1, "Invalid header counts", nullptr);
	DEBUG_CHECK_RETURN_EX_V(header->numStrings >= 1, "Invalid header counts", nullptr);
	DEBUG_CHECK_RETURN_EX_V(header->numStubs >= 1, "Invalid header counts", nullptr);

	//--

	// empty object case - legal
	if (header->numStubs == 1)
		return nullptr;

	//--

	StubDataReaderTables tables;
	tables.names.resizeWith(header->numNames, StringID());
	tables.strings.resizeWith(header->numStrings, StringView());
	tables.stubs.resizeWith(header->numStubs, nullptr);
	tables.stubTypes.resizeWith(header->numStubs, 0);
	TRACE_SPAM("Found {} strings, {} names and {} stubs in the blob", header->numStrings, header->numNames, header->numStubs);

	const uint8_t* dataPtr = (const uint8_t*)unpackedMem.ptr();
	const uint8_t* dataPtrEnd = dataPtr + header->uncompressedSize;

	// load strings
	for (uint32_t i = 1; i < header->numStrings; ++i)
	{
		StringView text;
		DEBUG_CHECK_RETURN_EX_V(TryReadString(dataPtr, dataPtrEnd, text), "EOF when reading string table", nullptr);
		tables.strings[i] = text;
	}

	// check marker
	{
		static const uint32_t MARKER = 'NAMS';
		uint32_t loadedMarker = 0;
		DEBUG_CHECK_RETURN_EX_V(TryRead(dataPtr, dataPtrEnd, loadedMarker), "EOF when reading string table", nullptr);
		DEBUG_CHECK_RETURN_EX_V(loadedMarker == MARKER, "Invalid file marker", nullptr);
	}

	// load names
	for (uint32_t i = 1; i < header->numNames; ++i)
	{
		StringView text;
		DEBUG_CHECK_RETURN_EX_V(TryReadString(dataPtr, dataPtrEnd, text), "EOF when reading name table", nullptr);
		tables.names[i] = text;
	}

	// check marker
	{
		static const uint32_t MARKER = 'TYPS';
		uint32_t loadedMarker = 0;
		DEBUG_CHECK_RETURN_EX_V(TryRead(dataPtr, dataPtrEnd, loadedMarker), "EOF when reading string table", nullptr);
		DEBUG_CHECK_RETURN_EX_V(loadedMarker == MARKER, "Invalid file marker", nullptr);
	}

	// load object types and create them
	uint32_t totalNeededMemory = 0;
	uint32_t totalNeededAlignment = 4;
	for (uint32_t i = 1; i < header->numStubs; ++i)
	{
		int type = 0;
		DEBUG_CHECK_RETURN_EX_V(TryReadCompressedInt(dataPtr, dataPtrEnd, type), "EOF when reading object table", nullptr);

		const auto* classInfo = m_factory.classInfo(type);
		DEBUG_CHECK_RETURN_EX_V(classInfo, "Invalid stub type", nullptr);

		auto ofs = Align(totalNeededMemory, classInfo->alignment);
		totalNeededAlignment = std::max<uint32_t>(totalNeededAlignment, classInfo->alignment);
		totalNeededMemory += classInfo->size;

		tables.stubTypes[i] = type;
	}
	TRACE_SPAM("Found {} memory needed for stubs, {} needed alignment ({} additional data)", MemSize(totalNeededMemory), totalNeededAlignment, header->additionalDataSize);

	// allocate the memory blob
	auto* objectMemoryBlock = (uint8_t*)mem::AllocateBlock(m_tag, totalNeededMemory + header->additionalDataSize, totalNeededAlignment);
	uint8_t* objectMemoryPtr = objectMemoryBlock;
	uint8_t* objectMemoryEndPtr = objectMemoryPtr + totalNeededMemory;
	uint8_t* additionalMemoryPtr = objectMemoryEndPtr;
	uint8_t* additionalMemoryEndPtr = additionalMemoryPtr + header->additionalDataSize;

	// in case we exit make sure everything is cleaned up
	StubLoaderCleanupHelper cleanupHelper(objectMemoryPtr, tables, m_factory);

	// zero memory
	// TODO: technically we can live without it but system is heave enough that this does not change much
	memset(objectMemoryPtr, 0, totalNeededMemory + header->additionalDataSize);

	// create stubs
	{
		PC_SCOPE_LVL1(CreateStubs);
		for (uint32_t i = 1; i < header->numStubs; ++i)
		{
			const auto type = tables.stubTypes[i];
			const auto* classInfo = m_factory.classInfo(type);

			auto* ptr = AlignPtr(objectMemoryPtr, classInfo->alignment);
			DEBUG_CHECK_RETURN_EX_V(ptr + classInfo->size <= objectMemoryEndPtr, "Incorrect object size calculation", nullptr);

			classInfo->initFunc(ptr);
			tables.stubs[i] = (IStub*)ptr;

			objectMemoryPtr = ptr + classInfo->size;
		}
	}

	// load stub content
	{
		PC_SCOPE_LVL1(LoadStubs);
		for (uint32_t i = 1; i < header->numStubs; ++i)
		{
			// check marker
			{
				static const uint32_t MARKER = 'STUB';
				uint32_t loadedMarker = 0;
				DEBUG_CHECK_RETURN_EX_V(TryRead(dataPtr, dataPtrEnd, loadedMarker), "EOF when reading string table", nullptr);
				DEBUG_CHECK_RETURN_EX_V(loadedMarker == MARKER, "Invalid file marker", nullptr);
			}

			int size = 0;
			DEBUG_CHECK_RETURN_EX_V(TryReadCompressedInt(dataPtr, dataPtrEnd, size), "EOF when reading object size", nullptr);
			DEBUG_CHECK_RETURN_EX_V(dataPtr + size <= dataPtrEnd, "Object data goes outside memory buffer", nullptr);

			{
				const auto startPtr = dataPtr;
				const auto expectedEndPtr = dataPtr + size;
				StubDataReader reader(header->version, tables, dataPtr, expectedEndPtr, additionalMemoryPtr, additionalMemoryEndPtr);

				auto* stub = tables.stubs[i];
				//TRACE_SPAM("Reading stub {}: {}", i, stub->debugName());
				stub->read(reader);

				const auto sizeRead = dataPtr - startPtr;
				DEBUG_CHECK_RETURN_EX_V(!reader.hasErrors(), base::TempString("Stub '{}' at index {}: read error at {} of size {}", stub->debugName(), i, sizeRead, size), nullptr);
				DEBUG_CHECK_RETURN_EX_V(sizeRead == size, base::TempString("Stub '{}' at index {}: read {} bytes of expected {}", stub->debugName(), i, sizeRead, size), nullptr);
			}
		}
	}

	// post load all objects
	{
		PC_SCOPE_LVL1(PostLoadStubs);
		for (uint32_t i = 1; i < header->numStubs; ++i)
			tables.stubs[i]->postLoad();
	}

	// move data to final buffers
	m_unpackedMem = unpackedMem.detach();
	m_stubMem = objectMemoryBlock;
	cleanupHelper.disable();

	// stats
	TRACE_INFO("Lodaed {} stubs from {} ({} packed) in {}", tables.stubs.size(), MemSize(header->uncompressedSize), MemSize(packedDataSize), timer);
	return tables.stubs[1]; // root object
}

//--

END_BOOMER_NAMESPACE(base)
