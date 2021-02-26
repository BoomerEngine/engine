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

BEGIN_BOOMER_NAMESPACE()

//----

StubFactory::StubFactory()
{}

StubFactory::~StubFactory()
{
	m_classes.clearPtr();
}

void StubFactory::registerType(StubTypeValue id, uint32_t size, uint32_t alignment, const std::function<void(void*)>& initStubFunc, const std::function<void(void*)>& cleanupStubFunc)
{
	DEBUG_CHECK_RETURN_EX(id != 0, "Stub ID=0 is reserved");
	DEBUG_CHECK_RETURN_EX(size != 0, "Stub size seems invalid");
	DEBUG_CHECK_RETURN_EX(alignment >= 1 && alignment <= 16, "Stub alignment is unsupported");

	m_classes.prepareWith(id + 1, nullptr);
	DEBUG_CHECK_RETURN_EX(m_classes[id] == nullptr, "Class already reigstered");

	auto entry = new StubClass();
	entry->size = size;
	entry->alignment = alignment;
	entry->initFunc = initStubFunc;
	entry->cleanupFunc = cleanupStubFunc;
	m_classes[id] = entry;
}

const StubFactory::StubClass* StubFactory::classInfo(StubTypeValue id) const
{
	DEBUG_CHECK_RETURN_EX_V(id < m_classes.size(), "Invalid/unregistered stub class", nullptr);

	const auto* entry = m_classes[id];
	DEBUG_CHECK_RETURN_EX_V(entry != nullptr, "Stub class with given ID not registered", nullptr);

	return entry;
}

//--

END_BOOMER_NAMESPACE()
