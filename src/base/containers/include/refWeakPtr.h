/***
* [#filter: smartptr #]
***/

#pragma once

#include "refPtr.h"

BEGIN_BOOMER_NAMESPACE(base)

//---

class RefWeakContainer;

//--

/// untyped wrapper for RefWeakContainer
class BASE_CONTAINERS_API UntypedRefWeakPtr
{
public:
    INLINE UntypedRefWeakPtr();
    INLINE UntypedRefWeakPtr(std::nullptr_t);
    INLINE UntypedRefWeakPtr(RefWeakContainer* holder); // does not add a holder reference
    INLINE UntypedRefWeakPtr(const UntypedRefWeakPtr& pointer);
    INLINE UntypedRefWeakPtr(UntypedRefWeakPtr&& pointer);

    template< typename T >
    INLINE UntypedRefWeakPtr(T* ptr); // never adds a reference since it's a weak ptr

    template< typename T >
    INLINE UntypedRefWeakPtr(const RefPtr<T>& ptr); // never adds a reference since it's a weak ptr

    INLINE ~UntypedRefWeakPtr();

    INLINE UntypedRefWeakPtr& operator=(const UntypedRefWeakPtr& pointer);
    INLINE UntypedRefWeakPtr& operator=(UntypedRefWeakPtr&& pointer);

    // get the internal holder
    INLINE RefWeakContainer* holder() const;

    // release reference being held here
    INLINE void reset();

    // lock to get the actual ref
    INLINE IReferencable* lock() const;

    // get unsafe pointer, you must be SURE that nothing will release reference in the mean time
    INLINE IReferencable* unsafe() const;

    // did this weak pointer expired ?
    INLINE bool expired() const;

    //--

    INLINE bool operator==(const UntypedRefWeakPtr& other) const { return m_holder == other.m_holder; }
    INLINE bool operator!=(const UntypedRefWeakPtr& other) const { return m_holder != other.m_holder; }

    template< typename T >
    INLINE bool operator==(T* ptr) const { return (void*)unsafe() == (void*)ptr; }

    template< typename T >
    INLINE bool operator!=(T* ptr) const { return (void*)unsafe() != (void*)ptr; }

    INLINE bool operator==(std::nullptr_t) const { return m_holder == nullptr; }
    INLINE bool operator!=(std::nullptr_t) const { return m_holder != nullptr; }

private:
    RefWeakContainer* m_holder = nullptr;
};

//--

/// wrapper for pointers that have the addRef/releaseRef methods
template< typename T >
class RefWeakPtr
{
public:
    INLINE RefWeakPtr();
    INLINE RefWeakPtr(std::nullptr_t);
    INLINE RefWeakPtr(RefWeakContainer* holder); // does not add a holder reference
    INLINE RefWeakPtr(const T* ptr); // never adds a reference since it's a weak ptr
    INLINE RefWeakPtr(const RefWeakPtr& pointer);
    INLINE RefWeakPtr(RefWeakPtr&& pointer);
        
    template< typename U >
    INLINE RefWeakPtr(const RefWeakPtr<U>& pointer);

    INLINE ~RefWeakPtr();

    INLINE RefWeakPtr& operator=(const RefWeakPtr& pointer);
    INLINE RefWeakPtr& operator=(RefWeakPtr&& pointer);

    template< typename U >
    INLINE RefWeakPtr& operator=(const RefWeakPtr<U> & pointer);

    template< typename U >
    INLINE RefWeakPtr& operator=(const RefPtr<U>& pointer);

    //---

    // get the internal holder
    INLINE RefWeakContainer* holder() const;

    // release reference being held here
    INLINE void reset();

    // lock to get the actual ref
    INLINE RefPtr<T> lock() const;

    // get unsafe pointer, you must be SURE that nothing will release reference in the mean time
    INLINE T* unsafe() const;

    // did this weak pointe expired ?
    INLINE bool expired() const;

    //---

    template< typename U >
    INLINE bool operator==(const RefWeakPtr<U>& other) const;

    template< typename U >
    INLINE bool operator==(const RefPtr<U>& other) const;

    template< typename U >
    INLINE bool operator==(U* ptr) const;

    INLINE bool operator==(std::nullptr_t) const;
    //---

    template< typename U >
    INLINE bool operator!=(const RefWeakPtr<U>& other) const;

    template< typename U >
    INLINE bool operator!=(const RefPtr<U>& other) const;

    INLINE bool operator!=(std::nullptr_t) const;

    template< typename U >
    INLINE bool operator!=(U* ptr) const;

    //---

    // boolean expression - test only emptiness
    INLINE explicit operator bool() const;
    INLINE bool operator!() const;

    // empty ?
    INLINE bool empty() const;

    // static cast to another shared pointer, DOES NOT CHECK DYNAMIC TYPE but the types must be related (ie. from the same type tree)
    template< typename U >
    INLINE RefWeakPtr<U> staticCast() const;

    //--

    INLINE static uint32_t CalcHash(const RefWeakPtr<T>& key);
        
private:
    RefWeakContainer* m_holder = nullptr;
};

//---

END_BOOMER_NAMESPACE(base)

#include "refWeakPtr.inl"
