/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: object #]
***/

#include "build.h"
#include "object.h"
#include "objectObserver.h"
#include "objectObserverEventDispatcher.h"

namespace base
{
    //---

    std::atomic<uint32_t> AllActiveListeners = 0;

    IObjectObserver::IObjectObserver()
    {
        ++AllActiveListeners;
    }

    IObjectObserver::~IObjectObserver()
    {
        //ASSERT_EX(registrationCount.load() == 0, "IObjectObserver was not unregistered before being removed");
        --AllActiveListeners;
    }

    void IObjectObserver::addTrackingReference()
    {
        ++registrationCount;
    }

    void IObjectObserver::removeTrackingReference()
    {
        --registrationCount;
    }

    //---

    ObjectObserver::ObjectObserver(StringID eventName)
        : observedEvent(eventName)
    {}

    ObjectObserver::~ObjectObserver()
    {
        unbind();
    }

    void ObjectObserver::unbind()
    {
        if (observedObjectId != 0)
        {
            IObjectObserver::UnregisterObserver(observedObjectId, this);
            observedObjectId = 0;
        }
    }

    void ObjectObserver::bindObject(IObject* object)
    {
        unbind();
        observedObjectId = IObjectObserver::RegisterObserver(object, this);
    }

    void ObjectObserver::bindCallback(const TObjectEventFunction& func)
    {
        callback = func;
    }

    void ObjectObserver::onObjectChangedEvent(OBJECT_EVENT_FUNC)
    {
        if (callback)
        {
            if (!observedEvent || observedEvent == eventID)
            {
                DEBUG_CHECK_EX(observedObjectId == object->id(), "Trying to call event on different object than registered with");

                auto temp = callback;
                temp(eventID, object, eventPath, eventData);
            }
        }
    }

    //---

} // base

