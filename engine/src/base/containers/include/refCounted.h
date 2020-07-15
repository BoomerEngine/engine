/***
* [#filter: smartptr #]
***/

#pragma once

namespace base
{

    //---

    class IReferencable;

    //---

    /// helper class for referencable objects that want to have weak refs to them
    class BASE_CONTAINERS_API RefWeakContainer : public NoCopy
    {
    public:
        RefWeakContainer(IReferencable* ptr);
        ~RefWeakContainer();

        void addRef();
        void releaseRef();

        void drop(); // invalidates all references EVEN if the object has a non-zero reference count (i.e. "you are dead to me")

        IReferencable* lock(); // returns a valid referencable with +1 reference count or NULL

        INLINE IReferencable* unsafe() const { return m_ptr; }

        INLINE bool expired() const { return m_ptr == nullptr; }

    private:
        std::atomic<uint32_t> m_refCount = 1;
        IReferencable* m_ptr; // unreferenced

        SpinLock m_lock;
    };

    //---

    /// basic implementation of a intrusive reference counting object
    class BASE_CONTAINERS_API IReferencable : public NoCopy
    {
    public:
        // object is constructed with initial refcount of 1, this can be overridden if required
        IReferencable(uint32_t initialRefCount = 1);

        // object can only be destroyed when having 0 reference count
        virtual ~IReferencable();

        //--

        // add a reference
        void addRef();

        // release a reference, when count reaches zero the dispose() function will be called
        void releaseRef();

        //--

        // lock a weak reference
        RefWeakContainer* makeWeakRef() const;

        //--

        // print some object description
        virtual void print(IFormatStream& f) const;

    protected:
        // dispose of this object - called when reference count reaches zero
        virtual void dispose();

    private:
        std::atomic<uint32_t> m_refCount;
        RefWeakContainer* m_weakHolder;
        bool m_realObject = true; // TODO: remove
    };

    //---

    // dump all still allocated reference counted objects
    extern BASE_CONTAINERS_API void DumpLiveRefCountedObjects();

    // enter the default object creation mode on current thread
    extern BASE_CONTAINERS_API void EnterDefaultObjectCreation();

    // leave the default object creation mode on current thread
    extern BASE_CONTAINERS_API void LeaveDefaultObjectCreation();

    // are we creating a default object ?
    extern BASE_CONTAINERS_API bool IsDefaultObjectCreation();

    //---

} // base
