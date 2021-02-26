/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: stubs #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

class StubFactory;

//---

// helper class to unpack stub structure from memory blob
class CORE_OBJECT_API StubLoader : public NoCopy
{
public:
	StubLoader(const StubFactory& factory, uint32_t minVersion, PoolTag tag = POOL_STUBS);
	~StubLoader();

	// clear data, releases allocated memory (will call destructor on stubs that need it)
	void clear();

	// unpack packed stubs, returns pointer to the root packed object
	// NOTE: we don't need the packed data once we unpack
	// NOTE: it may fail if we have version change or there's any other error
	const IStub* unpack(const void* packedData, uint32_t packedDataSize);

	// same as unpack but takes buffer
	const IStub* unpack(const Buffer& buffer);

private:
	const StubFactory& m_factory;

	void* m_unpackedMem = nullptr; // unpacked buffer, contains stub definitions + strings + inlined data
	void* m_stubMem = nullptr; // memory with stub objects

	uint32_t m_minVersion = 1;

	PoolTag m_tag;

	Array<IStub*> m_stubsThatNeedDestruction;
};

//---

END_BOOMER_NAMESPACE()
