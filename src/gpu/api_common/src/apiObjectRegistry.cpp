/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiObject.h"
#include "apiObjectRegistry.h"
#include "apiThread.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

ConfigProperty<uint32_t> cvMaxObjects("Rendering", "MaxApiObjects", 128 * 1024);

//--

ObjectRegistry::ObjectRegistry(IBaseThread* owner)
    : m_owner(owner)
{
    m_numObjects = std::max<uint32_t>(1024, cvMaxObjects.get());
    m_objects = GlobalPool<POOL_API_OBJECTS, Entry>::AllocN(m_numObjects);
    memzero(m_objects, sizeof(Entry) * m_numObjects);
    TRACE_INFO("Creating object registring with {} slots", m_numObjects);

    {
        auto runningCounter = m_numObjects - 1;
        m_freeEntries.resize(m_numObjects);
        for (auto& index : m_freeEntries)
            index = runningCounter--;
    }
}

ObjectRegistry::~ObjectRegistry()
{
    DEBUG_CHECK_EX(m_numAllocatedObjects == 0, "Not all objects deleted");

    for (uint32_t i=0; i<m_numObjects; ++i)
        DEBUG_CHECK_EX(m_objects[i].ptr == nullptr, "Not all objects deleted");
}

void ObjectRegistry::purge()
{
	if (m_numAllocatedObjects > 0)
	{
		TRACE_WARNING("There are still {} live API objects, deleting them", m_numAllocatedObjects);

		for (uint32_t i = 0; i < m_numObjects; ++i)
		{
			auto& obj = m_objects[i];
			if (obj.ptr && !obj.markedForDeletion && obj.ptr->canDelete())
			{
				TRACE_INFO("Purging {}({}) for deletion", obj.ptr, obj.ptr->objectType());
				obj.markedForDeletion = true;
				m_owner->scheduleObjectForDestruction(obj.ptr);
			}
		}
	}
}

ObjectID ObjectRegistry::registerObject(IBaseObject* ptr)
{
    DEBUG_CHECK_RETURN_V(ptr, ObjectID());

    auto lock = CreateLock(m_lock);

    ASSERT_EX(!m_freeEntries.empty(), "To many object GL objects");

    auto index = m_freeEntries.back();
    m_freeEntries.popBack();

    DEBUG_CHECK(m_objects[index].ptr == nullptr);
    DEBUG_CHECK(!m_objects[index].markedForDeletion);
    m_objects[index].ptr = ptr;
    m_objects[index].markedForDeletion = false;
    m_numAllocatedObjects += 1;
    m_generationCounter += 1;

    TRACE_SPAM("Registered object {}", m_generationCounter);

    return ObjectID(index, m_generationCounter, ptr);
}

void ObjectRegistry::unregisterObject(ObjectID id, IBaseObject* ptr)
{
    DEBUG_CHECK_RETURN(ptr);
    DEBUG_CHECK_RETURN(!id.empty());
    DEBUG_CHECK_RETURN(id.index() < MAX_OBJECTS);
    DEBUG_CHECK_RETURN(id.generation() <= m_generationCounter);

    auto lock = CreateLock(m_lock);

    DEBUG_CHECK(m_numAllocatedObjects > 0);
    m_numAllocatedObjects -= 1;

    auto& entry = m_objects[id.index()];
    DEBUG_CHECK_RETURN(entry.ptr == ptr);
	DEBUG_CHECK(entry.markedForDeletion == ptr->canDelete());

    m_objects[id.index()].ptr = nullptr;
    m_objects[id.index()].markedForDeletion = false;
    m_freeEntries.pushBack(id.index());

    TRACE_SPAM("Unregistered object {}", id.generation());
}

IBaseObject* ObjectRegistry::resolveStatic(ObjectID id, ObjectType expectedType) const
{
    if (!id.empty())
    {
        DEBUG_CHECK_RETURN_V(id.index() < MAX_OBJECTS, nullptr);
        DEBUG_CHECK_RETURN_V(id.generation() <= m_generationCounter, nullptr);

        auto lock = CreateLock(m_lock);

        auto& entry = m_objects[id.index()];
        if (entry.ptr && entry.ptr->handle() == id)
        {
            DEBUG_CHECK_RETURN_V(expectedType == ObjectType::Unknown || entry.ptr->objectType() == expectedType, nullptr);
            return entry.ptr;
        }
    }

    return nullptr;
}

void ObjectRegistry::requestObjectDeletion(ObjectID id)
{
    DEBUG_CHECK_RETURN(!id.empty());
    DEBUG_CHECK_RETURN(id.index() < MAX_OBJECTS);
    DEBUG_CHECK_RETURN(id.generation() <= m_generationCounter);

    auto lock = CreateLock(m_lock);

    auto& entry = m_objects[id.index()];
    if (entry.ptr && entry.ptr->handle() == id)
    {
		if (entry.ptr->canDelete())
		{
			if (!entry.markedForDeletion)
			{
				TRACE_SPAM("Marked object {} for deletion", id.generation());

				entry.ptr->disconnectFromClient();

				entry.markedForDeletion = true;
				m_owner->scheduleObjectForDestruction(entry.ptr);
			}
		}
    }
}

void ObjectRegistry::releaseToDevice(ObjectID id)
{
	requestObjectDeletion(id);
}

api::IBaseObject* ObjectRegistry::resolveInternalObjectPtrRaw(ObjectID id, uint8_t objectType)
{
	// TODO: any additional safety?
	return resolveStatic(id, (ObjectType)objectType);
}        

//--

END_BOOMER_NAMESPACE_EX(gpu::api)
