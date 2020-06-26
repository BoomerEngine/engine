/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once

#include "object.h"
#include "objectObserverEventDispatcher.h"
#include "rttiDataHolder.h"

#include "base/system/include/spinLock.h"
#include "base/system/include/mutex.h"
#include "base/system/include/atomic.h"
#include "base/containers/include/hashMap.h"

namespace base
{

    /// Global event tracker for object events
    class ObjectObserverEventDispatcher : public ISingleton
    {
        DECLARE_SINGLETON(ObjectObserverEventDispatcher);

    public:
        ObjectObserverEventDispatcher();

        ///---

        /// discard all pending events
        /// NOTE: this may have adverse effects on the may systems, should be used as last resort
        void discardPendingEvents();

        /// process all pending events
        /// NOTE: must be called from safe location - ie. main thread
        /// NOTE: by default called manually by local service container
        void processPendingEvents();

        ///---

        /// register per-object listener for given object ID
        /// NOTE: listener listens to all events on the given object
        /// NOTE: it's allowed to register the same listener multiple times
        uint32_t registerListener(const IObject* ptr, IObjectObserver* listener);

        /// unregister previously registered per-object event listener
        void unregisterListener(uint32_t objectId, IObjectObserver* listener);

        ///---

        /// post global event
        /// NOTE: event may be dropped if there are no listeners registered to listen for it
        void postEvent(StringID eventID, uint32_t objectId, IObject* obj, StringView<char> path, rtti::DataHolder eventData);

    protected:
        struct PendingEvent
        {
            StringID eventID;
            uint32_t objectId;
            ObjectPtr object; // once we agree to call the event we will keep object alive until it executed
            StringBuf path;
            rtti::DataHolder data;
        };

        struct ObjectInfo
        {
            Array<IObjectObserver*> listeners;
            ObjectWeakPtr ptr;
        };

        HashMap<uint32_t, ObjectInfo*> m_objects;
        Mutex m_objectsLock;

        Array<PendingEvent> m_pendingEvents;
        Array<PendingEvent> m_pendingEventsSwap;
        SpinLock m_pendingEventsLock;

        virtual void deinit() override;
    };

} // base

