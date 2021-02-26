/***
* [#filter: smartptr #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

template< typename T >
class RefWeakPtr;

//---

template< typename T >
struct NoAddRefWrapper : public NoCopy
{
    INLINE NoAddRefWrapper(std::nullptr_t) {};
    INLINE explicit NoAddRefWrapper(T* ptr_) : ptr(ptr_) {};

    //INLINE operator bool() const { return ptr != nullptr; }
    INLINE operator T* () const { return ptr; }
    INLINE T* operator->() const { return ptr; }
    INLINE T& operator*() const { return *ptr; }

    T* ptr = nullptr;
};

template< typename T >
struct AddRefWrapper : public NoCopy
{
    INLINE AddRefWrapper(std::nullptr_t) {};
    INLINE explicit AddRefWrapper(T* ptr_) : ptr(ptr_) {};

    //INLINE operator bool() const { return ptr != nullptr; }
    INLINE operator T* () const { return ptr; }
    INLINE T* operator->() const { return ptr; }
    INLINE T& operator*() const { return *ptr; }

    T* ptr = nullptr;
};

//---

/// base reference pointer
class CORE_CONTAINERS_API BaseRefPtr
{
public:
    BaseRefPtr();

protected:
    void addRefInternal(void* ptr);
    void releaseRefInternal(void** ptr);
    void swapRefInternal(void** ptr, void* newPtr);
    void printRefInternal(void* ptr, IFormatStream& f) const;

    uint32_t m_currentTrackingId = 0;
};

//---

/// wrapper for pointers that have the addRef/releaseRef methods
template< typename T >
class RefPtr : public BaseRefPtr
{
public:
    INLINE RefPtr();
    INLINE RefPtr(std::nullptr_t);
    INLINE RefPtr(const AddRefWrapper<T>& ptr); // adds a reference
    INLINE RefPtr(const NoAddRefWrapper<T>& ptr); // does not add a reference
    INLINE RefPtr(const RefPtr& pointer);
    INLINE RefPtr(RefPtr&& pointer);
        
    template< typename U >
    INLINE RefPtr(const RefPtr<U>& pointer);

    INLINE ~RefPtr();

    INLINE RefPtr& operator=(const RefPtr& pointer);
    INLINE RefPtr& operator=(RefPtr&& pointer);

    template< typename U >
    INLINE RefPtr& operator=(const RefPtr<U> & pointer);

    //---

    // release reference being held here
    INLINE void reset();

    // release from the refcounting
    INLINE T* release();

    // get internal pointer
    INLINE T* get() const;

    //---

    template< typename U >
    INLINE bool operator==(const RefPtr<U>& other) const;

    template< typename U >
    INLINE bool operator==(U* ptr) const;

    INLINE bool operator==(std::nullptr_t) const;
    //---

    template< typename U >
    INLINE bool operator!=(const RefPtr<U>& other) const;

    INLINE bool operator!=(std::nullptr_t) const;

    template< typename U >
    INLINE bool operator!=(U* ptr) const;

    //---

    // pointer/reference access
    INLINE T& operator*() const;
    INLINE T* operator->() const;

    // pointer cast
    INLINE operator T*() const { return get(); }

    // boolean expression
    INLINE explicit operator bool() const;
    INLINE bool operator!() const;

    // empty ?
    INLINE bool empty() const;

    // static cast to another shared pointer, DOES NOT CHECK DYNAMIC TYPE but the types must be related (ie. from the same type tree)
    template< typename U >
    INLINE RefPtr<U> staticCast() const;

    // make a weak reference
    INLINE RefWeakPtr<T> weak() const;

    //--
        
    // debug print, prints the NULL or calls the print() on the reference object
    INLINE void print(IFormatStream& f) const;

    //--

    INLINE static uint32_t CalcHash(const RefPtr<T>& key);
    INLINE static uint32_t CalcHash(const void* ptr);

protected:
    T* m_ptr = nullptr;
};

//---

END_BOOMER_NAMESPACE()

#include "refPtr.inl"
