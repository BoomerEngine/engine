/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "monoBindings.h"

BEGIN_BOOMER_NAMESPACE()

//---

template<typename T>
struct MonoFunctionShimParam
{
};

template<typename T>
struct MonoFunctionShimParam<const T&>
{
    using PT = T*;

    INLINE static const T& val(PT p)
    {
        return *p;
    }
};

template<typename T>
struct MonoFunctionShimParam<T&>
{
    using PT = T*;

    INLINE static T& val(PT p)
    {
        return *p;
    }
};

template<>
struct MonoFunctionShimParam<StringBuf>
{
    using PT = void*;

    INLINE static StringBuf val(PT p)
    {
        return MonoStringToNative(p);
    }
};

template<>
struct MonoFunctionShimParam<const StringBuf&>
{
    using PT = void*;

    INLINE static StringBuf val(PT p)
    {
        return MonoStringToNative(p);
    }
};

template<>
struct MonoFunctionShimParam<StringID>
{
    using PT = void*;

    INLINE static StringID val(PT p)
    {
        return MonoStringIDToNative(p);
    }
};

template<>
struct MonoFunctionShimParam<const StringID&>
{
    using PT = void*;

    INLINE static StringID val(PT p)
    {
        return MonoStringIDToNative(p);
    }
};

template<>
struct MonoFunctionShimParam<MonoStringRef>
{
    using PT = void**;

    INLINE static MonoStringRef val(PT p)
    {
        return MonoStringRef(p);
    }
};

template<typename T>
struct MonoFunctionShimParam<MonoArrayDataT<T>>
{
    using PT = void*;

    INLINE static MonoArrayDataT<T> val(PT p)
    {
        return MonoArrayDataT<T>(p);
    }
};

template<typename T>
struct MonoFunctionShimParam<MonoArrayObjectPtrT<T>>
{
    using PT = void*;

    INLINE static MonoArrayObjectPtrT<T> val(PT p)
    {
        return MonoArrayObjectPtrT<T>(p);
    }
};

template<typename T>
struct MonoFunctionShimParam<MonoArrayRefDataT<T>>
{
    using PT = void**;

    INLINE static MonoArrayRefDataT<T> val(PT p)
    {
        return MonoArrayRefDataT<T>(p);
    }
};

template<typename T>
struct MonoFunctionShimParam<MonoArrayRefObjectPtrT<T>>
{
    using PT = void**;

    INLINE static MonoArrayRefObjectPtrT<T> val(PT p)
    {
        return MonoArrayRefObjectPtrT<T>(p);
    }
};

//--

template<typename T>
struct MonoFunctionShimReturnValue
{
    using PT = T;
    INLINE static PT val(T p) { return p; }
};

template<>
struct MonoFunctionShimReturnValue<StringBuf>
{
    using PT = void*;

    INLINE static PT val(StringBuf p)
    {
        return MonoStringFromNative(p.view());
    }
};

//--

#define BOOMER_MONO_SHIM_VALUE_TYPE(type_) \
template<> struct MonoFunctionShimParam<type_> { \
using PT = type_; \
INLINE static type_ val(PT p) { return p; } \
}; \
template<> struct MonoFunctionShimParam<const type_&> { \
using PT = type_; \
INLINE static type_ val(PT p) { return p; } \
};

#define BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(type_) \
template<> struct MonoFunctionShimParam<type_> { \
using PT = type_*; \
INLINE static type_ val(PT p) { return *p; } \
};

BOOMER_MONO_SHIM_VALUE_TYPE(char)
BOOMER_MONO_SHIM_VALUE_TYPE(short)
BOOMER_MONO_SHIM_VALUE_TYPE(int)
BOOMER_MONO_SHIM_VALUE_TYPE(int64_t)
BOOMER_MONO_SHIM_VALUE_TYPE(uint8_t)
BOOMER_MONO_SHIM_VALUE_TYPE(uint16_t)
BOOMER_MONO_SHIM_VALUE_TYPE(uint32_t)
BOOMER_MONO_SHIM_VALUE_TYPE(uint64_t)
BOOMER_MONO_SHIM_VALUE_TYPE(float)
BOOMER_MONO_SHIM_VALUE_TYPE(double)
BOOMER_MONO_SHIM_VALUE_TYPE(bool)

struct MonoFunctionUnboxPointer
{
    template < typename T, typename std::enable_if < std::is_base_of<IObject, T>::value, int >::type = 0 >
    INLINE static T* UnboxContext(void* contextPtr)
    {
        return rtti_cast<T>(MonoObjectToNativePointerRaw(contextPtr));
    }

    template < typename T, typename std::enable_if < !std::is_base_of<IObject, T>::value, int >::type = 0>
    INLINE static T* UnboxContext(void* contextPtr)
    {
        return (T*)contextPtr;
    }
};

template< typename _class, typename FuncType, FuncType funcPtr >
class MonoFunctionShimBuilderClass
{
public:
    //--

