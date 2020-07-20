/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "array.h"

namespace base
{

    /// simple FIFO queue
    template<class T, typename Container = Array<T>>
    class Queue
    {
    public:
        INLINE Queue() = default;
        INLINE Queue(const Queue& other) = default;
        INLINE Queue& operator=(const Queue& other) = default;
        INLINE Queue(Queue&& other);
        INLINE Queue& operator=(Queue&& other);

        // empty ?
        INLINE bool empty() const;

        // full ? (will resize container when pushed)
        INLINE bool full() const;

        // number of elements in the queue
        INLINE uint32_t size() const;

        // current capacity of the container
        INLINE uint32_t capacity() const;

        // add to queue
        INLINE void push(const T& element);

        // get top element of the queue (the one that will be popped next)
        INLINE const T& top() const;

        // pop top
        INLINE void pop();

        // clear state
        INLINE void clear();

        // clear state without freeing memory
        INLINE void reset();

        //--

        // debug dump
        INLINE void print(IFormatStream& f) const;

    private:
        Container m_elements;
        uint32_t m_head = 0;
        uint32_t m_tail = 0;
        bool m_full = false; // discussable but I don't want to waste memory in the array itself

        INLINE void advance();
        INLINE void retreat();

        void grow();
    };

} // base

#include "queue.inl"