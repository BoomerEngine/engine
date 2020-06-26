/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#define UI_EVENT_FUNC base::StringID name, IElement* source, IElement* owner, const base::Variant& data

namespace ui
{
    ///---

    typedef std::function<bool(UI_EVENT_FUNC)> TEventFunction;

    ///---

    template<typename T>
    struct EventFunctionBinderHelper
    {};

    template<>
    struct EventFunctionBinderHelper<void()>
    {
        using type = std::function<void()>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                func();
                return true;
            };
        }
    };

    template<>
    struct EventFunctionBinderHelper<bool()>
    {
        using type = std::function<bool()>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                return func();
            };
        }
    };

    template<>
    struct EventFunctionBinderHelper<bool(*)()>
    {
        using type = std::function<bool()>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                return func();
            };
        }
    };

    template<typename Class>
    struct EventFunctionBinderHelper<void(Class::*)() const>
    {
        using type = std::function<void()>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                func();
                return true;
            };
        }
    };

    template<typename Class>
    struct EventFunctionBinderHelper<bool(Class::*)() const>
    {
        using type = std::function<bool()>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                return func();
            };
        }
    };

    ///---

    template<typename T>
    struct EventFunctionBinderHelper<void(T*)>
    {
        using type = std::function<void(T*)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                func(base::rtti_cast<T>(owner));
                return true;
            };
        }
    };

    template<typename T>
    struct EventFunctionBinderHelper<void(*)(T*)>
    {
        using type = std::function<void(T*)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                func(base::rtti_cast<T>(owner));
                return true;
            };
        }
    };

    template<typename Class, typename T>
    struct EventFunctionBinderHelper<void(Class::*)(T*) const>
    {
        using type = std::function<void(T*)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                func(base::rtti_cast<T>(owner));
                return true;
            };
        }
    };

    template<typename T>
    struct EventFunctionBinderHelper<bool(T*)>
    {
        using type = std::function<bool(T*)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                return func(base::rtti_cast<T>(owner));
            };
        }
    };

    template<typename T>
    struct EventFunctionBinderHelper<bool(*)(T*)>
    {
        using type = std::function<void(T*)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                return func(base::rtti_cast<T>(owner));
            };
        }
    };

    template<typename Class, typename T>
    struct EventFunctionBinderHelper<bool(Class::*)(T*) const>
    {
        using type = std::function<bool(T*)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                return func(base::rtti_cast<T>(owner));
            };
        }
    };

    ///---

    template<typename T, typename D>
    struct EventFunctionBinderHelper<void(T*, D)>
    {
        using type = std::function<void(T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                func(base::rtti_cast<T>(owner), *(const D*)data.data());
                return true;
            };
        }
    };

    template<typename T, typename D>
    struct EventFunctionBinderHelper<void(*)(T*, D)>
    {
        using type = std::function<void(T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                func(base::rtti_cast<T>(owner), *(const D*)data.data());
                return true;
            };
        }
    };

    template<typename Class, typename T, typename D>
    struct EventFunctionBinderHelper<void(Class::*)(T*, D) const>
    {
        using type = std::function<void(T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                func(base::rtti_cast<T>(owner), *(const D*)data.data());
                return true;
            };
        }
    };

    ///---

    template<typename T, typename D>
    struct EventFunctionBinderHelper<bool(T*, D)>
    {
        using type = std::function<bool(T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                return func(base::rtti_cast<T>(owner), *(const D*)data.data());
            };
        }
    };

    template<typename T, typename D>
    struct EventFunctionBinderHelper<bool(*)(T*, D)>
    {
        using type = std::function<bool(T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                return func(base::rtti_cast<T>(owner), *(const D*)data.data());
            };
        }
    };

    template<typename Class, typename T, typename D>
    struct EventFunctionBinderHelper<bool(Class::*)(T*, D) const>
    {
        using type = std::function<bool(T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                return func(base::rtti_cast<T>(owner), *(const D*)data.data());
            };
        }
    };

    ///---

    template<typename C, typename T, typename D>
    struct EventFunctionBinderHelper<void(C*, T*, D)>
    {
        using type = std::function<void(C*, T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                func(base::rtti_cast<C>(source), base::rtti_cast<T>(owner), *(const D*)data.data());
                return true;
            };
        }
    };

    template<typename C, typename T, typename D>
    struct EventFunctionBinderHelper<void(*)(C*, T*, D)>
    {
        using type = std::function<void(C*, T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                func(base::rtti_cast<C>(source), base::rtti_cast<T>(owner), *(const D*)data.data());
                return true;
            };
        }
    };

    template<typename Class, typename C, typename T, typename D>
    struct EventFunctionBinderHelper<void(Class::*)(C*, T*, D) const>
    {
        using type = std::function<void(C*, T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                func(base::rtti_cast<C>(source), base::rtti_cast<T>(owner), *(const D*)data.data());
                return true;
            };
        }
    };

    ///---

    template<typename C, typename T, typename D>
    struct EventFunctionBinderHelper<bool(C*, T*, D)>
    {
        using type = std::function<bool(C*, T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                return func(base::rtti_cast<C>(source), base::rtti_cast<T>(owner), *(const D*)data.data());
            };
        }
    };

    template<typename C, typename T, typename D>
    struct EventFunctionBinderHelper<bool(*)(C*, T*, D)>
    {
        using type = std::function<bool(C*, T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                return func(base::rtti_cast<C>(source), base::rtti_cast<T>(owner), *(const D*)data.data());
            };
        }
    };

    template<typename Class, typename C, typename T, typename D>
    struct EventFunctionBinderHelper<bool(Class::*)(C*, T*, D) const>
    {
        using type = std::function<bool(C*, T*, D)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                DEBUG_CHECK_EX(data.type() == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), data.type()));
                return func(base::rtti_cast<C>(source), base::rtti_cast<T>(owner), *(const D*)data.data());
            };
        }
    };

    ///---

    template<typename C, typename T>
    struct EventFunctionBinderHelper<void(C*, T*)>
    {
        using type = std::function<void(C*, T*)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                func(base::rtti_cast<C>(source), ase::rtti_cast<T>(owner));
                return true;
            };
        }
    };

    template<typename C, typename T>
    struct EventFunctionBinderHelper<void(*)(C*, T*)>
    {
        using type = std::function<void(C*, T*)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                func(base::rtti_cast<C>(source), ase::rtti_cast<T>(owner));
                return true;
            };
        }
    };

    template<typename Class, typename C, typename T>
    struct EventFunctionBinderHelper<void(Class::*)(C*, T*) const>
    {
        using type = std::function<void(C*, T*)>;
        static TEventFunction bind(typename type func)
        {
            return [func](UI_EVENT_FUNC)
            {
                func(base::rtti_cast<C>(source), base::rtti_cast<T>(owner));
                return true;
            };
        }
    };

    ///---

    /// magical helper for binding event callback functions
    struct BASE_UI_API EventFunctionBinder
    {
    public:
        INLINE EventFunctionBinder(TEventFunction* func) : m_function(func) {}
        INLINE EventFunctionBinder(const EventFunctionBinder& other) = default;
        INLINE EventFunctionBinder& operator=(const EventFunctionBinder& other) = default;

        template< typename F >
        INLINE void operator=(F func)
        {
            if (m_function)
                *m_function = EventFunctionBinderHelper<decltype(&F::operator())>::bind(func);
        }

        INLINE void operator=(base::StringID actionName)
        {
            if (m_function)
                *m_function = [actionName](UI_EVENT_FUNC) { return RunAction(owner, actionName); };
        }

        
    private:
        TEventFunction* m_function;

        static bool RunAction(IElement* element, base::StringID action);
    };

    ///---
    
} // ui
