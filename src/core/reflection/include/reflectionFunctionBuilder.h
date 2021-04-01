/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#pragma once

#include "reflectionTypeName.h"
#include "reflectionPropertyBuilder.h"

#include "core/object/include/rttiFunction.h"
#include "core/object/include/rttiFunctionPointer.h"

BEGIN_BOOMER_NAMESPACE()

//---

// helper class that can build a function signature
class CORE_REFLECTION_API FunctionSignatureBuilder : public NoCopy
{
public:
    template< typename T >
    INLINE static FunctionParamType MakeType()
    {
        typedef typename std::remove_reference< typename std::remove_pointer<T>::type >::type InnerT;
        typedef typename std::remove_cv<InnerT>::type SafeT;

        auto retType  = GetTypeObject<SafeT>();
        ASSERT_EX(retType != nullptr, "Return type not found in RTTI, only registered types can be used");

        FunctionParamFlags flags = FunctionParamFlag::Normal;
        if (std::is_const<InnerT>::value)
            flags |= FunctionParamFlag::Const;
        if (std::is_reference<T>::value)
            flags |= FunctionParamFlag::Ref;
        if (std::is_pointer<T>::value)
            flags |= FunctionParamFlag::Ptr;

        return FunctionParamType(retType, flags);
    }

    template< typename T >
    INLINE static void returnType(FunctionSignature& outSig)
    {
        outSig.m_returnType = MakeType<T>();
    }

    template< typename T >
    INLINE static void addParamType(FunctionSignature& outSig)
    {
        outSig.m_paramTypes.emplaceBack(MakeType<T>());
    }

    INLINE static void constFlag(FunctionSignature& outSig)
    {
        outSig.m_const = true;
    }

    INLINE static void staticFlag(FunctionSignature& outSig)
    {
        outSig.m_static = true;
    }

private:
    inline FunctionSignatureBuilder() {};
    inline ~FunctionSignatureBuilder() {};
};

//---

// helper class that can build a function call proxy for given class
template< typename _class >
class FunctionSignatureBuilderClass : public FunctionSignatureBuilder
{
public:
    //-- Param Count: 0

    INLINE static FunctionSignature capture(void(_class::* func)())
    {
        return FunctionSignature();
    }

    INLINE static FunctionSignature capture(void(_class::* func)() const)
    {
        FunctionSignature sig;
        constFlag(sig);
        return sig;
    }