    static INLINE TFunctionMonoWrapperPtr buildShim(void(_class::* func)())
    {
        auto func2 = [](void* contextPtr)
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            (ptr->*funcPtr)();
        };

        typedef void (*TBasic)(void*);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename Ret >
    static INLINE TFunctionMonoWrapperPtr buildShim(Ret(_class::* func)())
    {
        using R = typename MonoFunctionShimReturnValue<typename std::remove_cv<typename std::remove_reference<Ret>::type>::type>;

        auto func2 = [](void* contextPtr) -> typename R::PT
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            return R::val((ptr->*funcPtr)());
        };

        typedef typename R::PT(*TBasic)(void*);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    static INLINE TFunctionMonoWrapperPtr buildShim(void(_class::* func)() const)
    {
        auto func2 = [](void* contextPtr)
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            (ptr->*funcPtr)();
        };

        typedef void (*TBasic)(void*);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename Ret >
    static INLINE TFunctionMonoWrapperPtr buildShim(Ret(_class::* func)() const)
    {
        using R = typename MonoFunctionShimReturnValue<typename std::remove_cv<typename std::remove_reference<Ret>::type>::type>;

        auto func2 = [](void* contextPtr) -> typename R::PT
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            return R::val((ptr->*funcPtr)());
        };

        typedef typename R::PT(*TBasic)(void*);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    //--

    template< typename F1 >
    static INLINE TFunctionMonoWrapperPtr buildShim(void(_class::* func)(F1))
    {
        using P1 = typename MonoFunctionShimParam<F1>;

        auto func2 = [](void* contextPtr, typename P1::PT p1)
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            (ptr->*funcPtr)(P1::val(p1));
        };

        typedef void (*TBasic)(void*, typename P1::PT);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename Ret, typename F1 >
    static INLINE TFunctionMonoWrapperPtr buildShim(Ret(_class::* func)(F1))
    {
        using R = typename MonoFunctionShimReturnValue<typename std::remove_cv<typename std::remove_reference<Ret>::type>::type>;
        using P1 = typename MonoFunctionShimParam<F1>;

        auto func2 = [](void* contextPtr, typename P1::PT p1) -> typename R::PT
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            return R::val((ptr->*funcPtr)(P1::val(p1)));
        };

        typedef typename R::PT(*TBasic)(void*, typename P1::PT);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename F1 >
    static INLINE TFunctionMonoWrapperPtr buildShim(void(_class::* func)(F1) const)
    {
        using P1 = typename MonoFunctionShimParam<F1>;

        auto func2 = [](void* contextPtr, typename P1::PT p1)
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            (ptr->*funcPtr)(P1::val(p1));
        };

        typedef void (*TBasic)(void*, typename P1::PT);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename Ret, typename F1 >
    static INLINE TFunctionMonoWrapperPtr buildShim(Ret(_class::* func)(F1) const)
    {
        using R = typename MonoFunctionShimReturnValue<typename std::remove_cv<typename std::remove_reference<Ret>::type>::type>;
        using P1 = typename MonoFunctionShimParam<F1>;

        auto func2 = [](void* contextPtr, typename P1::PT p1) -> typename R::PT
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            return R::val((ptr->*funcPtr)(P1::val(p1)));
        };

        typedef typename R::PT(*TBasic)(void*, typename P1::PT);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    //--

    template< typename F1, typename F2 >
    static INLINE TFunctionMonoWrapperPtr buildShim(void(_class::* func)(F1, F2))
    {
        using P1 = typename MonoFunctionShimParam<F1>;
        using P2 = typename MonoFunctionShimParam<F2>;

        auto func2 = [](void* contextPtr, typename P1::PT p1, typename P2::PT p2)
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            (ptr->*funcPtr)(P1::val(p1), P2::val(p2));
        };

        typedef void (*TBasic)(void*, typename P1::PT, typename P2::PT);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename Ret, typename F1, typename F2 >
    static INLINE TFunctionMonoWrapperPtr buildShim(Ret(_class::* func)(F1, F2))
    {
        using R = typename MonoFunctionShimReturnValue<typename std::remove_cv<typename std::remove_reference<Ret>::type>::type>;
        using P1 = typename MonoFunctionShimParam<F1>;
        using P2 = typename MonoFunctionShimParam<F2>;

        auto func2 = [](void* contextPtr, typename P1::PT p1, typename P2::PT p2) -> typename R::PT
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            return R::val((ptr->*funcPtr)(P1::val(p1), P2::val(p2)));
        };

        typedef typename R::PT(*TBasic)(void*, typename P1::PT, typename P2::PT);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename F1, typename F2 >
    static INLINE TFunctionMonoWrapperPtr buildShim(void(_class::* func)(F1, F2) const)
    {
        using P1 = typename MonoFunctionShimParam<F1>;
        using P2 = typename MonoFunctionShimParam<F2>;

        auto func2 = [](void* contextPtr, typename P1::PT p1, typename P2::PT p2)
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            (ptr->*funcPtr)(P1::val(p1), P2::val(p2));
        };

