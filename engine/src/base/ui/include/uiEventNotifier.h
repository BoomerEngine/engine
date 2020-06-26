/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#include "base/containers/include/mutableArray.h"

namespace ui
{
    //---

    /// aggregate of registered event notification functions
    template< typename FuncType >
    class EventNotifierBase : public base::NoCopy
    {
    public:
        typedef FuncType TFunctionType;

        INLINE ~EventNotifierBase()
        {
            for (const auto* entry : m_entries)
            {
                auto token = entry->m_token.lock();
                if (token)
                    token->unbind();
                MemDelete(entry);
            }
            m_entries.clear();
        }

        /// register a notifier
        INLINE EventTokenPtr operator+=(const FuncType& func)
        {
            // create entry
            auto* entry = MemNew(Entry);
            entry->m_func = func;

            // create unregister function
            auto unregisterFunc = [this, entry]()
            {
                removeToken(entry);
            };

            // create token
            auto token = base::CreateSharedPtr<EventToken>(unregisterFunc);
            entry->m_token = token;

            // add entry to map
            m_entries.pushBack(entry);

            // return the token, as long as it's alive the callback will be registered
            return token;
        }

    protected:
        struct Entry
        {
            FuncType m_func;
            base::RefWeakPtr<EventToken> m_token;
        };

        base::MutableArray<Entry*> m_entries;

        INLINE void removeToken(Entry* entry)
        {
            m_entries.remove(entry);
            MemDelete(entry);
        }
    };

    //---

    /// aggregate of registered event notification functions
    class EventNotifier0 : public EventNotifierBase< std::function<void()> >
    {
    public:
        // call event on all registered notifers
        INLINE void notify()
        {
            for (const auto* entry : this->m_entries)
                entry->m_func();
        }
    };

    //---

    /// aggregate of registered event notification functions
    template< typename Arg1 >
    class EventNotifier1 : public EventNotifierBase< std::function<void(const Arg1& arg1)> >
    {
    public:
        // call event on all registered notifers
        INLINE void notify(const Arg1& arg1)
        {
            for (const auto* entry : this->m_entries)
                entry->m_func(arg1);
        }
    };

    //---

    /// aggregate of registered event notification functions
    template< typename Arg1, typename Arg2  >
    class EventNotifier2 : public EventNotifierBase< std::function<void(const Arg1& arg1, const Arg2& arg2)> >
    {
    public:
        // call event on all registered notifers
        INLINE void notify(const Arg1& arg1, const Arg2& arg2)
        {
            for (const auto* entry : this->m_entries)
                entry->m_func(arg1, arg2);
        }
    };

    //---

    /// aggregate of registered event notification functions
    template< typename Arg1, typename Arg2, typename Arg3 >
    class EventNotifier3 : public EventNotifierBase< std::function<void(const Arg1& arg1, const Arg2& arg2, const Arg2& arg3)> >
    {
    public:
        // call event on all registered notifers
        INLINE void notify(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
        {
            for (const auto* entry : this->m_entries)
                entry->m_func(arg1, arg2, arg3);
        }
    };

    //---

} // ui
