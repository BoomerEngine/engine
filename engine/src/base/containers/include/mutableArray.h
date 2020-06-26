/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/containers/include/array.h"

namespace base
{
    template<typename T>
    class MutableArrayIterator;

    // mutable array - array that can have elements SAFELY added/removed during iteration
    // NOTE: Main purpose of this container is a list of callback functions in which a callback function can add/remove other callbacks
    // NOTE: the mutable array is not copiable by design as it's intended for runtime types mostly
    // NOTE: array is thread safe by default (no exceptions)
    // TODO: add "policy" for the empty elements
    template< typename T >
    class MutableArray : public base::NoCopy
    {
    public:
        INLINE MutableArray();
        INLINE ~MutableArray();

        /// is this array empty
        INLINE bool empty() const;

        /// is the array being iterated ?
        /// NOTE: stuff can still be added and removed but not compacted
        INLINE bool locked() const;

        /// add element to the array
        /// NOTE: if array is being iterated over the element will NOT be visited by current iteration
        INLINE void pushBack(const T& data);

        /// remove element from the array
        /// NOTE: safe to call during iteration (the whole point of this class...)
        INLINE void remove(const T& data, const T& empty = T());

        /// clear all elements
        /// NOTE: we cannot be locked
        INLINE void clear();

        /// lock callbacks for iteration, create an iterator
        INLINE MutableArrayIterator<T> begin() const;

        /// iterator for the "last" element
        INLINE MutableArrayIterator<T> end() const;

    private:
        friend class MutableArrayIterator<T>;

        mutable Array<T> m_values;
        mutable Mutex m_lock;
        mutable volatile uint32_t m_lockCount;
        volatile uint32_t m_numRemoved;

        void lock() const;
        void unlock() const;
    };

    // iterator of callback list
    // NOTE: we don't have full compatibility with std::iterator
    template< typename T >
    class MutableArrayIterator
    {
    public:
        INLINE MutableArrayIterator(const MutableArray<T>& ar);
        INLINE MutableArrayIterator(const MutableArray<T>& ar, const int); // end of array
        INLINE ~MutableArrayIterator();

        INLINE MutableArrayIterator(const MutableArrayIterator<T>& other);
        INLINE MutableArrayIterator(MutableArrayIterator<T>&& other);

        INLINE MutableArrayIterator& operator=(const MutableArrayIterator<T>& other);
        INLINE MutableArrayIterator& operator=(MutableArrayIterator<T>&& other);

        INLINE bool operator==(const MutableArrayIterator<T>& other) const;
        INLINE bool operator!=(const MutableArrayIterator<T>& other) const;
        INLINE bool operator<(const MutableArrayIterator<T>& other) const;

        INLINE const T& operator->() const;
        INLINE const T& operator*() const;

        INLINE void operator++();
        INLINE void operator++(int);

    private:
        const Array<T>* m_data; // elements, cached
        int m_count; // captured at creation time so we don't it go over added elements
        int m_index; // current position in the iterator
        bool m_locked; // did we lock the array

        const MutableArray<T>* m_array;

        void advance();
    };
} // base

#include "mutableArray.inl"