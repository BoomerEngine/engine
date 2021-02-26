/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

template<typename T, class ListIntegration>
INLINE ListIterator<T, ListIntegration>::ListIterator(ListLink* cur)
	: m_cur(cur)
{}

template<typename T, class ListIntegration>
INLINE ListIterator<T, ListIntegration>::operator bool() const
{
	return (m_cur);
}

template<typename T, class ListIntegration>
INLINE T* ListIterator<T, ListIntegration>::operator->() const
{
	ASSERT_EX(m_cur, "Accessing NULL list iterator");
	return ListIntegration::GetElementFromLink(*m_cur);
}
		
template<typename T, class ListIntegration>
INLINE T* ListIterator<T, ListIntegration>::get() const
{
	return m_cur ? ListIntegration::GetElementFromLink(*m_cur) : nullptr;
}

template<typename T, class ListIntegration>
INLINE ListIterator<T, ListIntegration> ListIterator<T, ListIntegration>::operator++()
{
	ASSERT_EX(m_cur, "Accessing null iterator");
	m_cur = m_cur->next;
	return *this;
}

template<typename T, class ListIntegration>
INLINE ListIterator<T, ListIntegration> ListIterator<T, ListIntegration>::operator++(int)
{
	ASSERT_EX(m_cur, "Accessing null iterator");
	auto ret  = *this;
	m_cur = m_cur->next;
	return ret;
}

template<typename T, class ListIntegration>
INLINE ListIterator<T, ListIntegration> ListIterator<T, ListIntegration>::operator--()
{
	ASSERT_EX(m_cur, "Accessing null iterator");
	m_cur = m_cur->prev;
	return *this;
}

template<typename T, class ListIntegration>
INLINE ListIterator<T, ListIntegration> ListIterator<T, ListIntegration>::operator--(int)
{
	ASSERT_EX(m_cur, "Accessing null iterator");
	auto ret  = *this;
	m_cur = m_cur->prev;
	return ret;
}

//--

/// simple double linked list
template<typename T, class ListIntegration>
INLINE List<T, ListIntegration>::List()
{}

template<typename T, class ListIntegration>
INLINE List<T, ListIntegration>::List(List&& other)
	: m_head(other.m_head)
	, m_tail(other.m_tail)
{
	other.m_head = nullptr;
	other.m_tail = nullptr;
}

template<typename T, class ListIntegration>
INLINE List<T, ListIntegration>& List<T, ListIntegration>::operator=(List&& other)
{
	if (this != &other)
	{
		m_head = nullptr;
		m_tail = nullptr;
		other.m_head = nullptr;
		other.m_tail = nullptr;
	}
	return *this;
}

template<typename T, class ListIntegration>
INLINE List<T, ListIntegration>::~List()
{
	DEBUG_CHECK_EX(m_head == nullptr && m_tail == nullptr, "List still contains elements");
}

template<typename T, class ListIntegration>
INLINE bool List<T, ListIntegration>::empty() const
{
	return (m_head == nullptr) && (m_tail == nullptr);
}

template<typename T, class ListIntegration>
INLINE void List<T, ListIntegration>::clear()
{
	auto cur  = m_head;
    ListLink* prevLink = nullptr;
	while (cur)
	{
		auto link  = ListIntegration::GetLinkFromElement(cur);
		ASSERT(link->m_prev == prevLink);
		ASSERT(link->m_next != nullptr || cur == m_tail);
		auto nextLink  = link->m_next;
		link->m_next = nullptr;
		link->m_prev = nullptr;
		prevLink = link;
		link = nextLink;
	}

	m_head = nullptr;
	m_tail = nullptr;
}

template<typename T, class ListIntegration>
INLINE void List<T, ListIntegration>::reset()
{
	m_head = nullptr;
	m_tail = nullptr;
}

template<typename T, class ListIntegration>
INLINE void List<T, ListIntegration>::pushFront(T* elem)
{
	auto link  = ListIntegration::GetLinkFromElement(cur);
	ASSERT_EX(link->m_next == nullptr && link->m_prev == nullptr, "Element is already linked");

	if (m_head == nullptr)
	{
		m_head = elem;
		m_tail = elem;
	}
	else
	{
		auto headLink  = ListIntegration::GetLinkFromElement(m_head);
		ASSERT(headLink->m_prev == nullptr);
		headLink->m_prev = link;
		m_head = elem;
	}
}

template<typename T, class ListIntegration>
INLINE void List<T, ListIntegration>::pushBack(T* elem)
{
	auto link  = ListIntegration::GetLinkFromElement(elem);
	ASSERT_EX(link->m_next == nullptr && link->m_prev == nullptr, "Element is already linked");

	if (m_head == nullptr)
	{
		m_head = elem;
		m_tail = elem;
	}
	else
	{
		auto tailLink  = ListIntegration::GetLinkFromElement(m_tail);
		ASSERT(tailLink->m_next == nullptr);
		tailLink->m_next = link;
		link->m_prev = tailLink;
		m_tail = elem;
	}
}

template<typename T, class ListIntegration>
INLINE void List<T, ListIntegration>::unlink(T* elem)
{
	auto link  = ListIntegration::GetLinkFromElement(elem);

	if (elem == m_head)
	{
		if (link->m_next)
			m_head = ListIntegration::GetElementFromLink(*link->m_next);
		else
			m_head = nullptr;
	}
	else
	{
		ASSERT_EX(link->m_prev != nullptr, "Element is not part of the list");
	}

	if (elem == m_tail)
	{
		if (link->m_prev)
			m_tail = ListIntegration::GetElementFromLink(*link->m_prev);
		else
			m_tail = nullptr;
	}
	else
	{
		ASSERT_EX(link->m_next != nullptr, "Element is not part of the list");
	}

	if (link->m_prev)
		link->m_prev->m_next = link->m_next;
	if (link->m_next)
		link->m_next->m_prev = link->m_prev;

	link->m_prev = nullptr;
	link->m_next = nullptr;
}

template<typename T, class ListIntegration>
INLINE ListIterator<T, ListIntegration> List<T, ListIntegration>::head() const
{
	return m_head ? ListIterator<T, ListIntegration>(ListIntegration::GetLinkFromElement(m_head)) : nullptr;
}

template<typename T, class ListIntegration>
INLINE ListIterator<T, ListIntegration> List<T, ListIntegration>::tail() const
{
	return m_tail ? ListIterator<T, ListIntegration>(ListIntegration::GetLinkFromElement(m_tail)) : nullptr;
}

template<typename T, class ListIntegration>
INLINE T* List<T, ListIntegration>::headElement() const
{
	return m_head;
}

template<typename T, class ListIntegration>
INLINE T* List<T, ListIntegration>::tailElement() const
{
	return m_tail;
}

//--

END_BOOMER_NAMESPACE()
