/***
* [#filter: smartptr #]
***/

#pragma once

namespace base
{

    template< typename T >
    INLINE RefWeakPtr<T>::RefWeakPtr()
        : m_holder(nullptr)
    {
        static_assert(std::is_base_of< IReferencable, T >::value, "WeakRef can only be used with IReferencable based types");
    }

    template< typename T >
    INLINE RefWeakPtr<T>::RefWeakPtr(std::nullptr_t)
        : m_holder(nullptr)
    {
        static_assert(std::is_base_of< IReferencable, T >::value, "WeakRef can only be used with IReferencable based types");
    }

    template< typename T >
    INLINE RefWeakPtr<T>::RefWeakPtr(RefWeakContainer* holder) // does not add a holder reference
        : m_holder(holder)
    {}

    template< typename T >
    INLINE RefWeakPtr<T>::RefWeakPtr(const RefWeakPtr<T>& pointer)
        : m_holder(pointer.m_holder)
    {
        if (m_holder)
            m_holder->addRef();
    }

    template< typename T >
    INLINE RefWeakPtr<T>::RefWeakPtr(RefWeakPtr&& pointer)
        : m_holder(pointer.m_holder)
    {
        pointer.m_holder = nullptr;
    }

    template< typename T >
    template< typename U >
    INLINE RefWeakPtr<T>::RefWeakPtr(const RefWeakPtr<U> & pointer)
        : m_holder(pointer.m_holder)
    {
        static_assert(std::is_base_of< T, U >::value || std::is_base_of< U, T >::value, "Unrelated types cannot be casted.");
        ASSERT_EX(0 == (int64_t)static_cast<T*>((U*)nullptr), "Address to object in the shared pointer cannot change due to case");

        if (m_holder)
            m_holder->addRef();
    }

    template< typename T >
    INLINE RefWeakPtr<T>::RefWeakPtr(const T* ptr)
    {
        if (ptr)
            m_holder = ptr->makeWeakRef();
    }

    template< typename T >
    INLINE RefWeakPtr<T>::~RefWeakPtr()
    {
        if (m_holder)
            m_holder->releaseRef();
    }

    template< typename T >
    INLINE RefWeakPtr<T>& RefWeakPtr<T>::operator=(const RefWeakPtr<T>& other)
    {
        //static_assert(std::is_base_of< IReferencable, T >::value, "WeakRef can only be used with IReferencable based types");
        if (other.m_holder != m_holder)
        {
            auto old = m_holder;
            m_holder = other.m_holder;
            if (m_holder)
                m_holder->addRef();
            if (old)
                old->releaseRef();
        }
        return *this;
    }

    template< typename T >
    INLINE RefWeakPtr<T>& RefWeakPtr<T>::operator=(RefWeakPtr&& pointer)
    {
        //static_assert(std::is_base_of< IReferencable, T >::value, "WeakRef can only be used with IReferencable based types");
        if (this != &pointer)
        {
            auto old = m_holder;
            m_holder = pointer.m_holder;
            pointer.m_holder = nullptr;
            if (old)
                old->releaseRef();
        }
        return *this;
    }

    template< typename T >
    INLINE RefWeakContainer* RefWeakPtr<T>::holder() const
    {
        return m_holder;
    }

    template< typename T >
    template< typename U >
    INLINE RefWeakPtr<T>& RefWeakPtr<T>::operator=(const RefWeakPtr<U> & other)
    {
        static_assert(std::is_base_of< IReferencable, T >::value, "WeakRef can only be used with IReferencable based types");
        static_assert(std::is_base_of< T, U >::value || std::is_base_of< U, T >::value, "Unrelated types cannot be casted.");
        ASSERT_EX(0 == (int64_t)static_cast<T*>((U*)nullptr), "Address to object in the shared pointer cannot change due to case");

        if (other.m_holder != m_holder)
        {
            auto old = m_holder;
            m_holder = other.m_holder;
            if (m_holder)
                m_holder->addRef();
            if (old)
                old->releaseRef();
        }
        return *this;
    }

