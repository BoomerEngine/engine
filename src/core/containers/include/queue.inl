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

//--

template<class T, typename Container>
INLINE Queue<T, Container>::Queue(Queue&& other)
    : m_elements(std::move(other.m_elements))
    , m_head(other.m_head)
    , m_tail(other.m_tail)
    , m_full(other.m_full)
{
    other.m_head = 0;
    other.m_tail = 0;
    other.m_full = false;
}

template<class T, typename Container>
INLINE Queue<T, Container>& Queue<T, Container>::operator=(Queue&& other)
{
    if (this != &other)
    {
        m_elements = std::move(other.m_elements);
        m_head = other.m_head;
        m_tail = other.m_tail;
        m_full = other.m_full;
        other.m_full = false;
        other.m_head = 0;
        other.m_tail = 0;
    }

    return *this;
}

template<class T, typename Container>
INLINE bool Queue<T, Container>::empty() const
{
    return !m_full && (m_head == m_tail);
}

template<class T, typename Container>
INLINE bool Queue<T, Container>::full() const
{
    return m_full;
}

template<class T, typename Container>
INLINE uint32_t Queue<T, Container>::capacity() const
{
    return m_elements.size(); // yes, we don't have to consume the full array, in principle
}

template<class T, typename Container>
void Queue<T, Container>::reserve(uint32_t size)
{
	m_elements.reserve(size);
}

template<class T, typename Container>
INLINE uint32_t Queue<T, Container>::size() const
{
    auto size = m_elements.size();

    if (!m_full)
    {
        if (m_head >= m_tail)
            size = m_head - m_tail;
        else
            size = size + m_head - m_tail;
    }

    return size;
}

template<class T, typename Container>
INLINE void Queue<T, Container>::advance()
{
    if (m_full)
        m_tail = (m_tail + 1) % m_elements.size();

    m_head = (m_head + 1) % m_elements.size();
    m_full = (m_head == m_tail);
}

template<class T, typename Container>
INLINE void Queue<T, Container>::retreat()
{
    m_full = false;
    m_tail = (m_tail + 1) % m_elements.size();
}

template<class T, typename Container>
INLINE void Queue<T, Container>::push(const T& element)
{
    if (full() || m_elements.empty())
        grow();

    m_elements[m_head] = element;
    advance();
}

template<class T, typename Container>
INLINE const T& Queue<T, Container>::top() const
{
    DEBUG_CHECK(!empty());
    return m_elements[m_tail];
}

template<class T, typename Container>
INLINE void Queue<T, Container>::pop()
{
    DEBUG_CHECK(!empty());
    m_elements[m_tail] = T();
    retreat();
}

template<class T, typename Container>
INLINE void Queue<T, Container>::clear()
{
    m_elements.clear();
    m_head = 0;
    m_tail = 0;
    m_full = false;
}

template<class T, typename Container>
INLINE void Queue<T, Container>::reset()
{
    m_elements.reset();
    m_head = 0;
    m_tail = 0;
    m_full = false;
}

template<class T, typename Container>
INLINE void Queue<T, Container>::grow()
{
	auto oldCapacity = m_elements.size();
	auto oldSize = size();

	auto reqCapacity = std::max<uint32_t>(16, m_elements.size() * 2); // we need to have at least twice the element count
    m_elements.resize(reqCapacity);

	m_full = false;

	if (oldSize && m_head <= m_tail)
	{
		for (uint32_t i = 0; i < m_head; ++i)
			std::swap(m_elements[i], m_elements[oldCapacity + i]);
		m_head += oldCapacity;

		ASSERT(size() == oldSize);
		ASSERT(m_tail < m_head);
	}
}

//--

END_BOOMER_NAMESPACE()