    template< typename Ret >
    INLINE static FunctionSignature capture(Ret(_class::*func)())
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        return sig;
    }

    template< typename Ret >
    INLINE static FunctionSignature capture(Ret(_class::* func)() const)
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        constFlag(sig);
        return sig;
    }

    //-- Param Count: 1

    template< typename F1 >
    INLINE static FunctionSignature capture(void(_class::*func)(F1))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        return sig;
    }

    template< typename Ret, typename F1 >
    INLINE static FunctionSignature capture(Ret(_class::*func)(F1))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        return sig;
    }

    template< typename F1 >
    INLINE static FunctionSignature capture(void(_class::* func)(F1) const)
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        constFlag(sig);
        return sig;
    }

    template< typename Ret, typename F1 >
    INLINE static FunctionSignature capture(Ret(_class::* func)(F1) const)
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        constFlag(sig);
        return sig;
    }

    //-- Param Count: 2

    template< typename F1, typename F2 >
    INLINE static FunctionSignature capture(void(_class::*func)(F1, F2))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2 >
    INLINE static FunctionSignature capture(Ret(_class::*func)(F1, F2))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        return sig;
    }

    template< typename F1, typename F2 >
    INLINE static FunctionSignature capture(void(_class::* func)(F1, F2) const)
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        constFlag();
        return sig;
    }

    template< typename Ret, typename F1, typename F2 >
    INLINE static FunctionSignature capture(Ret(_class::* func)(F1, F2) const)
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        constFlag();
        return sig;
    }

    //-- Param Count: 3

    template< typename F1, typename F2, typename F3 >
    INLINE static FunctionSignature capture(void(_class::*func)(F1, F2, F3))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2, typename F3 >
    INLINE static FunctionSignature capture(Ret(_class::*func)(F1, F2, F3))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        return sig;
    }

    template< typename F1, typename F2, typename F3 >
    INLINE static FunctionSignature capture(void(_class::* func)(F1, F2, F3) const)
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        constFlag(sig);
        return sig;

    }

    template< typename Ret, typename F1, typename F2, typename F3 >
    INLINE static FunctionSignature capture(Ret(_class::* func)(F1, F2, F3) const)
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        constFlag(sig);
        return sig;
    }

    //-- Param Count: 4

    template< typename F1, typename F2, typename F3, typename F4 >
    INLINE static FunctionSignature capture(void(_class::*func)(F1, F2, F3, F4))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4 >
    INLINE static FunctionSignature capture(Ret(_class::*func)(F1, F2, F3, F4))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        return sig;
    }

    template< typename F1, typename F2, typename F3, typename F4 >
    INLINE static FunctionSignature capture(void(_class::* func)(F1, F2, F3, F4) const)
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        constFlag(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4 >
    INLINE static FunctionSignature capture(Ret(_class::* func)(F1, F2, F3, F4) const)
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        constFlag(sig);
        return sig;
    }

    //-- Param Count: 5

    template< typename F1, typename F2, typename F3, typename F4, typename F5 >
    INLINE static FunctionSignature capture(void(_class::*func)(F1, F2, F3, F4, F5))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        addParamType<F5>(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5 >
    INLINE static FunctionSignature capture(Ret(_class::*func)(F1, F2, F3, F4, F5))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        addParamType<F5>(sig);
        return sig;
    }

    template< typename F1, typename F2, typename F3, typename F4, typename F5 >
    INLINE static FunctionSignature capture(void(_class::* func)(F1, F2, F3, F4, F5) const)
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        addParamType<F5>(sig);
        constFlag(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5 >
    INLINE static FunctionSignature capture(Ret(_class::* func)(F1, F2, F3, F4, F5) const)
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        addParamType<F5>(sig);
        constFlag(sig);
        return sig;
    }

private:
    FunctionSignatureBuilder* m_builder;
};

//---

// helper class that can build a function call proxy for static function
class FunctionSignatureBuilderStatic : public FunctionSignatureBuilder
{
public:
    //-- Param Count: 0

    INLINE static FunctionSignature capture(void(*func)())
    {
        FunctionSignature sig;
        staticFlag(sig);
        return sig;
    }

