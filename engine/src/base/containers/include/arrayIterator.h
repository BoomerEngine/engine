/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers\common #]
***/

#pragma once

namespace base
{

    /// Iterator class
    template< typename T >
    class ArrayIterator
    {
    public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = T;
		using difference_type   = ptrdiff_t;
		using pointer           = T*;
		using reference         = T &;

        ArrayIterator()
            : m_ptr(nullptr)
        {}

        ArrayIterator(T* ptr)
            : m_ptr(ptr)
        {}

        ArrayIterator(const ArrayIterator& other)
            : m_ptr(other.m_ptr)
        {}

        //! Move to next elem
        INLINE ArrayIterator<T>& operator++()
        {
            ++m_ptr;
            return *this;
        }

        //! Move to prev elem
        INLINE ArrayIterator<T>& operator--()
        {
            --m_ptr;
            return *this;
        }

        //! Move to next elem
        INLINE ArrayIterator<T> operator++(int)
        {
            ArrayIterator<T> copy(*this);
            ++m_ptr;
            return copy;
        }

        //! Move to prev elem
        INLINE ArrayIterator<T> operator--(int)
        {
            ArrayIterator<T> copy(*this);
            --m_ptr;
            return copy;
        }
    
        //! Get reference to item
        INLINE T& operator*() const
        {
            return *m_ptr;
        }

        //! Get pointer to item
        INLINE T* operator->() const
        {
            return m_ptr;
        }

        //! Difference
        INLINE ptrdiff_t operator-(const ArrayIterator<T>& other) const
        {
            return (m_ptr - other.m_ptr);
        }

        //! Compare relative position with other iterator
        INLINE bool operator==(const ArrayIterator<T>& other) const
        {
            return m_ptr == other.m_ptr;
        }

        //! Compare relative position with other iterator
        INLINE bool operator!=(const ArrayIterator<T>& other) const
        {
            return m_ptr != other.m_ptr;
        }

        //! Compare relative position with other iterator
        INLINE bool operator<(const ArrayIterator<T>& other) const
        {
            return m_ptr < other.m_ptr;
        }

        //! Move forward
        INLINE ArrayIterator<T> operator+(const ptrdiff_t diff) const
        {
            return ArrayIterator<T>(m_ptr + diff);
        }

        //! Move forward
        INLINE ArrayIterator<T>& operator+=(const ptrdiff_t diff)
        {
            m_ptr += diff;
            return *this;
        }

        //! Move backward
        INLINE ArrayIterator<T> operator-(const ptrdiff_t diff) const
        {
            return ArrayIterator<T>(m_ptr - diff);
        }

        //! Move backward
        INLINE ArrayIterator<T>& operator-=(const ptrdiff_t diff)
        {
            m_ptr -= diff;
            return *this;
        }

    private:
        T* m_ptr;
    };

    /// Iterator class
    template< typename T >
    class ConstArrayIterator
    {
    public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = const T;
		using difference_type   = ptrdiff_t;
		using pointer           = const T *;
		using reference         = const T &;

        ConstArrayIterator()
            : m_ptr(nullptr)
        {}

        ConstArrayIterator(const T* ptr)
            : m_ptr(ptr)
        {}

        ConstArrayIterator(const ConstArrayIterator& other)
            : m_ptr(other.m_ptr)
        {}

        ConstArrayIterator(ConstArrayIterator&& other)
            : m_ptr(other.m_ptr)
        {}

        ConstArrayIterator(const ArrayIterator<T>& other)
            : m_ptr(&*other)
        {}

        ConstArrayIterator(ArrayIterator<T>&& other)
            : m_ptr(&*other)
        {}

        //! Move to next elem
        INLINE ConstArrayIterator<T>& operator++()
        {
            ++m_ptr;
            return *this;
        }

        //! Move to next elem
        INLINE ConstArrayIterator<T> operator++(int)
        {
            ConstArrayIterator<T> copy(*this);
            ++m_ptr;
            return copy;
        }

        //! Move to prev elem
        INLINE ConstArrayIterator<T>& operator--()
        {
            --m_ptr;
            return *this;
        }

        //! Move to prev elem
        INLINE ConstArrayIterator<T> operator--(int)
        {
            ConstArrayIterator<T> copy(*this);
            --m_ptr;
            return copy;
        }

        //! Get reference to item
        INLINE const T& operator*() const
        {
            return *m_ptr;
        }

        //! Get pointer to item
        INLINE const T* operator->() const
        {
            return m_ptr;
        }

        //! Compare relative position with other iterator
        INLINE bool operator==(const ConstArrayIterator<T>& other) const
        {
            return m_ptr == other.m_ptr;
        }

        //! Assign
        INLINE ConstArrayIterator<T>& operator=(const ConstArrayIterator<T>& other)
        {
            m_ptr = other.m_ptr;
            return *this;
        }

        //! Compare relative position with other iterator
        INLINE bool operator!=(const ConstArrayIterator<T>& other) const
        {
            return m_ptr != other.m_ptr;
        }

        //! Compare relative position with other iterator
        INLINE bool operator<(const ConstArrayIterator<T>& other) const
        {
            return m_ptr < other.m_ptr;
        }

        //! Difference
        INLINE ptrdiff_t operator-(const ConstArrayIterator<T>& other) const
        {
            return (m_ptr - other.m_ptr);
        }

        //! Move forward
        INLINE ConstArrayIterator<T> operator+(const ptrdiff_t diff) const
        {
            return ConstArrayIterator<T>(m_ptr + diff);
        }

        //! Move forward
        INLINE ConstArrayIterator<T>& operator+=(const ptrdiff_t diff)
        {
            m_ptr += diff;
            return *this;
        }

        //! Move backward
        INLINE ConstArrayIterator<T> operator-(const ptrdiff_t diff) const
        {
            return ConstArrayIterator<T>(m_ptr - diff);
        }

        //! Move backward
        INLINE ConstArrayIterator<T>& operator-=(const ptrdiff_t diff)
        {
            m_ptr -= diff;
            return *this;
        }

        //! Get internal pointer
        INLINE const T* ptr() const
        {
            return m_ptr;
        }

    private:
        const T* m_ptr;
    };


};