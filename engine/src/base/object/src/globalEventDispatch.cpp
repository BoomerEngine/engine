/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: event #]
***/

#include "build.h"
#include "globalEventFunction.h"
#include "globalEventTable.h"
#include "globalEventKey.h"
#include "globalEventDispatch.h"
#include "base/containers/include/inplaceArray.h"

namespace base
{
    //--

    IGlobalEventListener::IGlobalEventListener(GlobalEventKey key, StringID eventName)
        : m_key(key)
        , m_eventName(eventName)
    {}

    IGlobalEventListener::~IGlobalEventListener()
    {}

    //--

    namespace prv
    {
        static mem::PoolID POOL_EVENTS("Engine.Events");

        struct DispatchList
        {
            DispatchList(StringID name)
                : m_name(name)
            {}

            INLINE StringID name() const
            {
                return m_name;
            }

            void dispatch(base::IObject* source, const void* data, base::Type dataType)
            {
                InplaceArray<RefWeakPtr<IGlobalEventListener>, 20> validListeners;

                for (auto entry : m_list)
                    if (entry && !entry.expired())
                        validListeners.pushBack(entry);

                for (auto entry : validListeners)
                    if (auto listener = entry.lock())
                        listener->dispatch(source, data, dataType);
            }

            void add(IGlobalEventListener* listener)
            {
                if (listener)
                    m_list.pushBackUnique(listener);
            }

            bool remove(IGlobalEventListener* listener)
            {
                m_list.remove(listener);
                return m_list.empty();
            }

        private:
            Array<RefWeakPtr<IGlobalEventListener>> m_list;
            StringID m_name;
        };

        struct KeyEntry
        {
            KeyEntry(GlobalEventKey key)
            {}

            void dispatch(StringID name, base::IObject* source, const void* data, base::Type dataType)
            {
                for (auto* list : m_lists)
                    if (list->name() == name)
                        list->dispatch(source, data, dataType);
            }

            void add(StringID name, IGlobalEventListener* listener)
            {
                for (auto* list : m_lists)
                {
                    if (list->name() == name)
                    {
                        list->add(listener);
                        return;
                    }
                }

                auto* list = MemNewPool(POOL_EVENTS, DispatchList, name).ptr;
                m_lists.pushBack(list);
                list->add(listener);
            }

            bool remove(StringID name, IGlobalEventListener* listener)
            {
                for (auto* list : m_lists)
                {
                    if (list->name() == name)
                    {
                        if (list->remove(listener))
                            m_lists.remove(list);
                        break;
                    }
                }

                return m_lists.empty();
            }

        private:
            GlobalEventKey m_key;
            Array<DispatchList*> m_lists;
        };

        class GlobalEventRegistry : public ISingleton
        {
            DECLARE_SINGLETON(GlobalEventRegistry);

        public:
            GlobalEventRegistry()
            {
                m_keyMaps.reserve(1024);
            }

            void registerListener(IGlobalEventListener* listener)
            {
                if (listener && listener->key() && listener->name())
                {
                    auto lock = CreateLock(m_lock);

                    KeyEntry* key = nullptr;
                    if (!m_keyMaps.find(listener->key(), key))
                    {
                        key = MemNewPool(POOL_EVENTS, KeyEntry, listener->key());
                        m_keyMaps[listener->key()] = key;
                    }

                    key->add(listener->name(), listener);
                }
            }

            void unregisterListener(IGlobalEventListener* listener)
            {
                if (listener && listener->key() && listener->name())
                {
                    auto lock = CreateLock(m_lock);

                    KeyEntry* key = nullptr;
                    if (m_keyMaps.find(listener->key(), key))
                    {
                        if (key->remove(listener->name(), listener))
                        {
                            m_keyMaps.remove(listener->key());
                            MemDelete(key);
                        }
                    }
                }
            }

            void dispatch(GlobalEventKey keyName, StringID eventName, IObject* source, const void* data /*= nullptr*/, Type dataType /*= Type()*/)
            {
                if (keyName && eventName)
                {
                    auto lock = CreateLock(m_lock);

                    KeyEntry* key = nullptr;
                    if (m_keyMaps.find(keyName, key))
                    {
                        TRACE_INFO("GlobalEvent: Dispatching {} from {}", eventName, keyName);
                        key->dispatch(eventName, source, data, dataType);
                    }
                }
            }

        private:
            Mutex m_lock;
            HashMap<GlobalEventKey, KeyEntry*> m_keyMaps;

            virtual void deinit() override
            {
                m_keyMaps.clearPtr();
            }
        };

    };

    //--

    void RegisterGlobalEventListener(IGlobalEventListener* listener)
    {
        prv::GlobalEventRegistry::GetInstance().registerListener(listener);
    }

    void UnregisterGlobalEventListener(IGlobalEventListener* listener)
    {
        prv::GlobalEventRegistry::GetInstance().unregisterListener(listener);
    }

    void DispatchGlobalEvent(GlobalEventKey key, StringID eventName, IObject* source /*= nullptr*/, const void* data /*= nullptr*/, Type dataType /*= Type()*/)
    {
        prv::GlobalEventRegistry::GetInstance().dispatch(key, eventName, source, data, dataType);
    }

    //--

} // base

