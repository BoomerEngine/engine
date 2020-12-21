/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: stubs #]
***/

#include "build.h"
#include "stub.h"
#include "stubBuilder.h"
#include "stubFactory.h"

#include "base/memory/include/linearAllocator.h"

namespace base
{

    //----

	StubBuilder::StubBuilder(mem::LinearAllocator& mem, const StubFactory& factory)
		: m_mem(mem)
		, m_factory(factory)
	{}

	StubBuilder::~StubBuilder()
	{
		clear();
	}

	void* StubBuilder::createData(uint32_t size)
	{
		return m_mem.alloc(size, 4);
	}

	void StubBuilder::clear()
	{
		for (IStub* stub : m_stubsToDestroy)
		{
			if (const auto* entry = m_factory.classInfo(stub->runtimeType()))
				entry->cleanupFunc(stub);
		}			

		m_stubsToDestroy.reset();
	}

	IStub* StubBuilder::createStub(StubTypeValue id)
	{
		if (!id)
			return nullptr;

		const auto* entry = m_factory.classInfo(id);
		DEBUG_CHECK_RETURN_EX_V(entry, "Invalid stub type", nullptr);

		auto* ptr = (IStub*) m_mem.alloc(entry->size, entry->alignment);
		entry->initFunc(ptr);

		if (entry->cleanupFunc)
			m_stubsToDestroy.pushBack(ptr);

		DEBUG_CHECK_RETURN_EX_V(ptr->runtimeType() == id, TempString("Stub got created with type {} but registered as type {}", ptr->runtimeType(), id), nullptr);

		return ptr;
	}

	void* StubBuilder::allocateInternal(uint32_t size)
	{
		return m_mem.alloc(size, 4);
	}

	//--


	//--

} // base
