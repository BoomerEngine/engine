/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: smartptr #]
***/

#include "build.h"
#include "refCounted.h"
#include "refPtr.h"
#include "refWeakPtr.h"

#include <unordered_set>

namespace base
{
    ///--

    mem::PoolID POOL_REF_HOLDER("Memory.RefHolder");
    mem::PoolID POOL_REF_PTR("Memory.Referencable");

    ///--

    RefWeakContainer::RefWeakContainer(IReferencable* ptr)
        : m_ptr(ptr)
    {}

    RefWeakContainer::~RefWeakContainer()
    {
        DEBUG_CHECK_EX(m_refCount.load() == 0, "Invalid ref count");
        DEBUG_CHECK_EX(m_ptr == nullptr, "Object still referenced");
    }

    void RefWeakContainer::addRef()
    {
        ++m_refCount;
    }

    void RefWeakContainer::releaseRef()
    {
        if (0 == --m_refCount)
        {
            MemDelete(this);
        }
    }

    void RefWeakContainer::drop()
    {
        auto lock = CreateLock(m_lock);
        m_ptr = nullptr;
    }

    IReferencable* RefWeakContainer::lock()
    {
        IReferencable* ret = nullptr;

        {
            auto lock = CreateLock(m_lock);
            if (m_ptr)
            {
                m_ptr->addRef();
                ret = m_ptr;
            }
        }

        return ret;
    }

    ///--

    class PointerTrackingRegistry
    {
    public:
        PointerTrackingRegistry()
        {}

        static PointerTrackingRegistry& GetInstance()
        {
            static PointerTrackingRegistry theInstance;
            return theInstance;
        }

        //--

        Mutex m_lock;

        struct TrackingPointer;

        struct TrackingRef
        {
            uint32_t callstack = 0;
        };

        std::atomic<uint32_t> m_activeRefIndex = 1;
        std::unordered_map<uint32_t, TrackingRef*> m_activeRefs;

        struct TrackingPointer
        {
            IReferencable* ptr = nullptr;
            uint32_t creationCallstack = 0;
            std::unordered_set<uint32_t> refs;
        };

        std::unordered_map<void*, TrackingPointer*> m_activePointers;

        //--

        uint32_t CreateRef()
        {
            auto index = m_activeRefIndex++;
            auto callstackIndex = debug::CaptureCallstack(2);

            {
                auto lock = CreateLock(m_lock);
                auto* entry = MemNew(TrackingRef).ptr;
                entry->callstack = callstackIndex;
                m_activeRefs[index] = entry;
            }

            return index;
        }

        void ReleaseRef(uint32_t index)
        {
            auto lock = CreateLock(m_lock);

            auto ptr = m_activeRefs.find(index);
            if (ptr != m_activeRefs.end())
                MemDelete(ptr->second);
            m_activeRefs.erase(ptr);
        }

        void TrackPointer(IReferencable* ptr)
        {
            auto lock = CreateLock(m_lock);

            auto it = m_activePointers.find(ptr);
            ASSERT_EX(it == m_activePointers.end(), "Pointer already tracked");
            if (it != m_activePointers.end())
                return;

            auto* entry = MemNew(TrackingPointer).ptr;
            entry->ptr = ptr;
            entry->creationCallstack = debug::CaptureCallstack(2);
            m_activePointers[ptr] = entry;
        }

        void UntrackPointer(IReferencable* ptr)
        {
            auto lock = CreateLock(m_lock);

            auto it = m_activePointers.find(ptr);
            DEBUG_CHECK_EX(it != m_activePointers.end(), "Pointer not tracked");
            if (it != m_activePointers.end())
            {
                DEBUG_CHECK_EX(it->second->refs.empty(), "Pointer has tracking refs");
                MemDelete(it->second);
                m_activePointers.erase(it);
            }
        }

