/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#include "uiEventFunction.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///---

/// table of events 
class ENGINE_UI_API EventTable : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_EVENTS)

public:
    EventTable();
    ~EventTable();

    /// clear all event bindings
    void clear();

    /// clear all bindings owned by given element
    void clearByOwner(IElement* owner);

    /// clear all bindings by given event name
    void clearByName(StringID name);

    //---

    /// bind event
    EventFunctionBinder bind(StringID name, IElement* owner);

    /// unbind event
    void unbind(StringID name, IElement* owner);

    //---

    // call event with no data
    bool call(StringID name, IElement* source);

    // call event with generic data
    bool callGeneric(StringID name, IElement* source, const Variant& data);

    // call event with specific data
    template< typename T >
    INLINE bool call(StringID name, IElement* source, const T& data)
    {
        return callGeneric(name, source, CreateVariant(data));
    }

    //---

private:
    struct Entry : public IReferencable
    {
        ElementWeakPtr owner;
        TEventFunction func;
    };

    typedef RefPtr<Entry> EntryPtr;
    typedef RefWeakPtr<Entry> EntryWeakPtr;

    HashMap<StringID, Array<EntryPtr> > m_entries;
};

///---
    
END_BOOMER_NAMESPACE_EX(ui)
