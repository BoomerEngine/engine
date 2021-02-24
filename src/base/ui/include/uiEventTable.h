/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#include "uiEventFunction.h"

BEGIN_BOOMER_NAMESPACE(ui)

///---

/// table of events 
class BASE_UI_API EventTable : public base::NoCopy
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
    void clearByName(base::StringID name);

    //---

    /// bind event
    EventFunctionBinder bind(base::StringID name, IElement* owner);

    /// unbind event
    void unbind(base::StringID name, IElement* owner);

    //---

    // call event with no data
    bool call(base::StringID name, IElement* source);

    // call event with generic data
    bool callGeneric(base::StringID name, IElement* source, const base::Variant& data);

    // call event with specific data
    template< typename T >
    INLINE bool call(base::StringID name, IElement* source, const T& data)
    {
        return callGeneric(name, source, base::CreateVariant(data));
    }

    //---

private:

    struct Entry : public base::IReferencable
    {
        ElementWeakPtr owner;
        TEventFunction func;
    };

    typedef base::RefPtr<Entry> EntryPtr;
    typedef base::RefWeakPtr<Entry> EntryWeakPtr;

    base::HashMap<base::StringID, base::Array<EntryPtr> > m_entries;
};

///---
    
END_BOOMER_NAMESPACE(ui)
