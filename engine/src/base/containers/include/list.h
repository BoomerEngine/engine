/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers\dynamic #]
***/

#pragma once

namespace base
{

	/// list entry
	struct ListLink
	{
		ListLink* prev = nullptr;
		ListLink* next = nullptr;
	};

	/// intrusive list integration
	template< typename T, uint32_t OFFSET >
	struct ListElemAtOffset
	{
		INLINE static T* GetElementFromLink(ListLink& link)
		{
			return (T*)((uint8_t*)& link - OFFSET);
		}

		INLINE static const T* GetElementFromLink(const ListLink& link)
		{
			return (const T*)((const uint8_t*)& link - OFFSET);
		}

		INLINE static ListLink* GetLinkFromElement(T* elem)
		{
			return elem ? (ListLink*)((uint8_t*)elem + OFFSET) : nullptr;
		}

		INLINE static const ListLink* GetLinkFromElement(const T* elem)
		{
			return elem ? (const ListLink*)((const uint8_t*)elem + OFFSET) : nullptr;
		}
	};

	/// list iterator
	template< typename T, class ListIntegration >
	class ListIterator
	{
	public:
		ListIterator(ListLink* link = nullptr);

		// get if still valid
		operator bool() const;

		// access the object
		T* operator->() const;
		
		// get the object
		T* get() const;

		// go to next object
		ListIterator<T, ListIntegration> operator++();
		ListIterator<T, ListIntegration> operator++(int);

		// go to previous object
		ListIterator<T, ListIntegration> operator--();
		ListIterator<T, ListIntegration> operator--(int);

	private:
		ListLink* m_cur;
	};

	/// simple double linked list
	template< typename T, class ListIntegration = ListElemAtOffset<T, 0> >
	class List : public NoCopy
	{
	public:
		List();
		List(List&& other);
		List(const List& other) = delete;
		List& operator=(List&& other);
		List& operator=(const List& other) = delete;
		~List();

		/// is the list empty ?
		bool empty() const;

		/// reset list without unlinking elements
		void reset();

		/// unlink all elements in a clean way
		void clear();

		/// link element into the list 
		void pushFront(T* elem);

		/// push element to the back of the list 
		void pushBack(T* elem);

		/// unlink element from the list
		void unlink(T* elem);

		//--

		/// get head
		ListIterator<T, ListIntegration> head() const;

		/// get tail
		ListIterator<T, ListIntegration> tail() const;

		//--

		/// get head element
		T* headElement() const;

		/// get tail element
		T* tailElement() const;

	private:
		T* m_head = nullptr;
		T* m_tail = nullptr;
	};
    
} // base

#include "list.inl"