    template< typename Ret >
    INLINE static FunctionSignature capture(Ret(*func)())
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        staticFlag(sig);
        return sig;
    }

    //-- Param Count: 1

    template< typename F1 >
    INLINE static FunctionSignature capture(void(*func)(F1))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        staticFlag(sig);
        return sig;
    }

    template< typename Ret, typename F1 >
    INLINE static FunctionSignature capture(Ret(*func)(F1))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        staticFlag(sig);
        return sig;
    }

    //-- Param Count: 2

    template< typename F1, typename F2 >
    INLINE static FunctionSignature capture(void(*func)(F1, F2))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        staticFlag(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2 >
    INLINE static FunctionSignature capture(Ret(*func)(F1, F2))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        staticFlag(sig);
        return sig;
    }

    //-- Param Count: 3

    template< typename F1, typename F2, typename F3 >
    INLINE static FunctionSignature capture(void(*func)(F1, F2, F3))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        staticFlag(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2, typename F3 >
    INLINE static FunctionSignature capture(Ret(*func)(F1, F2, F3))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        staticFlag(sig);
        return sig;
    }

    //-- Param Count: 4

    template< typename F1, typename F2, typename F3, typename F4 >
    INLINE static FunctionSignature capture(void(*func)(F1, F2, F3, F4))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        staticFlag(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4 >
    INLINE static FunctionSignature capture(Ret(*func)(F1, F2, F3, F4))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        staticFlag(sig);
        return sig;
    }

    //-- Param Count: 5

    template< typename F1, typename F2, typename F3, typename F4, typename F5 >
    INLINE static FunctionSignature capture(void(*func)(F1, F2, F3, F4, F5))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        addParamType<F5>(sig);
        staticFlag(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5 >
    INLINE static FunctionSignature capture(Ret(*func)(F1, F2, F3, F4, F5))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        addParamType<F5>(sig);
        staticFlag(sig);
        return sig;
    }

    //-- Param Count: 6

    template< typename F1, typename F2, typename F3, typename F4, typename F5, typename F6 >
    INLINE static FunctionSignature capture(void(*func)(F1, F2, F3, F4, F5, F6))
    {
        FunctionSignature sig;
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        addParamType<F5>(sig);
        addParamType<F6>(sig);
        staticFlag(sig);
        return sig;
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6 >
    INLINE static FunctionSignature capture(Ret(*func)(F1, F2, F3, F4, F5, F6))
    {
        FunctionSignature sig;
        returnType<Ret>(sig);
        addParamType<F1>(sig);
        addParamType<F2>(sig);
        addParamType<F3>(sig);
        addParamType<F4>(sig);
        addParamType<F5>(sig);
        addParamType<F6>(sig);
        staticFlag(sig);
        return sig;
    }
};

//--

template< typename _class, typename FuncType, FuncType funcPtr >
class NativeFunctionShimBuilderClass : public NoCopy
{
public:
    //-- Param Count: 0

    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)())
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            (classPtr->*funcPtr)();
        };
    }

    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)() const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            (classPtr->*funcPtr)();
        };
    }

    template <typename Ret>
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)())
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)());
        };
    }

    template <typename Ret>
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)() const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)());
        };
    }

    //-- Param Count: 1

    template< typename F1 >
    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)(F1))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            (classPtr->*funcPtr)(params.arg<F1, 0>());
        };
    }

    template< typename Ret, typename F1 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)(F1))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)(params.arg<F1, 0>()));
        };
    }

    template< typename F1 >
    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)(F1) const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            (classPtr->*funcPtr)(params.arg<F1, 0>());
        };
    }

    template< typename Ret, typename F1 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)(F1) const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)(params.arg<F1, 0>()));
        };
    }

    //-- Param Count: 2

    template< typename F1, typename F2 >
    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)(F1, F2))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {            
            auto classPtr = (_class*)contextPtr;
            (classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>());
        };
    }

    template< typename Ret, typename F1, typename F2 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)(F1, F2))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>()));
        };
    }

    template< typename F1, typename F2 >
    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)(F1, F2) const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            (classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>());
        };
    }

    template< typename Ret, typename F1, typename F2 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)(F1, F2) const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>()));
        };
    }

    //-- Param Count: 3

    template< typename F1, typename F2, typename F3 >
    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)(F1, F2, F3))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            (classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>());
        };
    }

    template< typename Ret, typename F1, typename F2, typename F3 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)(F1, F2, F3))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>()));
        };
    }

    template< typename F1, typename F2, typename F3 >
    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)(F1, F2, F3) const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            (classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>());
        };
    }

    template< typename Ret, typename F1, typename F2, typename F3 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)(F1, F2, F3) const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>()));
        };
    }

    //-- Param Count: 4

    template< typename F1, typename F2, typename F3, typename F4 >
    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)(F1, F2, F3, F4))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            (classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>());
        };
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)(F1, F2, F3, F4))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>()));
        };
    }

    template< typename F1, typename F2, typename F3, typename F4 >
    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)(F1, F2, F3, F4) const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            (classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>());
        };
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)(F1, F2, F3, F4) const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>()));
        };
    }

    //-- Param Count: 5

    template< typename F1, typename F2, typename F3, typename F4, typename F5 >
    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)(F1, F2, F3, F4, F5))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            (classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>(), params.arg<F5, 4>());
        };
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)(F1, F2, F3, F4, F5))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (_class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>(), params.arg<F5, 4>()));
        };
    }

    template< typename F1, typename F2, typename F3, typename F4, typename F5 >
    static INLINE TFunctionWrapperPtr buildShim(void(_class::* func)(F1, F2, F3, F4, F5) const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            (classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>(), params.arg<F5, 4>());
        };
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(_class::* func)(F1, F2, F3, F4, F5) const)
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            auto classPtr = (const _class*)contextPtr;
            params.writeResult<Ret>((classPtr->*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>(), params.arg<F5, 4>()));
        };
    }

    //--

};

