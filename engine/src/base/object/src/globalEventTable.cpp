/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: event #]
***/

#include "build.h"
#include "globalEventKey.h"
#include "globalEventFunction.h"
#include "globalEventTable.h"
#include "globalEventDispatch.h"

namespace base
{
    //--

    IGlobalEventTable::IGlobalEventTable()
    {}

    IGlobalEventTable::~IGlobalEventTable()
    {
        clear();
    }

    void IGlobalEventTable::clear()
    {
        if (m_table)
        {
            for (auto& entry : m_table->entries)
                UnregisterGlobalEventListener(entry);
            delete m_table;
            m_table = nullptr;
        }
    }

    void IGlobalEventTable::bind(const GlobalEventListenerPtr& ptr)
    {
        if (ptr)
        {
            if (!m_table)
                m_table = new Table;

            bool added = false;
            for (auto& entry : m_table->entries)
            {
                if (entry->name() == ptr->name() && entry->key() == ptr->key())
                {
                    if (ptr != entry)
                    {
                        UnregisterGlobalEventListener(entry);
                        entry = ptr;
                        RegisterGlobalEventListener(ptr);
                    }
                    added = true;
                    break;
                }
            }

            if (!added)
            {
                m_table->entries.pushBack(ptr);
                RegisterGlobalEventListener(ptr);
            }
        }
    }

    void IGlobalEventTable::unbind(GlobalEventKey key)
    {
        if (m_table)
        {
            for (auto index : m_table->entries.indexRange().reversed())
            {
                auto& entry = m_table->entries[index];
                if (entry->key() == key)
                {
                    UnregisterGlobalEventListener(entry);
                    m_table->entries.eraseUnordered(index);
                }
            }

            if (m_table->entries.empty())
            {
                delete m_table;
                m_table = nullptr;
            }
        }
    }

    void IGlobalEventTable::unbind(StringID name)
    {
        if (m_table)
        {
            for (auto index : m_table->entries.indexRange().reversed())
            {
                auto& entry = m_table->entries[index];
                if (entry->name() == name)
                {
                    UnregisterGlobalEventListener(entry);
                    m_table->entries.eraseUnordered(index);
                }
            }

            if (m_table->entries.empty())
            {
                delete m_table;
                m_table = nullptr;
            }
        }
    }

    void IGlobalEventTable::unbind(GlobalEventKey key, StringID name)
    {
        if (m_table)
        {
            for (auto index : m_table->entries.indexRange().reversed())
            {
                auto& entry = m_table->entries[index];
                if (entry->name() == name && entry->key() == key)
                {
                    UnregisterGlobalEventListener(entry);
                    m_table->entries.eraseUnordered(index);
                }
            }

            if (m_table->entries.empty())
            {
                delete m_table;
                m_table = nullptr;
            }
        }
    }

    //--
     
    class SyncDispatcher : public IGlobalEventListener
    {
    public:
        SyncDispatcher(GlobalEventKey key, StringID eventName, const RefWeakPtr<IReferencable>& contextObject, bool strictOrdering)
            : IGlobalEventListener(key, eventName)
            , m_context(contextObject)
            , m_strictOrdering(strictOrdering)
        {}

        INLINE TGlobalEventFunction& func()
        {
            return m_func;
        }

        virtual void dispatch(base::IObject* source, const void* data, base::Type dataType) const
        {
            auto context = m_context.lock();
            if (!m_context || context) // context object we want to call the function on must be valid
            {
                if (m_strictOrdering || !Fibers::GetInstance().isMainFiber())
                {
                    rtti::DataHolder holder(dataType, data);
                    auto sourceRef = RefPtr<IObject>(AddRef(source));
                    auto contextRef = m_context;
                    auto eventName = name();
                    auto eventFunc = m_func;

                    RunSync("SyncGlobalEventDispatcher") << [contextRef, eventFunc, sourceRef, eventName, holder](FIBER_FUNC)
                    {
                        auto context = contextRef.lock();
                        if (!contextRef || context) // context object we want to call the function on must be valid
                        {
                            eventFunc(eventName, sourceRef, holder.data(), holder.type());
                        }
                    };
                }
                else
                {
                    m_func(name(), source, data, dataType);
                }
            }
        }

    private:
        RefWeakPtr<IReferencable> m_context;
        bool m_strictOrdering = false;

        TGlobalEventFunction m_func;
    };

    GlobalEventTable::GlobalEventTable(IReferencable* contextObject /*= nullptr*/, bool strictOrdering /*= false*/)
        : m_context(contextObject)
        , m_strictOrdering(strictOrdering)
    {
    }

    GlobalEventTable::~GlobalEventTable()
    {}

    GlobalEventFunctionBinder GlobalEventTable::bind(GlobalEventKey key, StringID eventName)
    {
        if (key && eventName)
        {
            auto entry = RefNew<SyncDispatcher>(key, eventName, m_context, m_strictOrdering );
            IGlobalEventTable::bind(entry);
            return GlobalEventFunctionBinder(&entry->func(), entry);
        }

        return GlobalEventFunctionBinder(nullptr, nullptr);
    }

    
    //--

} // base