        void PrintActivePointer()
        {
            auto lock = CreateLock(m_lock);

            TRACE_INFO("There are still {} active IReferencables", m_activePointers.size());

            uint32_t index = 0;
            for (const auto& it : m_activePointers)
            {
                TRACE_INFO("  [{}]: 0x{} ({})", index, Hex((uint64_t)it.second->ptr), typeid(*(it.second->ptr)).name());

                const auto& stack = debug::ResolveCapturedCallstack(it.second->creationCallstack);
                if (!stack.empty())
                    TRACE_INFO("     Creation callstack: {}", stack);

                index += 1;
            }
        }
    };

    ///--

    IReferencable::IReferencable(uint32_t initialRefCount /*= 1*/)
        : m_refCount(initialRefCount)
        , m_realObject(!IsDefaultObjectCreation())
    {
        m_weakHolder = MemNewPool(POOL_REF_HOLDER, RefWeakContainer, this);

        if (m_realObject)
            PointerTrackingRegistry::GetInstance().TrackPointer(this);
    }

    IReferencable::~IReferencable()
    {
        if (m_realObject)
        {
            DEBUG_CHECK_EX(m_refCount.load() == 0, "Deleting object with non zero reference count");
            DEBUG_CHECK_EX(m_weakHolder == nullptr, "Weak holder not released");
        }

        if (m_weakHolder)
        {
            m_weakHolder->drop();
            m_weakHolder->releaseRef();
            m_weakHolder = nullptr;
        }

        if (m_realObject)
            PointerTrackingRegistry::GetInstance().UntrackPointer(this);
    }

    void IReferencable::addRef()
    {
        ++m_refCount;
    }

    void IReferencable::releaseRef()
    {
        auto refCount = --m_refCount;
        DEBUG_CHECK_EX(refCount >= 0, "Invalid reference count on release");
        if (0 == refCount)
            dispose();
    }

    RefWeakContainer* IReferencable::makeWeakRef() const
    {
        auto* ret = m_weakHolder;
        if (ret)
            ret->addRef();
        return ret;
    }

    void IReferencable::dispose()
    {
        m_weakHolder->drop();
        m_weakHolder->releaseRef();
        m_weakHolder = nullptr;

        MemDelete(this);
    }

    ///--

    BaseRefPtr::BaseRefPtr()
    {}

    void BaseRefPtr::addRefInternal(void* ptr)
    {
        DEBUG_CHECK_EX(m_currentTrackingId == 0, "Ref already tracking something");

        if (ptr)
        {
            //m_currentTrackingId =
            ((IReferencable*)ptr)->addRef();
        }
    }

    void BaseRefPtr::swapRefInternal(void** ptr, void* newPtr)
    {
        if (*ptr != newPtr)
        {
            auto oldPtr = *(IReferencable**)ptr;
            auto oldTracking = m_currentTrackingId;
            m_currentTrackingId = 0;
            *ptr = nullptr;

            if (newPtr)
            {
                *ptr = newPtr;
                (*(IReferencable**)ptr)->addRef();
            }

            if (oldPtr)
            {
                oldPtr->releaseRef();
            }
        }
    }

    void BaseRefPtr::releaseRefInternal(void** ptr)
    {
        if (*ptr)
        {
            auto old = *(IReferencable**)ptr;
            *ptr = nullptr;
            old->releaseRef();
        }
    }

    ///--

    void DumpLiveRefCountedObjects()
    {
        PointerTrackingRegistry::GetInstance().PrintActivePointer();
    }

    ///--

    TYPE_TLS volatile uint32_t GDefaultObjectCreationDepth = 0;

    void EnterDefaultObjectCreation()
    {
        GDefaultObjectCreationDepth += 1;
    }

    void LeaveDefaultObjectCreation()
    {
        ASSERT(GDefaultObjectCreationDepth > 0);
        GDefaultObjectCreationDepth -= 1;
    }

    bool IsDefaultObjectCreation()
    {
        return GDefaultObjectCreationDepth > 0;
    }

    //--

} // base