//--

template< typename FuncType, FuncType funcPtr >
class NativeFunctionShimBuilderStatic : public NoCopy
{
public:
    static INLINE TFunctionWrapperPtr buildShim(void(*func)())
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            (*funcPtr)();
        };
    }

    template< typename Ret >
    static INLINE TFunctionWrapperPtr buildShim(Ret(*func)())
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            params.writeResult<Ret>((*funcPtr)());
        };
    }

    //-- Param Count: 1

    template< typename F1 >
    static INLINE TFunctionWrapperPtr buildShim(void(*func)(F1))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            (*funcPtr)(params.arg<F1, 0>());
        };
    }

    template< typename Ret, typename F1 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(*func)(F1))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            params.writeResult<Ret>((*funcPtr)(params.arg<F1, 0>()));
        };
    }

    //-- Param Count: 2

    template< typename F1, typename F2 >
    static INLINE TFunctionWrapperPtr buildShim(void(*func)(F1, F2))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            (*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>());
        };
    }

    template< typename Ret, typename F1, typename F2 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(*func)(F1, F2))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            params.writeResult<Ret>((*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>()));
        };
    }

    //-- Param Count: 3

    template< typename F1, typename F2, typename F3 >
    static INLINE TFunctionWrapperPtr buildShim(void(*func)(F1, F2, F3))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            (*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>());
        };
    }

    template< typename Ret, typename F1, typename F2, typename F3 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(*func)(F1, F2, F3))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            params.writeResult<Ret>((*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>()));
        };
    }

    //-- Param Count: 4

    template< typename F1, typename F2, typename F3, typename F4 >
    static INLINE TFunctionWrapperPtr buildShim(void(*func)(F1, F2, F3, F4))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            (*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>());
        };
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(*func)(F1, F2, F3, F4))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {   
            params.writeResult<Ret>((*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>()));
        };
    }

    //-- Param Count: 5

    template< typename F1, typename F2, typename F3, typename F4, typename F5 >
    static INLINE TFunctionWrapperPtr buildShim(void(*func)(F1, F2, F3, F4, F5))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            (*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>(), params.arg<F5, 4>());
        };
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(*func)(F1, F2, F3, F4, F5))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            params.writeResult<Ret>((*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>(), params.arg<F5, 4>()));
        };
    }

    //-- Param Count: 6

    template< typename F1, typename F2, typename F3, typename F4, typename F5, typename F6 >
    static INLINE TFunctionWrapperPtr buildShim(void(*func)(F1, F2, F3, F4, F5, F6))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {
            (*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>(), params.arg<F5, 4>(), params.arg<F6, 5>());
        };
    }

    template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6 >
    static INLINE TFunctionWrapperPtr buildShim(Ret(*func)(F1, F2, F3, F4, F5, F6))
    {
        return [](void* contextPtr, const FunctionCallingParams& params)
        {   
            params.writeResult<Ret>((*funcPtr)(params.arg<F1, 0>(), params.arg<F2, 1>(), params.arg<F3, 2>(), params.arg<F4, 3>(), params.arg<F5, 4>(), params.arg<F6, 5>()));
        };
    }

};

//--

END_BOOMER_NAMESPACE()
