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

#include "base/object/include/rttiFunction.h"
#include "base/object/include/rttiFunctionPointer.h"

namespace base
{
    namespace reflection
    {

        //---

        // helper class that can build a function
        class BASE_REFLECTION_API FunctionBuilder
        {
        public:
            FunctionBuilder(const char* name);
            ~FunctionBuilder();

            void submit(rtti::IClassType* targetClass);

            template< typename T >
            INLINE static rtti::FunctionParamType MakeType()
            {
                typedef typename std::remove_reference< typename std::remove_pointer<T>::type >::type InnerT;
                typedef typename std::remove_cv<InnerT>::type SafeT;

                auto retType  = reflection::GetTypeObject<SafeT>();
                ASSERT_EX(retType != nullptr, "Return type not found in RTTI, only registered types can be used");

                rtti::FunctionParamFlags flags = rtti::FunctionParamFlag::Normal;
                if (std::is_const<InnerT>::value)
                    flags |= rtti::FunctionParamFlag::Const;
                if (std::is_reference<T>::value)
                    flags |= rtti::FunctionParamFlag::Ref;
                if (std::is_pointer<T>::value)
                    flags |= rtti::FunctionParamFlag::Ptr;

                return rtti::FunctionParamType(retType, flags);
            }

            template< typename T >
            INLINE void returnType()
            {
                m_returnType = MakeType<T>();
            }

            template< typename T >
            INLINE void addParamType()
            {
                m_paramTypes.pushBack(MakeType<T>());
            }

            INLINE void functionPtr(const rtti::FunctionPointer& ptr)
            {
                m_functionPtr = ptr;
            }

            INLINE void functionWrapper(rtti::TFunctionWrapperPtr ptr)
            {
                m_functionWrapperPtr = ptr;
            }

            INLINE void constFlag()
            {
                m_isConst = true;
            }

            INLINE void staticFlag()
            {
                m_isStatic = true;
            }

        protected:
            StringBuf m_name; // name of the function

            rtti::FunctionPointer m_functionPtr; // point to the function
            rtti::TFunctionWrapperPtr m_functionWrapperPtr; // point to the function

            rtti::FunctionParamType m_returnType; // type returned by the function

            Array<rtti::FunctionParamType> m_paramTypes; // type of parameters accepted by function

            bool m_isConst;
            bool m_isStatic;
        };

        //---

        // helper class that can build a function call proxy for given class
        template< typename _class >
        class FunctionBuilderClass
        {
        public:
            INLINE FunctionBuilderClass(FunctionBuilder& builder)
                : m_builder(&builder)
            {}

            //-- Param Count: 0

            INLINE void setupProxy(void(_class::*func)(), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc  = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;
                    (classPtr->*callableFunc)();
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->functionPtr(funcPtr);
            }

            template< typename Ret >
            INLINE void setupProxy(Ret(_class::*func)(), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc  = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)());
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->functionPtr(funcPtr);
            }

            //-- Param Count: 1

            template< typename F1 >
            INLINE void setupProxy(void(_class::*func)(F1), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc  = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)( params.arg<F1,0>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->functionPtr(funcPtr);
            }

