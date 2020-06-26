/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: object #]
***/

#include "build.h"
#include "object.h"
#include "objectObserverEventDispatcher.h"

#include "base/object/include/object.h"
#include "base/object/include/objectObserver.h"
#include "base/system/include/scopeLock.h"

namespace base
{
    ///--

    ObjectObserverEventDispatcher::ObjectObserverEventDispatcher()
    {
        m_objects.reserve(16*1024);
        m_pendingEventsSwap.reserve(1024);
        m_pendingEvents.reserve(1024);
    }

    void ObjectObserverEventDispatcher::deinit()
    {
        {
            ScopeLock<Mutex> lock(m_objectsLock);
            m_objects.clearPtr();
        }

        {
            ScopeLock<SpinLock> lock(m_pendingEventsLock);
            m_pendingEvents.clear();
            m_pendingEventsSwap.clear();
        }
    }

    void ObjectObserverEventDispatcher::discardPendingEvents()
    {
        ScopeLock<SpinLock> lock(m_pendingEventsLock);
        m_pendingEvents.clear();
    }

    void ObjectObserverEventDispatcher::processPendingEvents()
    {
        // get the events to send
        m_pendingEventsLock.acquire();
        std::swap(m_pendingEventsSwap, m_pendingEvents); // the "Swap" will now contain the events
        m_pendingEventsLock.release();

        ScopeLock<Mutex> lock(m_objectsLock);

        // send events
        for (auto& event : m_pendingEventsSwap)
        {
            // find the target
            ObjectInfo* info = nullptr;
            if (m_objects.find(event.objectId, info))
            {
                // call the event
                bool hasEmptyListeners = false;
                auto numListeners = info->listeners.size();
                for (uint32_t i=0; i<numListeners; ++i)
                {
                    auto listener  = info->listeners[i];
                    if (listener)
                    {
                        listener->onObjectChangedEvent(event.eventID, event.object.get(), event.path.view(), event.data);
                    }
                    else
                    {
                        hasEmptyListeners = true;
                    }
                }

                // some listeners were removed, compact the list
                // NOTE: the list may be purged which will mean that the nobody is listening for the events on this object any more
                if (hasEmptyListeners)
                    info->listeners.remove(nullptr);

                // no listeners for this object, remove it from the list
                if (info->listeners.empty())
                    m_objects.remove(event.objectId);
            }
        }

        // reset the event list without freeing the memory
        m_pendingEventsSwap.reset();
    }

    uint32_t ObjectObserverEventDispatcher::registerListener(const IObject* ptr, IObjectObserver* listener)
    {
         // only non-empty objects are tracked
        if (ptr)
        {
            ScopeLock<Mutex> lock(m_objectsLock);

            // get container for given object
            ObjectInfo* info = nullptr;
            if (!m_objects.find(ptr->id(), info))
            {
                // create container
                info = MemNew(ObjectInfo);
                info->listeners.reserve(2);
                info->ptr = ptr;
                m_objects.set(ptr->id(), info);
            }

            // register listener
            ASSERT_EX(!info->listeners.contains(listener), "Listener already registered");
            info->listeners.pushBackUnique(listener);

            // keep track of registration count for each listener
            listener->addTrackingReference();
            return ptr->id();
        }

        return 0;
    }

    void ObjectObserverEventDispatcher::unregisterListener(uint32_t objectId, IObjectObserver* listener)
    {
        if (0 != objectId)
        {
            ScopeLock<Mutex> lock(m_objectsLock);

            // get container for given object
            // NOTE: if container does not exist the listener was not registered :)
            ObjectInfo* info = nullptr;
            if (m_objects.find(objectId, info))
            {
                // null the entry
                for (auto & entry : info->listeners)
                {
                    if (entry == listener)
                    {
                        listener->removeTrackingReference();
                        entry = nullptr;
                    }
                }
            }
        }
    }

    void ObjectObserverEventDispatcher::postEvent(StringID eventID, uint32_t objectId, IObject* obj, StringView<char> path, rtti::DataHolder eventData)
    {
        // only valid sources are tracked
        if (0 != objectId && obj)
        {
            ScopeLock<SpinLock> lock(m_pendingEventsLock);

            // post event to the table
            auto &eventInfo = m_pendingEvents.emplaceBack();
            eventInfo.eventID = eventID;
            eventInfo.objectId = objectId;
            eventInfo.object = AddRef(obj);
            eventInfo.path = StringBuf(path);
            eventInfo.data = std::move(eventData);

            //TRACE_INFO("Posted '{}' with path '{}' on object '{}'", eventID, eventPath, objectID);
        }
    }

    void IObjectObserver::DispatchPendingEvents()
    {
        ObjectObserverEventDispatcher::GetInstance().processPendingEvents();
    }

    uint32_t IObjectObserver::RegisterObserver(const IObject* obj, IObjectObserver* observer)
    {
        return ObjectObserverEventDispatcher::GetInstance().registerListener(obj, observer);
    }

    void IObjectObserver::UnregisterObserver(uint32_t id, IObjectObserver* observer)
    {
        ObjectObserverEventDispatcher::GetInstance().unregisterListener(id, observer);
    }

    //--

} // base

