/***
* [#filter: smartptr #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

template< typename T >
INLINE RefPtr<T>::RefPtr()
    : m_ptr(nullptr)
{}

template< typename T >
INLINE RefPtr<T>::RefPtr(std::nullptr_t)
    : m_ptr(nullptr)
{}

template< typename T >
INLINE RefPtr<T>::RefPtr(const AddRefWrapper<T>& ptr)
    : m_ptr(ptr.ptr)
{
    addRefInternal(m_ptr);
}

template< typename T >
INLINE RefPtr<T>::RefPtr(const NoAddRefWrapper<T>& ptr)
    : m_ptr(ptr.ptr)
{
}

template< typename T >
INLINE RefPtr<T>::RefPtr(const RefPtr<T>& other)
    : m_ptr(other.m_ptr)
{
    addRefInternal(m_ptr);
}

template< typename T >
INLINE RefPtr<T>::RefPtr(RefPtr<T>&& other)
{
    swapRefInternal((void**)&m_ptr, (void*)other.get());
    other.reset();
    //other.m_ptr = nullptr;
}

template< typename T >
template< typename U >
INLINE RefPtr<T>::RefPtr(const RefPtr<U>& other)
    : m_ptr(static_cast<T*>(other.get()))
{
    ASSERT_EX(0 == (int64_t) static_cast<T*>((U*)nullptr), "Address to object in the shared pointer cannot change due to case");
    static_assert(std::is_base_of<T, U>::value, "Cannot down-cast a pointer through construction, use rtti_cast");

    addRefInternal((void*)m_ptr);
}

template< typename T >
INLINE RefPtr<T>::~RefPtr()
{
    releaseRefInternal((void**)&m_ptr);
}

template< typename T >
INLINE RefPtr<T>& RefPtr<T>::operator=(const RefPtr<T>& other)
{
    swapRefInternal((void**)&m_ptr, (void*)other.get());
    return *this;
}

template< typename T >
INLINE RefPtr<T>& RefPtr<T>::operator=(RefPtr<T>&& other)
{
    if (this != &other)
    {
        swapRefInternal((void**)&m_ptr, (void*)other.get());
        other.reset();
/*
        auto prev  = m_ptr;
            
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
            
        if (prev)
            prev->releaseRef();*/
    }

    return *this;
}

template< typename T >
template< typename U >
INLINE RefPtr<T>& RefPtr<T>::operator=(const RefPtr<U>& other)
{
    auto* otherPtr = static_cast<U*>(other.get());
    if (m_ptr != otherPtr)
    {
        swapRefInternal((void**)&m_ptr, (void*)other.get());
        /*auto old = m_ptr;
        m_ptr = otherPtr;
        if (m_ptr)
            m_ptr->addRef();
        if (old)
            old->releaseRef();*/
    }
    return *this;
}

template< typename T >
INLINE void RefPtr<T>::reset()
{
    releaseRefInternal((void**)&m_ptr);
}

template< typename T >
INLINE T* RefPtr<T>::release()
{
    auto* ret = m_ptr;
    m_ptr = nullptr;
    return ret;
}

template< typename T >
INLINE T* RefPtr<T>::get() const
{
    return m_ptr;
}

template< typename T >
INLINE void RefPtr<T>::print(IFormatStream& f) const
{
    printRefInternal(m_ptr, f);
}

//---

template< typename T >
INLINE RefPtr<T>::operator bool() const
{
    return (m_ptr);
}

template< typename T >
INLINE bool RefPtr<T>::operator!() const
{
    return (!m_ptr);
}

template< typename T >
INLINE T& RefPtr<T>::operator*() const
{
    ASSERT_EX(m_ptr != nullptr, "Accessing NULL RefPtr");
    return *m_ptr;
}

template< typename T >
INLINE bool RefPtr<T>::empty() const
{
    return m_ptr == nullptr;
}

template< typename T >
INLINE T* RefPtr<T>::operator->() const
{
    ASSERT_EX(m_ptr != nullptr, "Accessing NULL RefPtr");
    return (T*)m_ptr;
}

template< typename T >
INLINE uint32_t RefPtr<T>::CalcHash(const RefPtr<T>& key)
{
    return std::hash<const void*>{}(key.m_ptr);
}

template< typename T >
INLINE uint32_t RefPtr<T>::CalcHash(const void* ptr)
{
    return std::hash<const void*>{}(ptr);
}

//---

template< typename T >
template< typename U >
INLINE bool RefPtr<T>::operator==(const RefPtr<U>& other) const
{
    return m_ptr == other.get();
}

template< typename T >
template< typename U >
INLINE bool RefPtr<T>::operator==(U* ptr) const
{
    return m_ptr == ptr;
}

template< typename T >
INLINE bool RefPtr<T>::operator==(std::nullptr_t) const
{
    return m_ptr == nullptr;
}

//---

template< typename T >
template< typename U >
INLINE bool RefPtr<T>::operator!=(const RefPtr<U>& other) const
{
    return m_ptr != other.m_ptr;
}

template< typename T >
INLINE bool RefPtr<T>::operator!=(std::nullptr_t) const
{
    return m_ptr != nullptr;
}

template< typename T >
template< typename U >
INLINE bool RefPtr<T>::operator!=(U* ptr) const
{
    return m_ptr != ptr;
}

//---

END_BOOMER_NAMESPACE()
