/***
* [#filter: smartptr #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

template< typename T >
INLINE UniquePtr<T>::UniquePtr()
    : m_ptr(nullptr)
{}

template< typename T >
INLINE UniquePtr<T>::UniquePtr(std::nullptr_t)
    : m_ptr(nullptr)
{}

template< typename T >
INLINE UniquePtr<T>::UniquePtr(T* ptr)
    : m_ptr(ptr)
{}

template< typename T >
INLINE UniquePtr<T>::UniquePtr(UniquePtr<T>&& other)
    : m_ptr(other.release())
{}

template< typename T >
INLINE UniquePtr<T>::~UniquePtr()
{
    destroy();
}

template< typename T >
template< typename U >
INLINE UniquePtr<T>::UniquePtr(UniquePtr< U > && other)
    : m_ptr(other.release())
{}

template< typename T >
INLINE UniquePtr<T>& UniquePtr<T>::operator=(UniquePtr<T>&& other)
{
    auto old = m_ptr;
    m_ptr = other.release();
    delete old;
    return *this;
}

template< typename T >
template< typename U >
INLINE UniquePtr<T>& UniquePtr<T>::operator=(UniquePtr<U> && other)
{
    auto old = m_ptr;
    m_ptr = other.release();
    delete old;
    return *this;
}

template< typename T >
INLINE T* UniquePtr<T>::get() const
{
    return m_ptr;
}

template< typename T >
INLINE T& UniquePtr<T>::operator*() const
{
    ASSERT_EX(m_ptr != nullptr, "Accessing NULL UniquePtr");
    return *m_ptr;
}

template< typename T >
INLINE T* UniquePtr<T>::operator->() const
{
    ASSERT_EX(m_ptr != nullptr, "Accessing NULL UniquePtr");
    return m_ptr;
}

template< typename T >
INLINE T* UniquePtr<T>::release()
{
    auto ret  = m_ptr;
    m_ptr = nullptr;
    return ret;
}

template< typename T >
INLINE void UniquePtr<T>::reset(T* ptr)
{
    destroy();
    m_ptr = ptr;
}

template< typename T >
template< typename... Args >
INLINE void UniquePtr<T>::create(Args && ... args)
{
    auto old = m_ptr;
    m_ptr = nullptr;
    m_ptr = new T(std::forward< Args >(args)...);
    delete old;
}

template< typename T >
INLINE UniquePtr<T>::operator bool() const
{
    return (m_ptr);
}

template< typename T >
INLINE bool UniquePtr<T>::operator!() const
{
    return (!m_ptr);
}

template< typename T >
INLINE void UniquePtr<T>::destroy()
{
    if (m_ptr != nullptr)
    {
        delete m_ptr;
        m_ptr = nullptr;
    }
}

//---

template< typename LeftType, typename RightType >
INLINE bool operator==(const UniquePtr< LeftType > & leftPtr, const UniquePtr< RightType > & rightPtr)
{
    return leftPtr.get() == rightPtr.get();
}

template< typename LeftType, typename RightType >
INLINE bool operator!=(const UniquePtr< LeftType > & leftPtr, const UniquePtr< RightType > & rightPtr)
{
    return leftPtr.get() != rightPtr.get();
}

template< typename LeftType, typename RightType >
INLINE bool operator<(const UniquePtr< LeftType > & leftPtr, const UniquePtr< RightType > & rightPtr)
{
    return leftPtr.get() < rightPtr.get();
}

END_BOOMER_NAMESPACE(base)