        typedef void (*TBasic)(void*, typename P1::PT, typename P2::PT);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename Ret, typename F1, typename F2 >
    static INLINE TFunctionMonoWrapperPtr buildShim(Ret(_class::* func)(F1, F2) const)
    {
        using R = typename MonoFunctionShimReturnValue<typename std::remove_cv<typename std::remove_reference<Ret>::type>::type>;
        using P1 = typename MonoFunctionShimParam<F1>;
        using P2 = typename MonoFunctionShimParam<F2>;

        auto func2 = [](void* contextPtr, typename P1::PT p1, typename P2::PT p2) -> typename R::PT
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            return R::val((ptr->*funcPtr)(P1::val(p1), P2::val(p2)));
        };

        typedef typename R::PT(*TBasic)(void*, typename P1::PT, typename P2::PT);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    //--

    template< typename F1, typename F2, typename F3 >
    static INLINE TFunctionMonoWrapperPtr buildShim(void(_class::* func)(F1, F2, F3))
    {
        using P1 = typename MonoFunctionShimParam<F1>;
        using P2 = typename MonoFunctionShimParam<F2>;
        using P3 = typename MonoFunctionShimParam<F3>;

        auto func2 = [](void* contextPtr, typename P1::PT p1, typename P2::PT p2, typename P2::PT p3)
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            (ptr->*funcPtr)(P1::val(p1), P2::val(p2), P3::val(p3));
        };

        typedef void (*TBasic)(void*, typename P1::PT, typename P2::PT, typename P3::PT);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename Ret, typename F1, typename F2, typename F3 >
    static INLINE TFunctionMonoWrapperPtr buildShim(Ret(_class::* func)(F1, F2, F3))
    {
        using R = typename MonoFunctionShimReturnValue<typename std::remove_cv<typename std::remove_reference<Ret>::type>::type>;
        using P1 = typename MonoFunctionShimParam<F1>;
        using P2 = typename MonoFunctionShimParam<F2>;
        using P3 = typename MonoFunctionShimParam<F3>;

        auto func2 = [](void* contextPtr, typename P1::PT p1, typename P2::PT p2, typename P2::PT p3) -> typename R::PT
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            return R::val((ptr->*funcPtr)(P1::val(p1), P2::val(p2), P3::val(p3)));
        };

        typedef typename R::PT(*TBasic)(void*, typename P1::PT, typename P2::PT, typename P3::PT);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename F1, typename F2, typename F3 >
    static INLINE TFunctionMonoWrapperPtr buildShim(void(_class::* func)(F1, F2, F3) const)
    {
        using P1 = typename MonoFunctionShimParam<F1>;
        using P2 = typename MonoFunctionShimParam<F2>;
        using P3 = typename MonoFunctionShimParam<F3>;

        auto func2 = [](void* contextPtr, typename P1::PT p1, typename P2::PT p2, typename P3::PT p3)
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            (ptr->*funcPtr)(P1::val(p1), P2::val(p2), P3::val(p3));
        };

        typedef void (*TBasic)(void*, typename P1::PT, typename P2::PT, typename P3::PT p3);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    template< typename Ret, typename F1, typename F2, typename F3 >
    static INLINE TFunctionMonoWrapperPtr buildShim(Ret(_class::* func)(F1, F2, F3) const)
    {
        using R = typename MonoFunctionShimReturnValue<typename std::remove_cv<typename std::remove_reference<Ret>::type>::type>;
        using P1 = typename MonoFunctionShimParam<F1>;
        using P2 = typename MonoFunctionShimParam<F2>;
        using P3 = typename MonoFunctionShimParam<F3>;

        auto func2 = [](void* contextPtr, typename P1::PT p1, typename P2::PT p2, typename P3::PT p3) -> typename R::PT
        {
            auto ptr = MonoFunctionUnboxPointer::UnboxContext<_class>(contextPtr);
            return R::val((ptr->*funcPtr)(P1::val(p1), P2::val(p2), P3::val(p3)));
        };

        typedef typename R::PT(*TBasic)(void*, typename P1::PT, typename P2::PT, typename P3::PT p3);
        return (TFunctionMonoWrapperPtr)(TBasic)func2;
    }

    //--
};

//---

// Define a type's object function (requires pointer to "this" to work)
#define RTTI_MONO_CLASS_FUNCTION_EX(_name, _func) {\
    auto sig = FunctionSignatureBuilderClass<TType>::capture(&TType::_func); \
    auto shim = MonoFunctionShimBuilderClass<TType, decltype(&TType::_func), &TType::_func>::buildShim(&TType::_func); \
    builder.addMonoFunction(_name, sig, shim); }

#define RTTI_MONO_CLASS_FUNCTION(_func) \
    RTTI_MONO_CLASS_FUNCTION_EX(#_func, _func)

//---

END_BOOMER_NAMESPACE()
