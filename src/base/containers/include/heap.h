/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

/// heap, implemented on top of array
template <typename T, typename Container=Array<T>, typename Compare = std::less<T>>
class Heap
{
public:
    Heap() = default;
    Heap(Container&& baseData); // allow to construct heap from un-sorted data
    Heap(const Container& baseData); // allow to construct heap from un-sorted data
	Heap(const T* data, uint32_t size); // allow to construct heap from un-sorted data
    Heap(const Heap &other) = default;
    Heap(Heap &&other) = default;
    Heap& operator=(const Heap &other) = default;
    Heap& operator=(Heap &&other) = default;

    //! true is queue is empty
    bool empty() const;

    //! get number of elements in the heap
    uint32_t size() const;

    //! clear the heap
    void clear();

    //! get head item (read only)
    const T &front() const;

    //! insert element to heap
    void push(const T& data);

    //! remove the top element from the heap
    void pop();

    //--

    //! get all values for iteration
    const Array<T>& values() const;

	//--

	//! get read only iterator to start of the array
	ConstArrayIterator<T> begin() const;

	//! get read only iterator to end of the array
	ConstArrayIterator<T> end() const;

private:
    Container m_values;
};

END_BOOMER_NAMESPACE(base)

#include "heap.inl"