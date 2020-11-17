/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\class #]
***/

#pragma once

#include "rttiClassType.h"
#include "rttiCustomType.h"

namespace base
{
    namespace rtti
    {

        //--
        
        /// Native class is tight to a C++ class thus will use it's method for construction/destruction/compare and copy
        /// There are many "flavors" of the native class:
        ///  - one with no constructor/destructor function can't be created directly (it's abstract)
        ///  - one with no copy function will assume member-wise copy only (slow)
        ///  - one with no compare function will assume member-wise compare only (slow)
        /// NOTE: we are NOT a template on the "T", it's not necessary as we can inject functions
        /// NOTE: if class is fully abstract it can't have a proper default object - we create an zeroed buffer with no vtable and initialize each property to it's default
        class BASE_OBJECT_API NativeClass : public IClassType
        {
        public:
            NativeClass(const char* name, uint32_t size, uint32_t alignment, uint64_t nativeHash, PoolTag pool = POOL_OBJECTS);

            typedef void (*TConstructFunc)(void* ptr);
            typedef void (*TDestructFunc)(void* ptr);
            typedef bool (*TCompareFunc)(const void* a, const void* b); // if empty we assume "compare via comparing each member"
            typedef void (*TCopyFunc)(void* dest, const void* src); // if empty we assume "copy via coping each member"

            // "vtable" for class - can be used directly (ie. by scripting engine integration)
            TConstructFunc funcConstruct = nullptr; // usually valid for all but abstract classes
            TDestructFunc funcDestruct = nullptr; // usually valid for all but abstract classes
            TCompareFunc funcComare = nullptr; // usually null for "true objects", not null for some POD-like structures (math, etc)
            TCopyFunc funcCopyAssign = nullptr; // usually null for "true objects" but valid for almost all structures
            TCopyFunc funcCopyConstruct = nullptr; // usually Null for "true objects" but valid for almost all structures

            // IClassType
            virtual bool isAbstract() const override; // true if we don't have construct/destruct function
            virtual const void* defaultObject() const override; // returns default object constructed via createDefaultObject
            virtual const void* createDefaultObject() const override; // always works, even if all we return is zeroed memory
            virtual void destroyDefaultObject() const override;
            virtual void construct(void* object) const override final;
            virtual void destruct(void* object) const override final;
            virtual bool compare(const void* data1, const void* data2) const override final;
            virtual void copy(void* dest, const void* src) const override final;

            //--

            template< typename T >
            void bindCtorDtor()
            {
                m_traits.requiresConstructor = !std::is_trivially_constructible<T>::value;
                m_traits.requiresDestructor = !std::is_trivially_destructible<T>::value;
                funcConstruct = &prv::CtorHelper<T>::Func;
                funcDestruct = &prv::DtorHelper<T>::Func;
            }

            template< typename T >
            void bindCopy()
            {
                funcCopyAssign = &prv::CopyHelper<T>::Func;
                funcCopyConstruct = &prv::CopyConstructHelper<T>::Func;
                m_traits.simpleCopyCompare = false;
            }

            template< typename T >
            void bindCompare()
            {
                funcComare = &prv::CompareHelper<T>::Func;
                m_traits.simpleCopyCompare = false;
            }

        private:
            mutable std::atomic<const void*> m_defaultObject = nullptr;
        };

        //--

    } // rtti
} // base
