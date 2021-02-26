/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: event #]
***/

#pragma once

#include "globalEventKey.h"

BEGIN_BOOMER_NAMESPACE()

///--

/// listener/dispatcher that is capable of listening for particular event on particular object
class CORE_OBJECT_API IGlobalEventListener : public IReferencable
{
public:
    IGlobalEventListener(GlobalEventKey key, StringID eventName);
    virtual ~IGlobalEventListener();

    //--

    // event key we are listening for
    INLINE GlobalEventKey key() const { return m_key; }

    // event name we are listening for
    INLINE StringID name() const { return m_eventName; }

    //--

    // called when event occurred
    virtual void dispatch(IObject* source, const void* data, Type dataType) const = 0;

    //--

private:
    GlobalEventKey m_key;
    StringID m_eventName;
};

//--

/// register a global event listener 
extern CORE_OBJECT_API void RegisterGlobalEventListener(IGlobalEventListener* listener);

/// unregister a global event listener
extern CORE_OBJECT_API void UnregisterGlobalEventListener(IGlobalEventListener* listener);

/// dispatch global event
extern CORE_OBJECT_API void DispatchGlobalEvent(GlobalEventKey key, StringID eventName, IObject* source = nullptr, const void* data = nullptr, Type dataType = Type());

#ifdef HAS_CORE_REFLECTION
/// dispatch global event
template< typename T >
INLINE void DispatchGlobalEvent(GlobalEventKey key, StringID eventName, const T& data)
{
    static_assert(!std::is_pointer<T>::value, "Cannot dispatch event with a pointer, wrap it in a container");
    DispatchGlobalEvent(key, eventName, nullptr, &data, reflection::GetTypeObject<T>());
}

/// dispatch global event
template< typename T >
INLINE void DispatchGlobalEvent(GlobalEventKey key, StringID eventName, IObject* source, const T& data)
{
    static_assert(!std::is_pointer<T>::value, "Cannot dispatch event with a pointer, wrap it in a container");
    DispatchGlobalEvent(key, eventName, source, &data, reflection::GetTypeObject<T>());
}
#endif

//--

END_BOOMER_NAMESPACE()