            template< typename Ret, typename F1 >
            INLINE void setupProxy(Ret(_class::*func)(F1), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc  = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)( params.arg<F1,0>() ));
                };

                m_builder->functionWrapper(wrapperPtr);

                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->functionPtr(funcPtr);
            }

            //-- Param Count: 2

            template< typename F1, typename F2 >
            INLINE void setupProxy(void(_class::*func)(F1, F2), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc  = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->functionPtr(funcPtr);
            }

            template< typename Ret, typename F1, typename F2 >
            INLINE void setupProxy(Ret(_class::*func)(F1, F2), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc  = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->functionPtr(funcPtr);
            }

            //-- Param Count: 3

            template< typename F1, typename F2, typename F3 >
            INLINE void setupProxy(void(_class::*func)(F1, F2, F3), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc  = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->functionPtr(funcPtr);
            }

            template< typename Ret, typename F1, typename F2, typename F3 >
            INLINE void setupProxy(Ret(_class::*func)(F1, F2, F3), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc  = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->functionPtr(funcPtr);
            }

            //-- Param Count: 4

            template< typename F1, typename F2, typename F3, typename F4 >
            INLINE void setupProxy(void(_class::*func)(F1, F2, F3, F4), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->functionPtr(funcPtr);
            }

            template< typename Ret, typename F1, typename F2, typename F3, typename F4 >
            INLINE void setupProxy(Ret(_class::*func)(F1, F2, F3, F4), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->functionPtr(funcPtr);
            }

            //-- Param Count: 5

            template< typename F1, typename F2, typename F3, typename F4, typename F5 >
            INLINE void setupProxy(void(_class::*func)(F1, F2, F3, F4, F5), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>(), params.arg<F5,4>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->addParamType<F5>();
                m_builder->functionPtr(funcPtr);
            }

            template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5 >
            INLINE void setupProxy(Ret(_class::*func)(F1, F2, F3, F4, F5), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>(), params.arg<F5,4>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->addParamType<F5>();
                m_builder->functionPtr(funcPtr);
            }

            //---- CONST VERSION

            //-- Param Count: 0

            INLINE void setupProxy(void(_class::*func)() const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)();
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            template< typename Ret >
            INLINE void setupProxy(Ret(_class::*func)() const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)());
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            //-- Param Count: 1

            template< typename F1 >
            INLINE void setupProxy(void(_class::*func)(F1) const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)( params.arg<F1,0>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            template< typename Ret, typename F1 >
            INLINE void setupProxy(Ret(_class::*func)(F1) const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)( params.arg<F1,0>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            //-- Param Count: 2

            template< typename F1, typename F2 >
            INLINE void setupProxy(void(_class::*func)(F1, F2) const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            template< typename Ret, typename F1, typename F2 >
            INLINE void setupProxy(Ret(_class::*func)(F1, F2) const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>()));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            //-- Param Count: 3

            template< typename F1, typename F2, typename F3 >
            INLINE void setupProxy(void(_class::*func)(F1, F2, F3) const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            template< typename Ret, typename F1, typename F2, typename F3 >
            INLINE void setupProxy(Ret(_class::*func)(F1, F2, F3) const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            //-- Param Count: 4

            template< typename F1, typename F2, typename F3, typename F4 >
            INLINE void setupProxy(void(_class::*func)(F1, F2, F3, F4) const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>());
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            template< typename Ret, typename F1, typename F2, typename F3, typename F4 >
            INLINE void setupProxy(Ret(_class::*func)(F1, F2, F3, F4) const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>()) );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            //-- Param Count: 5

            template< typename F1, typename F2, typename F3, typename F4, typename F5 >
            INLINE void setupProxy(void(_class::*func)(F1, F2, F3, F4, F5) const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    (classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>(), params.arg<F5,4>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->addParamType<F5>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

            template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5 >
            INLINE void setupProxy(Ret(_class::*func)(F1, F2, F3, F4, F5) const, const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                   auto callableFunc = *((decltype(func)*)&nativeFunc.ptr.dummy);
                    auto classPtr  = (_class*)contextPtr;

                    params.writeResult<Ret>((classPtr->*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>(), params.arg<F5,4>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->addParamType<F5>();
                m_builder->functionPtr(funcPtr);
                m_builder->constFlag();
            }

        private:
            FunctionBuilder* m_builder;
        };

        //---

        // helper class that can build a function call proxy for static function
        class FunctionBuilderStatic
        {
        public:
            INLINE FunctionBuilderStatic(FunctionBuilder& builder)
                : m_builder(&builder)
            {}

            //-- Param Count: 0

            INLINE void setupProxy(void(*func)(), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    (*callableFunc)();
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            template< typename Ret >
            INLINE void setupProxy(Ret(*func)(), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    params.writeResult<Ret>( (*callableFunc)() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            //-- Param Count: 1

            template< typename F1 >
            INLINE void setupProxy(void(*func)(F1), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    (*callableFunc)( params.arg<F1,0>());
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            template< typename Ret, typename F1 >
            INLINE void setupProxy(Ret(*func)(F1), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    params.writeResult<Ret>( (*callableFunc)( params.arg<F1,0>()));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            //-- Param Count: 2

            template< typename F1, typename F2 >
            INLINE void setupProxy(void(*func)(F1, F2), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    (*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            template< typename Ret, typename F1, typename F2 >
            INLINE void setupProxy(Ret(*func)(F1, F2), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    params.writeResult<Ret>( (*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            //-- Param Count: 3

            template< typename F1, typename F2, typename F3 >
            INLINE void setupProxy(void(*func)(F1, F2, F3), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    (*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            template< typename Ret, typename F1, typename F2, typename F3 >
            INLINE void setupProxy(Ret(*func)(F1, F2, F3), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    params.writeResult<Ret>( (*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            //-- Param Count: 4

            template< typename F1, typename F2, typename F3, typename F4 >
            INLINE void setupProxy(void(*func)(F1, F2, F3, F4), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    (*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>());
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            template< typename Ret, typename F1, typename F2, typename F3, typename F4 >
            INLINE void setupProxy(Ret(*func)(F1, F2, F3, F4), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    params.writeResult<Ret>( (*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>()));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            //-- Param Count: 5

            template< typename F1, typename F2, typename F3, typename F4, typename F5 >
            INLINE void setupProxy(void(*func)(F1, F2, F3, F4, F5), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    (*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>(), params.arg<F5,4>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->addParamType<F5>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5 >
            INLINE void setupProxy(Ret(*func)(F1, F2, F3, F4, F5), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    params.writeResult<Ret>( (*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>(), params.arg<F5,4>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->addParamType<F5>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            //-- Param Count: 6

            template< typename F1, typename F2, typename F3, typename F4, typename F5, typename F6 >
            INLINE void setupProxy(void(*func)(F1, F2, F3, F4, F5, F6), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    (*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>(), params.arg<F5,4>(), params.arg<F6,5>() );
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->addParamType<F5>();
                m_builder->addParamType<F6>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

            template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6 >
            INLINE void setupProxy(Ret(*func)(F1, F2, F3, F4, F5, F6), const rtti::FunctionPointer& funcPtr)
            {
                rtti::TFunctionWrapperPtr wrapperPtr = [](void* contextPtr, const rtti::FunctionPointer& nativeFunc, const rtti::FunctionCallingParams& params)
                {
                    auto callableFunc = ((decltype(func))nativeFunc.ptr.f.func);
                    params.writeResult<Ret>( (*callableFunc)( params.arg<F1,0>(), params.arg<F2,1>(), params.arg<F3,2>(), params.arg<F4,3>(), params.arg<F5,4>(), params.arg<F6,5>() ));
                };

                m_builder->functionWrapper(wrapperPtr);
                m_builder->returnType<Ret>();
                m_builder->addParamType<F1>();
                m_builder->addParamType<F2>();
                m_builder->addParamType<F3>();
                m_builder->addParamType<F4>();
                m_builder->addParamType<F5>();
                m_builder->addParamType<F6>();
                m_builder->functionPtr(funcPtr);
                m_builder->staticFlag();
            }

        private:
            FunctionBuilder* m_builder;
        };

    } // reflection
} // base
