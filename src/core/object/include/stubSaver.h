/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: stub #]
***/

#include "build.h"
#include "stub.h"

#include "core/memory/include/linearAllocator.h"
#include "core/containers/include/hashSet.h"

BEGIN_BOOMER_NAMESPACE()

//--

// stub mapper - explores all the stubs
class StubMapper : public IStubWriter
{
public:
	StubMapper();

	virtual void writeBool(bool b) override final {};
	virtual void writeInt8(char val) override final {};
	virtual void writeInt16(short val) override final {};
	virtual void writeInt32(int val) override final {};
	virtual void writeInt64(int64_t val) override final {};
	virtual void writeUint8(uint8_t val) override final {};
	virtual void writeUint16(uint16_t val) override final {};
	virtual void writeUint32(uint32_t val) override final {};
	virtual void writeUint64(uint64_t val) override final {};
	virtual void writeFloat(float val) override final {};
	virtual void writeDouble(double val) override final {};
	virtual void writeCompressedInt(int val) override final {};

	virtual void writeData(const void* data, uint32_t size) override final {};

	virtual void writeString(StringView str) override final;
	virtual void writeName(StringID name) override final;
	virtual void writeRef(const IStub* otherStub) override final;
	virtual void writeRefList(const IStub* const* otherStubs, uint32_t numRefs) override final;

	//--

	HashMap<const IStub*, uint32_t> m_stubMap;
	HashMap<StringBuf, uint16_t> m_stringMap;
	HashMap<StringID, uint16_t> m_nameMap;

	Array<const IStub*> m_stubs;
	Array<StringBuf> m_strings;
	Array<StringID> m_names;

	uint32_t m_firstUnprocessedObjects;
	Array<const IStub*> m_unprocessedObjects;
	HashSet<const IStub*> m_processedObjects;

	uint32_t m_additionalMemoryNeeded = 0;

	void processObject(const IStub* stub);
	bool processRemainingObjects();
};

//--

class StubDataWriter : public IStubWriter
{
public:
	StubDataWriter(const StubMapper& mapper, Array<uint8_t>& outStorage);

	virtual void writeBool(bool b) override final;
	virtual void writeInt8(char val) override final;
	virtual void writeInt16(short val) override final;
	virtual void writeInt32(int val) override final;
	virtual void writeInt64(int64_t val) override final;
	virtual void writeUint8(uint8_t val) override final;
	virtual void writeUint16(uint16_t val) override final;
	virtual void writeUint32(uint32_t val) override final;
	virtual void writeUint64(uint64_t val) override final;
	virtual void writeFloat(float val) override final;
	virtual void writeDouble(double val) override final;
	virtual void writeData(const void* data, uint32_t size) override final;
	virtual void writeCompressedInt(int val) override final;

	virtual void writeString(StringView str) override final;
	virtual void writeName(StringID name) override final;
	virtual void writeRef(const IStub* otherStub) override final;
	virtual void writeRefList(const IStub* const* otherStubs, uint32_t numRefs) override final;

private:
	const StubMapper& m_mapper;
	Array<uint8_t>& m_outStorage;

	template< typename T >
	ALWAYS_INLINE void writeTyped(T data)
	{
		*(T*)m_outStorage.allocateUninitialized(sizeof(T)) = data;
	}
};

//--

END_BOOMER_NAMESPACE()
