/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "arrayIterator.h"
#include "array.h"

namespace base
{

    /// sorted array on top of normal array
    template< typename T, typename Container=Array<T> >
    class SortedArray
    {
    public:
        INLINE SortedArray();
        INLINE SortedArray(const SortedArray<T, Container>& other);
        INLINE SortedArray(SortedArray<T, Container>&& other);
        INLINE SortedArray<T, Container>& operator=(const SortedArray<T, Container>& other);
        INLINE SortedArray<T, Container>& operator=(SortedArray<T, Container>&& other);
        INLINE SortedArray(const Container& other); // sorts the array
        INLINE SortedArray(Container&& other); // sorts the array
		INLINE SortedArray(const T* data, uint32_t count); // sorts the array

        //! is it empty
        INLINE bool empty() const;

        //! number of elements
        INLINE uint32_t size() const;

        //! capacity
        INLINE uint32_t capacity() const;

        //! access operator
        INLINE const T& operator[](uint32_t index) const;

        //! remove that last element in array
        INLINE void popBack();

        //! get the last element from the array
        INLINE const T &back() const;

        //! get the first element from the array
        INLINE const T &front() const;

        //! clear array
        INLINE void clear();

        //! reset size without freeing memory
        INLINE void reset();

        //! reserve space for given number of array element
        INLINE void reserve(uint32_t newSize);

        //! returns true if array contains given item, faster than on normal array
        INLINE bool contains(const T &element) const;

        //! find element in array and return it's index, faster than on normal array
        INLINE int find(const T &element) const;

        //! remove element(s) from array
        INLINE void erase(uint32_t index, uint32_t count = 1);

        //! add single element to array at the proper place to keep the array sorted
        //! returns true if element was added, false if it already exists
        INLINE bool insert(const T &item);

        //! remove element from array
        //! returns true if element was removed, false if it did not exist
        INLINE bool remove(const T &item);

        //--

        //! get the array of elements
        INLINE const Container& elements() const;

    	//--

		//! get read only iterator to start of the array
		ConstArrayIterator<T> begin() const;

		//! get read only iterator to end of the array
		ConstArrayIterator<T> end() const;

    private:
        Container m_array;
    };

} // base

#include "sortedArray.inl"