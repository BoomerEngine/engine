/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: event #]
***/

#pragma once

#define GLOBAL_EVENT_FUNC base::StringID name, base::IObject* source, const void* data, base::Type dataType

BEGIN_BOOMER_NAMESPACE(base)

///---

typedef std::function<void(GLOBAL_EVENT_FUNC)> TGlobalEventFunction;

//--------------
// VOID only

template<typename T>
struct GlobalEventFunctionBinderHelper
{};

template<>
struct GlobalEventFunctionBinderHelper<void()>
{
    using type = std::function<void()>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
            func();
        };
    }
};

template<typename Class>
struct GlobalEventFunctionBinderHelper<void(Class::*)() const>
{
    using type = std::function<void()>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
            func();
        };
    }
};

//--------------
// OBJECT only

template<typename T>
struct GlobalEventFunctionBinderHelper<void(T*)>
{
    using type = std::function<void(T*)>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
            func(base::rtti_cast<T>(source));
            return true;
        };
    }
};

template<typename T>
struct GlobalEventFunctionBinderHelper<void(*)(T*)>
{
    using type = std::function<void(T*)>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
            func(base::rtti_cast<T>(source));
            return true;
        };
    }
};

template<typename Class, typename T>
struct GlobalEventFunctionBinderHelper<void(Class::*)(T*) const>
{
    using type = std::function<void(T*)>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
            func(base::rtti_cast<T>(source));
            return true;
        };
    }
};

//--------------
// DATA only

template<typename D>
struct GlobalEventFunctionBinderHelper<void(D)>
{
    using type = std::function<void(D)>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
#ifdef HAS_BASE_REFLECTION
            DEBUG_CHECK_EX(dataType == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), dataType));
#endif
            func(*(const D*)data);
            return true;
        };
    }
};

template<typename D>
struct GlobalEventFunctionBinderHelper<void(*)(D)>
{
    using type = std::function<void(D)>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
#ifdef HAS_BASE_REFLECTION
            DEBUG_CHECK_EX(dataType == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), dataType));
#endif
            func(*(const D*)data);
            return true;
        };
    }
};

template<typename Class, typename D>
struct GlobalEventFunctionBinderHelper<void(Class::*)(D) const>
{
    using type = std::function<void(D)>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
#ifdef HAS_BASE_REFLECTION
            DEBUG_CHECK_EX(dataType == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), dataType));
#endif
            func(*(const D*)data);
            return true;
        };
    }
};


//--------------
// OBJECT + DATA

template<typename T, typename D>
struct GlobalEventFunctionBinderHelper<void(T*, D)>
{
    using type = std::function<void(T*, D)>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
            DEBUG_CHECK_EX(dataType == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), dataType));
            func(base::rtti_cast<T>(source), *(const D*)data);
            return true;
        };
    }
};

template<typename T, typename D>
struct GlobalEventFunctionBinderHelper<void(*)(T*, D)>
{
    using type = std::function<void(T*, D)>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
#ifdef HAS_BASE_REFLECTION
            DEBUG_CHECK_EX(dataType == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), dataType));
#endif
            func(base::rtti_cast<T>(source), *(const D*)data);
            return true;
        };
    }
};

template<typename Class, typename T, typename D>
struct GlobalEventFunctionBinderHelper<void(Class::*)(T*, D) const>
{
    using type = std::function<void(T*, D)>;
    static TGlobalEventFunction bind(typename type func)
    {
        return [func](GLOBAL_EVENT_FUNC)
        {
#ifdef HAS_BASE_REFLECTION
            DEBUG_CHECK_EX(dataType == base::reflection::GetTypeObject<D>(), base::TempString("Invalid event data type, expected '{}', got '{}'", base::reflection::GetTypeName<D>(), dataType));
#endif
            func(base::rtti_cast<T>(source), *(const D*)data);
            return true;
        };
    }
};

//--

/// magical helper for binding event callback functions
struct BASE_OBJECT_API GlobalEventFunctionBinder
{
public:
    ALWAYS_INLINE GlobalEventFunctionBinder(TGlobalEventFunction* func, const GlobalEventListenerPtr& entry) : m_function(func), m_entry(entry) {}
    ALWAYS_INLINE GlobalEventFunctionBinder(const GlobalEventFunctionBinder& other) = default;
    ALWAYS_INLINE GlobalEventFunctionBinder& operator=(const GlobalEventFunctionBinder& other) = default;

    template< typename F >
    ALWAYS_INLINE void operator=(F func)
    {
        if (m_function)
        {
            *m_function = GlobalEventFunctionBinderHelper<decltype(&F::operator())>::bind(func);
            publish();
        }
    }

private:
    TGlobalEventFunction* m_function = nullptr;
    GlobalEventListenerPtr m_entry;

    void publish();
};

//--

END_BOOMER_NAMESPACE(base)
