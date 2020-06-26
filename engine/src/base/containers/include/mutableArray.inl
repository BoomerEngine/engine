/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/system/include/scopeLock.h"

namespace base
{

    //---

    template< typename T >
    INLINE MutableArray<T>::MutableArray()
        : m_lockCount(0)
        , m_numRemoved(0)
    {}

    template< typename T >
    INLINE MutableArray<T>::~MutableArray()
    {
        ASSERT_EX(m_lockCount == 0, "Destroying mutable array that has active iterator");
    }

    template< typename T >
    INLINE void MutableArray<T>::clear()
    {
        ScopeLock<> lock(m_lock);
        m_values.clear();
    }

    template< typename T >
    INLINE bool MutableArray<T>::empty() const
    {
        return m_values.empty();
    }

    template< typename T >
    INLINE bool MutableArray<T>::locked() const
    {
        return m_lockCount > 0;
    }

    template< typename T >
    INLINE void MutableArray<T>::pushBack(const T& data)
    {
        ScopeLock<> lock(m_lock);
        m_values.pushBack(data);
    }

    template< typename T >
    INLINE void MutableArray<T>::remove(const T& data, const T& empty)
    {
        ScopeLock<> lock(m_lock);

        if (m_lockCount > 0)
        {
            auto index = m_values.find(data);
            if (index != INDEX_NONE)
            {
                m_numRemoved += 1;
                m_values[index] = empty;
            }
        }
        else
        {
            m_values.remove(data);
        }
    }

    template< typename T >
    INLINE MutableArrayIterator<T> MutableArray<T>::begin() const
    {
        lock();
        return MutableArrayIterator<T>(*this);
    }

    template< typename T >
    INLINE MutableArrayIterator<T> MutableArray<T>::end() const
    {
        return MutableArrayIterator<T>(*this, 0);
    }

    template< typename T >
    void MutableArray<T>::lock() const
    {
        m_lock.acquire();
        m_lockCount += 1;
    }

    template< typename T >
    void MutableArray<T>::unlock() const
    {
        ASSERT_EX(m_lockCount > 0, "Invalid lock count");
        if (0 == --m_lockCount)
        {
            if (m_numRemoved > 0)
                m_values.remove(T());
        }
        m_lock.release();
    }

    //---

    template< typename T >
    INLINE MutableArrayIterator<T>::MutableArrayIterator(const MutableArray<T>& ar)
            : m_array(&ar)
            , m_data(&ar.m_values)
            , m_index(-1)
            , m_count((int)ar.m_values.size())
            , m_locked(true)
    {
        advance();
    }

    template< typename T >
    INLINE MutableArrayIterator<T>::MutableArrayIterator(const MutableArray<T>& ar, const int)
            : m_array(&ar)
            , m_data(&ar.m_values)
            , m_index((int)ar.m_values.size()) // end of list
            , m_count((int)ar.m_values.size())
            , m_locked(false)
    {}

    template< typename T >
    INLINE MutableArrayIterator<T>::~MutableArrayIterator()
    {
        if (m_locked)
        {
            m_array->unlock();
            m_array = nullptr;
        }
    }

    template< typename T >
    INLINE MutableArrayIterator<T>::MutableArrayIterator(const MutableArrayIterator<T>& other)
        : m_array(other.m_array)
        , m_data(other.m_data)
        , m_index(other.m_index)
        , m_count(other.m_count)
        , m_locked(other.m_locked)
    {
        if (m_locked)
            m_array->lock();
    }

    template< typename T >
    INLINE MutableArrayIterator<T>::MutableArrayIterator(MutableArrayIterator<T>&& other)
        : m_array(other.m_array)
        , m_data(other.m_data)
        , m_index(other.m_index)
        , m_count(other.m_count)
        , m_locked(other.m_locked)
    {
        other.m_array = nullptr;
        other.m_data = nullptr;
        other.m_index = 0;
        other.m_count = 0;
        other.m_locked = false;
    }

    template< typename T >
    INLINE MutableArrayIterator<T>& MutableArrayIterator<T>::operator=(const MutableArrayIterator<T>& other)
    {
        if (this != &other)
        {
            if (m_locked)
                m_array->unlock();

            m_array = other.m_array;
            m_data = other.m_data;
            m_index = other.m_index;
            m_count = other.m_count;
            m_locked = other.m_locked;

            if (m_locked)
                m_array->lock();
        }

        return *this;
    }

    template< typename T >
    INLINE MutableArrayIterator<T>& MutableArrayIterator<T>::operator=(MutableArrayIterator<T>&& other)
    {
        if (this != &other)
        {
            if (m_locked)
                m_array->unlock();

            m_array = other.m_array;
            m_data = other.m_data;
            m_index = other.m_index;
            m_count = other.m_count;
            m_locked = other.m_locked;

            other.m_array = nullptr;
            other.m_data = nullptr;
            other.m_index = 0;
            other.m_count = 0;
            other.m_locked = false;
        }

        return *this;
    }

    template< typename T >
    INLINE bool MutableArrayIterator<T>::operator==(const MutableArrayIterator<T>& other) const
    {
        return (other.m_array == m_array) && (other.m_index == m_index);
    }

    template< typename T >
    INLINE bool MutableArrayIterator<T>::operator!=(const MutableArrayIterator<T>& other) const
    {
        return (other.m_array != m_array) || (other.m_index != m_index);
    }

    template< typename T >
    INLINE bool MutableArrayIterator<T>::operator<(const MutableArrayIterator<T>& other) const
    {
        return (other.m_array == m_array) && (other.m_index < m_index);
    }

    template< typename T >
    INLINE const T& MutableArrayIterator<T>::operator->() const
    {
        ASSERT_EX(m_data != nullptr && m_index < m_count, "Accessing element via invalid iterator");
        return m_data->typedData()[m_index];
    }

    template< typename T >
    INLINE const T& MutableArrayIterator<T>::operator*() const
    {
        ASSERT_EX(m_data != nullptr && m_index < m_count, "Accessing element via invalid iterator");
        return m_data->typedData()[m_index];
    }

    template< typename T >
    INLINE void MutableArrayIterator<T>::operator++()
    {
        advance();
    }

    template< typename T >
    INLINE void MutableArrayIterator<T>::operator++(int)
    {
        advance();
    }

    template< typename T >
    void MutableArrayIterator<T>::advance()
    {
        while (m_index < m_count)
        {
            ++m_index;

            if (m_index >= m_count)
                break;

            if (m_data->typedData()[m_index] != T())
                break;
        }
    }

} // base