    template< typename T >
    template< typename U >
    INLINE RefWeakPtr<T>& RefWeakPtr<T>::operator=(const RefPtr<U>& other)
    {
        static_assert(std::is_base_of< IReferencable, T >::value, "WeakRef can only be used with IReferencable based types");
        static_assert(std::is_base_of< T, U >::value || std::is_base_of< U, T >::value, "Unrelated types cannot be casted.");
        ASSERT_EX(0 == (int64_t)static_cast<T*>((U*)nullptr), "Address to object in the shared pointer cannot change due to case");

        auto old = m_holder;
        m_holder = other ? static_cast<IReferencable*>(other)->makeWeakRef() : nullptr;
        if (m_holder)
            m_holder->addRef();
        if (old)
            old->releaseRef();

        return *this;
    }

    template< typename T >
    INLINE RefPtr<T> RefWeakPtr<T>::lock() const
    {
        static_assert(std::is_base_of< IReferencable, T >::value, "WeakRef can only be used with IReferencable based types");
        if (m_holder)
        {
            if (auto* ptr = m_holder->lock())
                return RefPtr<T>(NoAddRef(static_cast<T*>(ptr))); // lock() already provided reference, do not add another one
        }

        return RefPtr<T>();
    }

    template< typename T >
    INLINE T* RefWeakPtr<T>::unsafe() const
    {
        static_assert(std::is_base_of< IReferencable, T >::value, "WeakRef can only be used with IReferencable based types");
        if (m_holder)
            return static_cast<T*>(m_holder->unsafe());

        return nullptr;
    }

    template< typename T >
    INLINE bool RefWeakPtr<T>::expired() const
    {
        return !m_holder || m_holder->expired();
    }

    template< typename T >
    INLINE bool RefWeakPtr<T>::empty() const
    {
        return nullptr == m_holder;
    }

    template< typename T >
    INLINE RefWeakPtr<T>::operator bool() const
    {
        return nullptr != m_holder;
    }

    template< typename T >
    INLINE bool RefWeakPtr<T>::operator!() const
    {
        return nullptr == m_holder;
    }

    template< typename T >
    INLINE void RefWeakPtr<T>::reset()
    {
        if (m_holder)
        {
            m_holder->releaseRef();
            m_holder = nullptr;
        }
    }

    template< typename T >
    uint32_t RefWeakPtr<T>::CalcHash(const RefWeakPtr<T>& key)
    {
        return std::hash<const void*>{}(key.m_holder);
    }

    //---

    // get weak pointer
    template< typename T >
    INLINE RefWeakPtr<T> RefPtr<T>::weak() const
    {
        if (m_ptr)
            return RefWeakPtr<T>(m_ptr->makeWeakRef());
        return RefWeakPtr<T>();
    }

    template<typename T>
    template<typename U>
    INLINE RefPtr<U> RefPtr<T>::staticCast() const
    {
        static_assert(std::is_base_of< T, U >::value || std::is_base_of< U, T >::value, "Unrelated types cannot be casted.");
        ASSERT_EX(0 == (int64_t)static_cast<T*>((U*)nullptr), "Address to object in the shared pointer cannot change due to case");
        return RefPtr<U>(AddRef<U>(static_cast<U*>(m_ptr)));
    }

    //---

    template<typename T>
    template<typename U>
    INLINE RefWeakPtr<U> RefWeakPtr<T>::staticCast() const
    {
        static_assert(std::is_base_of< T, U >::value || std::is_base_of< U, T >::value, "Unrelated types cannot be casted.");
        ASSERT_EX(0 == (int64_t)static_cast<T*>((U*)nullptr), "Address to object in the shared pointer cannot change due to case");
        return *reinterpret_cast<RefWeakPtr<U>*>(this);
    }

    //---
    // RefWeakPtr
    //---

    template< typename T >
    template< typename U >
    INLINE bool RefWeakPtr<T>::operator==(const RefWeakPtr<U>& other) const { return holder() == other.holder(); }

