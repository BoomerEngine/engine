/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#pragma once

#include <atomic>

namespace base
{
    // we don't wrap std::atomic, it's not a "wrAPI" engine, we just add some functions that are useful

    // increment by one, returns NEW (incremented) value
    template< typename T >
    ALWAYS_INLINE T AtomicIncrement(std::atomic<T>& a)
    {
        return ++a;
    }

    // decrement by one, returns NEW (decremented) value
    template< typename T >
    ALWAYS_INLINE T AtomicDecrement(std::atomic<T>& a)
    {
        return --a;
    }

    // get (atomically) maximum value between current atomic state and incoming one, used for stats tracking
    template< typename T >
    ALWAYS_INLINE void AtomicMin(std::atomic<T>& a, T other)
    {
        T current = a.load();
        if (!a.compare_exchange_strong(current, std::min<T>(current, other)))
            current = a.load();
    }

    // get (atomically) maximum value between current atomic state and incoming one, used for stats tracking
    template< typename T >
    ALWAYS_INLINE void AtomicMax(std::atomic<T>& a, T other)
    {
        T current = a.load();
        if (!a.compare_exchange_strong(current, std::max<T>(current, other)))
            current = a.load();
    }

} // base