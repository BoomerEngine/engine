/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "glObject.h"
#include "glObjectRegistry.h"
#include "glDeviceThread.h"

namespace rendering
{
    namespace gl4
    {
        //--

        base::ConfigProperty<uint32_t> cvMaxObjects("Rendering.GL4", "MaxObjects", 128 * 1024);

        //--

        ObjectRegistry::ObjectRegistry(Device* drv, DeviceThread* drvThread)
            : m_device(drv)
            , m_thread(drvThread)
        {
            m_proxy = base::RefNew<ObjectRegistryProxy>(this);

            m_numObjects = std::max<uint32_t>(1024, cvMaxObjects.get());
            m_objects = base::mem::GlobalPool<POOL_API_OBJECTS, Entry>::AllocN(m_numObjects);
            memzero(m_objects, sizeof(Entry) * m_numObjects);
            TRACE_INFO("GL: Creating object registring with {} slots", m_numObjects);

            {
                auto runningCounter = m_numObjects - 1;
                m_freeEntries.resize(m_numObjects);
                for (auto& index : m_freeEntries)
                    index = runningCounter--;
            }
        }

        ObjectRegistry::~ObjectRegistry()
        {
            m_proxy->disconnect();
            m_proxy.reset();

            if (m_numAllocatedObjects > 0)
            {
                TRACE_WARNING("GL: There are still {} live API objects, deleting them", m_numAllocatedObjects);

                for (uint32_t i = 0; i < m_numObjects; ++i)
                {
                    auto& obj = m_objects[i];
                    if (obj.ptr && !obj.markedForDeletion)
                    {
                        obj.markedForDeletion = true;
                        m_thread->releaseObject(obj.ptr);
                    }
                }

                m_thread->sync(); // this should actually delete all objects
                m_thread->sync(); // this should actually delete all objects

                DEBUG_CHECK_EX(m_numAllocatedObjects == 0, "Not all objects deleted");

                for (uint32_t i=0; i<m_numObjects; ++i)
                    DEBUG_CHECK_EX(m_objects[i].ptr == nullptr, "Not all objects deleted");
            }
        }

        ObjectID ObjectRegistry::registerObject(Object* ptr)
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

            TRACE_SPAM("GL: Registered object {}", m_generationCounter);

            return ObjectID::CreateStaticID(index, m_generationCounter);
        }

        void ObjectRegistry::unregisterObject(ObjectID id, Object* ptr)
        {
            DEBUG_CHECK_RETURN(ptr);
            DEBUG_CHECK_RETURN(!id.empty());
            DEBUG_CHECK_RETURN(id.index() < MAX_OBJECTS);
            DEBUG_CHECK_RETURN(id.generation() <= m_generationCounter);

            DEBUG_CHECK(m_numAllocatedObjects > 0);
            m_numAllocatedObjects -= 1;

            auto lock = CreateLock(m_lock);

            auto& entry = m_objects[id.index()];
            DEBUG_CHECK_RETURN(entry.ptr == ptr);
            DEBUG_CHECK(entry.markedForDeletion);

            m_objects[id.index()].ptr = nullptr;
            m_objects[id.index()].markedForDeletion = false;
            m_freeEntries.pushBack(id.index());

            TRACE_SPAM("GL: Unregistered object {}", id.generation());
        }

        Object* ObjectRegistry::resolveStatic(ObjectID id, ObjectType expectedType) const
        {
            if (!id.empty())
            {
                DEBUG_CHECK_RETURN_V(id.isStatic(), nullptr);
                DEBUG_CHECK_RETURN_V(id.index() < MAX_OBJECTS, nullptr);
                DEBUG_CHECK_RETURN_V(id.generation() <= m_generationCounter, nullptr);

                auto lock = CreateLock(m_lock);

                auto& entry = m_objects[id.index()];
                if (entry.ptr && entry.ptr->handle() == id)
                {
                    DEBUG_CHECK_RETURN_V(expectedType == ObjectType::Invalid || entry.ptr->objectType() == expectedType, nullptr);
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
                if (!entry.markedForDeletion)
                {
                    TRACE_SPAM("GL: Marked object {} for deletion", id.generation());

                    entry.markedForDeletion = true;
                    m_thread->releaseObject(entry.ptr);
                }
            }
        }

        bool ObjectRegistry::runWithObject(ObjectID id, const std::function<void(Object*)>& func)
        {
            DEBUG_CHECK_RETURN_V(!id.empty(), false);
            DEBUG_CHECK_RETURN_V(id.index() < MAX_OBJECTS, false);
            DEBUG_CHECK_RETURN_V(id.generation() <= m_generationCounter, false);

            auto lock = CreateLock(m_lock);

            auto& entry = m_objects[id.index()];
            if (entry.ptr && entry.ptr->handle() == id)
            {
                DEBUG_CHECK_RETURN_V(!entry.markedForDeletion, false);
                func(entry.ptr);
                return true;
            }

            return false;
        }

        //--

        ObjectRegistryProxy::ObjectRegistryProxy(ObjectRegistry* target)
            : m_registry(target)
        {}

        void ObjectRegistryProxy::disconnect()
        {
            auto lock = CreateLock(m_lock);
            m_registry = nullptr;
        }

        void ObjectRegistryProxy::releaseToDevice(ObjectID id)
        {
            auto lock = CreateLock(m_lock);
            if (m_registry)
                m_registry->requestObjectDeletion(id);
        }

        bool ObjectRegistryProxy::runWithObject(ObjectID id, const std::function<void(Object*)>& func)
        {
            auto lock = CreateLock(m_lock);
            if (m_registry)
                return m_registry->runWithObject(id, func);
            else
                return false;
        }

        //--

    } // gl4
} // rendering
