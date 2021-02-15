/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\class #]
***/

#include "build.h"
#include "rttiClassType.h"
#include "rttiNativeClassType.h"

namespace base
{
    namespace rtti
    {
        //--

        NativeClass::NativeClass(const char* name, uint32_t size, uint32_t alignment, uint64_t nativeHash, PoolTag pool)
            : IClassType(StringID(name), size, alignment, pool)
        {
            DEBUG_CHECK_EX(nativeHash != 0, "Native class with no native type hash");
            m_traits.nativeHash = nativeHash;
            m_traits.pool = pool;
        }

        bool NativeClass::isAbstract() const
        {
            DEBUG_CHECK_EX((!funcConstruct && !funcDestruct) || (funcConstruct && funcDestruct), TempString("Both constructor and destructor should be specified or neither at class '{}'", name()));
            return !funcConstruct || !funcDestruct;
        }

        const void* NativeClass::defaultObject() const
        {
            const void* currentDefault = m_defaultObject.load();
            if (!currentDefault)
            {
                currentDefault = createDefaultObject();
                m_defaultObject.exchange(currentDefault); // note: we don't care if we leak a default object, they are all the same and read only
            }
            return currentDefault;
        }

        void NativeClass::destroyDefaultObject() const
        {
            if (auto* ptr = (void*)m_defaultObject.exchange(nullptr))
            {
                EnterDefaultObjectCreation();
                if (funcDestruct)
                    funcDestruct(ptr);
                LeaveDefaultObjectCreation();
            }
        }

        const void* NativeClass::createDefaultObject() const
        {
            void* mem = mem::AllocateBlock(POOL_DEFAULT_OBJECTS, size(), alignment(), name().c_str());
            memzero(mem, size());

            EnterDefaultObjectCreation();
            if (funcConstruct)
                funcConstruct(mem);
            LeaveDefaultObjectCreation();

            return mem;
        }

        void NativeClass::construct(void* object) const
        {
            auto memSize = size();

            if (funcConstruct)
            {
#ifdef BUILD_DEBUG
                memset(object, 0xB0, memSize); // fill with crap to detect uninitialized properties better
#endif
                funcConstruct(object);
            }
            else
            {
                memzero(object, memSize); // we don't have a constructor function, just zero initialize the memory
            }
        }
    
        void NativeClass::destruct(void* object) const
        {
            if (funcDestruct)
                funcDestruct(object);
        }

        bool NativeClass::compare(const void* data1, const void* data2) const
        {
            if (funcComare)
                return funcComare(data1, data2);
            else
                return IClassType::compare(data1, data2);
        }

        void NativeClass::copy(void* dest, const void* src) const
        {
            if (funcCopyAssign)
                return funcCopyAssign(dest, src);
            else
                IClassType::copy(dest, src);
        }

        //--

    } // rtti
} // base
