/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#define UI_EVENT_FUNC StringID name, IElement* source, IElement* owner, const Variant& data

BEGIN_BOOMER_NAMESPACE_EX(ui)

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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            func(rtti_cast<T>(source));
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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            func(rtti_cast<T>(source));
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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            func(rtti_cast<T>(source));
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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            return func(rtti_cast<T>(source));
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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            return func(rtti_cast<T>(source));
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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            return func(rtti_cast<T>(source));
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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            func(rtti_cast<T>(source), *(const D*)data.data());
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
            static_assert(std::is_pointer<D>::value, "Data can't be a pointer");
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            func(rtti_cast<T>(source), *(const D*)data.data());
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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            func(rtti_cast<T>(source), *(const D*)data.data());
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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            return func(rtti_cast<T>(source), *(const D*)data.data());
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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            return func(rtti_cast<T>(source), *(const D*)data.data());
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
            static_assert(std::is_base_of<IElement, T>::value, "Context object must be UI element");
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            return func(rtti_cast<T>(source), *(const D*)data.data());
        };
    }
};

///---

template<typename D>
struct EventFunctionBinderHelper<void(D)>
{
    using type = std::function<void(D)>;
    static TEventFunction bind(typename type func)
    {
        return [func](UI_EVENT_FUNC)
        {
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            func(*(const D*)data.data());
            return true;
        };
    }
};

template<typename D>
struct EventFunctionBinderHelper<void(*)(D)>
{
    using type = std::function<void(D)>;
    static TEventFunction bind(typename type func)
    {
        return [func](UI_EVENT_FUNC)
        {
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            func(*(const D*)data.data());
            return true;
        };
    }
};

template<typename Class, typename D>
struct EventFunctionBinderHelper<void(Class::*)(D) const>
{
    using type = std::function<void(D)>;
    static TEventFunction bind(typename type func)
    {
        return [func](UI_EVENT_FUNC)
        {
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            func(*(const D*)data.data());
            return true;
        };
    }
};

///---

template<typename D>
struct EventFunctionBinderHelper<bool(D)>
{
    using type = std::function<bool(D)>;
    static TEventFunction bind(typename type func)
    {
        return [func](UI_EVENT_FUNC)
        {
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            return func(*(const D*)data.data());
        };
    }
};

template<typename D>
struct EventFunctionBinderHelper<bool(*)(D)>
{
    using type = std::function<bool(D)>;
    static TEventFunction bind(typename type func)
    {
        return [func](UI_EVENT_FUNC)
        {
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            return func(*(const D*)data.data());
        };
    }
};

template<typename Class, typename D>
struct EventFunctionBinderHelper<bool(Class::*)(D) const>
{
    using type = std::function<bool(D)>;
    static TEventFunction bind(typename type func)
    {
        return [func](UI_EVENT_FUNC)
        {
            static_assert(!std::is_pointer<D>::value, "Data can't be a pointer");
            DEBUG_CHECK_EX(data.type() == reflection::GetTypeObject<D>(), TempString("Invalid event data type, expected '{}', got '{}'", reflection::GetTypeName<D>(), data.type()));
            return func(*(const D*)data.data());
        };
    }
};

///---

/// magical helper for binding event callback functions
struct ENGINE_UI_API EventFunctionBinder
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

    INLINE void operator=(StringID actionName)
    {
        if (m_function)
            *m_function = [actionName](UI_EVENT_FUNC) { return RunAction(owner, source, actionName); };
    }

        
private:
    TEventFunction* m_function;

    static bool RunAction(IElement* element, IElement* source, StringID action);
};

///---
    
END_BOOMER_NAMESPACE_EX(ui)
