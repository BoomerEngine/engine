/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "array.h"

BEGIN_BOOMER_NAMESPACE()

/// simple FIFO queue
template<class T, typename Container = Array<T>>
class Queue
{
public:
    INLINE Queue() = default;
	INLINE Queue(const Queue& other) = default;
	INLINE Queue& operator=(const Queue& other) = default;
    Queue(Queue&& other);
    Queue& operator=(Queue&& other);

    // empty ?
    bool empty() const;

    // full ? (will resize container when pushed)
    bool full() const;

    // number of elements in the queue
    uint32_t size() const;

    // current capacity of the container
    uint32_t capacity() const;

	// reserve space for the queue
	void reserve(uint32_t size);

    // add to queue
    void push(const T& element);

    // get top element of the queue (the one that will be popped next)
    const T& top() const;

    // pop top
    void pop();

    // clear state
    void clear();

    // clear state without freeing memory
    void reset();

    //--

    // debug dump
    void print(IFormatStream& f) const;

private:
    Container m_elements;
    uint32_t m_head = 0;
    uint32_t m_tail = 0;
    bool m_full = false; // questionable but I don't want to waste memory in the array itself

    void advance();
    void retreat();

    void grow();
};

END_BOOMER_NAMESPACE()

#include "queue.inl"