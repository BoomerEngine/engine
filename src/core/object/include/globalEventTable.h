/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: event #]
***/

#pragma once

#include "globalEventKey.h"
#include "globalEventFunction.h"

BEGIN_BOOMER_NAMESPACE()

///--

/// global event listener, can register to listener on various events on various objects
class CORE_OBJECT_API IGlobalEventTable : public NoCopy
{
public:
    IGlobalEventTable();
    virtual ~IGlobalEventTable();

    /// unregister from all events
    void clear();

    /// unbind particular key
    void unbind(GlobalEventKey key);

    /// unbind particular event name
    void unbind(StringID name);

    /// unbind particular event
    void unbind(GlobalEventKey key, StringID name);

protected:
    struct Table : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_EVENTS)

    public:
        Array<GlobalEventListenerPtr> entries;
    };

    Table* m_table = nullptr;

    void bind(const GlobalEventListenerPtr& ptr);
};

//--

/// Global event listener that dispatches events on main thread only
/// If any given event was received on a thread different than main than the even is moved to main thread (NOTE: this may break the order of events)
/// Optionally the events, will only be dispatched on main thread if the given context object is still alive
class CORE_OBJECT_API GlobalEventTable : public IGlobalEventTable
{
public:
    GlobalEventTable(IReferencable* contextObject = nullptr, bool strictOrdering = false);
    ~GlobalEventTable();

    /// bind function to event
    GlobalEventFunctionBinder bind(GlobalEventKey key, StringID eventName);

private:
    RefWeakPtr<IReferencable> m_context;
    bool m_strictOrdering = false;
};

//--

END_BOOMER_NAMESPACE()