    template< typename T >
    template< typename U >
    INLINE bool RefWeakPtr<T>::operator==(const RefPtr<U>& other) const { return unsafe() == other.get(); }

    template< typename T >
    INLINE bool RefWeakPtr<T>::operator==(std::nullptr_t) const { return empty(); }

    template< typename T >
    template< typename U >
    INLINE bool RefWeakPtr<T>::operator==(U* ptr) const { return unsafe() == ptr; }

    //---

    template< typename T >
    template< typename U >
    INLINE bool RefWeakPtr<T>::operator!=(const RefWeakPtr<U>& other) const { return holder() != other.holder(); }

    template< typename T >
    template< typename U >
    INLINE bool RefWeakPtr<T>::operator!=(const RefPtr<U>& other) const { return unsafe() != other.get(); }

    template< typename T >
    INLINE bool RefWeakPtr<T>::operator!=(std::nullptr_t) const { return !empty(); }

    template< typename T >
    template< typename U >
    INLINE bool RefWeakPtr<T>::operator!=(U* ptr) const { return unsafe() != ptr; }

    //--

    INLINE UntypedRefWeakPtr::UntypedRefWeakPtr()
    {}

    INLINE UntypedRefWeakPtr::UntypedRefWeakPtr(std::nullptr_t)
    {}

    INLINE UntypedRefWeakPtr::UntypedRefWeakPtr(RefWeakContainer* holder)
        : m_holder(holder)
    {}

    INLINE UntypedRefWeakPtr::UntypedRefWeakPtr(const UntypedRefWeakPtr& pointer)
        : m_holder(pointer.m_holder)
    {
        if (m_holder)
            m_holder->addRef();
    }

    INLINE UntypedRefWeakPtr::UntypedRefWeakPtr(UntypedRefWeakPtr&& pointer)
        : m_holder(pointer.m_holder)
    {
        pointer.m_holder = nullptr;
    }

    template< typename T >
    INLINE UntypedRefWeakPtr::UntypedRefWeakPtr(T* ptr)
    {
        if (ptr)
            m_holder = ptr->makeWeakRef();
    }

    INLINE UntypedRefWeakPtr::~UntypedRefWeakPtr()
    {
        if (m_holder)
        {
            m_holder->releaseRef();
            m_holder = nullptr;
        }
    }

    INLINE UntypedRefWeakPtr& UntypedRefWeakPtr::operator=(const UntypedRefWeakPtr& pointer)
    {
        if (pointer.m_holder != m_holder)
        {
            auto old = m_holder;
            m_holder = pointer.m_holder;
            if (m_holder)
                m_holder->addRef();
            if (old)
                old->releaseRef();
        }
        return *this;
    }

    INLINE UntypedRefWeakPtr& UntypedRefWeakPtr::operator=(UntypedRefWeakPtr&& pointer)
    {
        if (this != &pointer)
        {
            auto old = m_holder;
            m_holder = pointer.m_holder;
            pointer.m_holder = nullptr;
            if (old)
                old->releaseRef();
        }
        return *this;
    }

    INLINE RefWeakContainer* UntypedRefWeakPtr::holder() const
    {
        return m_holder;
    }

    INLINE void UntypedRefWeakPtr::reset()
    {
        if (m_holder)
        {
            m_holder->releaseRef();
            m_holder = nullptr;
        }
    }

    INLINE IReferencable* UntypedRefWeakPtr::lock() const
    {
        return m_holder ? m_holder->lock() : nullptr;
    }

    INLINE IReferencable* UntypedRefWeakPtr::unsafe() const
    {
        return m_holder ? m_holder->unsafe() : nullptr;
    }

    INLINE bool UntypedRefWeakPtr::expired() const
    {
        return !m_holder || m_holder->expired();
    }

    template< typename T >
    INLINE UntypedRefWeakPtr::UntypedRefWeakPtr(const RefPtr<T>& ptr)
    {
        if (ptr)
            m_holder = ptr->makeWeakRef();
    }

} // base
