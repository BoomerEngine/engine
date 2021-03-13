/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
*/

#include "build.h"
#include "uiElement.h"
#include "uiEventFunction.h"
#include "uiEventTable.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//----

EventTable::EventTable()
{
    m_entries.reserve(4);
}

EventTable::~EventTable()
{
    clear();
}

void EventTable::clear()
{
    m_entries.clear();
}

void EventTable::clearByOwner(IElement* owner)
{
    for (auto& table : m_entries.values())
        for (int i = table.lastValidIndex(); i >= 0; --i)
            if (table[i]->owner == owner)
                table.erase(i);
}
    
void EventTable::clearByName(StringID name)
{
    m_entries.remove(name);
}

EventFunctionBinder EventTable::bind(StringID name, IElement* owner)
{
    DEBUG_CHECK_EX(name, "Event should have a name");
    if (!name)
        return EventFunctionBinder(nullptr);

    // use existing
    if (owner)
    {
        if (auto* table = m_entries.find(name))
            for (auto& entry : *table)
                if (entry->owner == owner)
                    return &entry->func;
    }

    // create new entry
    auto entry = RefNew<Entry>();
    entry->owner = owner;
    m_entries[name].pushBack(entry);

    // return for registering a function
    return &entry->func;
}

void EventTable::unbind(StringID name, IElement* owner)
{
    DEBUG_CHECK_EX(name, "Event should have a name");
    DEBUG_CHECK_EX(owner, "Only owned events can be unbound");
    if (!name || !owner)
        return;

    if (auto* table = m_entries.find(name))
    {
        for (int i = table->lastValidIndex(); i >= 0; --i)
        {
            if ((*table)[i]->owner == owner)
            {
                table->erase(i);
                break;
            }
        }
    }
}

bool EventTable::call(StringID name, IElement* source)
{
    return callGeneric(name, source, Variant::EMPTY());
}

bool EventTable::callGeneric(StringID name, IElement* source, const Variant& data)
{
    bool handled = false;

    if (auto* list = m_entries.find(name))
    {
        InplaceArray<EntryWeakPtr, 16> callbacks;

        // collect callback for non expired targets
        callbacks.reserve(list->size());
        for (const auto& entry : *list)
            if (entry->owner.empty() || !entry->owner.expired())
                callbacks.pushBack(entry.weak());

        // call the callbacks, but only for valid object and only if callback has not yet been deleted in the mean time
        for (const auto& callbackWeak : callbacks)
        {
            if (auto callback = callbackWeak.lock())
            {
                auto owner = callback->owner.lock();
                if (owner || callback->owner.empty())
                {
                    if (callback->func)
                        callback->func(name, source, owner.get(), data);
                    handled = true;
                }
            }
            else
            {
                TRACE_WARNING("Callback for '{}' lost during iteration", name);
            }
        }
    }

    return handled;
}

//----

END_BOOMER_NAMESPACE_EX(ui)
 