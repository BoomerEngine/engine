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
    //--

	template< typename T, typename Container >
	INLINE SortedArray<T, Container>::SortedArray()
	{}

	template< typename T, typename Container >
	INLINE SortedArray<T, Container>::SortedArray(const SortedArray<T, Container>& other)
		: m_array(other.m_array)
	{}

	template< typename T, typename Container >
	INLINE SortedArray<T, Container>::SortedArray(SortedArray<T, Container>&& other)
		: m_array(std::move(other.m_array))
	{}

	template< typename T, typename Container >
	INLINE SortedArray<T, Container>& SortedArray<T, Container>::operator=(const SortedArray<T, Container>& other)
	{
		m_array = other.m_array;
		return *this;
	}

	template< typename T, typename Container >
	INLINE SortedArray<T, Container>& SortedArray<T, Container>::operator=(SortedArray<T, Container>&& other)
	{
		m_array = std::move(other.m_array);
		return *this;
	}

    template< typename T, typename Container >
    INLINE SortedArray<T, Container>::SortedArray(const Container& other)
        : m_array(other)
    {
        std::stable_sort(m_array.begin(), m_array.end());
		auto count  = std::unique(m_array.begin(), m_array.end());
		m_array.resize(std::distance(m_array.begin(), count));
    }

    template< typename T, typename Container >
    INLINE SortedArray<T, Container>::SortedArray(Container&& other)
        : m_array(std::move(other))
    {
        std::stable_sort(m_array.begin(), m_array.end());
		auto count  = std::unique(m_array.begin(), m_array.end());
		m_array.resize(std::distance(m_array.begin(), count));
    }

	template< typename T, typename Container >
	INLINE SortedArray<T, Container>::SortedArray(const T* data, uint32_t count)
		: m_array(data, count)
    {
		std::stable_sort(m_array.begin(), m_array.end());
		auto finalCount  = std::unique(m_array.begin(), m_array.end());
		m_array.resize(std::distance(m_array.begin(), finalCount));
    }

    template< typename T, typename Container >
    INLINE uint32_t SortedArray<T, Container>::size() const
    {
        return m_array.size();
    }

    template< typename T, typename Container >
    INLINE uint32_t SortedArray<T, Container>::capacity() const
    {
        return m_array.capacity();
    }

    template< typename T, typename Container >
    INLINE bool SortedArray<T, Container>::empty() const
    {
        return m_array.empty();
    }

    template< typename T, typename Container >
    INLINE const Container& SortedArray<T, Container>::elements() const
    {
        return m_array;
    }

    template< typename T, typename Container >
    INLINE void SortedArray<T, Container>::popBack()
    {
        m_array.popBack();
    }

	template< typename T, typename Container >
	INLINE const T& SortedArray<T, Container>::back() const
	{
		return m_array.back();
	}

	template< typename T, typename Container >
	INLINE const T& SortedArray<T, Container>::front() const
	{
		return m_array.front();
	}

    template< typename T, typename Container >
    INLINE void SortedArray<T, Container>::reset()
    {
        m_array.reset();
    }

    template< typename T, typename Container >    
    INLINE void SortedArray<T, Container>::reserve(uint32_t newSize)
    {
        m_array.reserve(newSize);
    }

    template< typename T, typename Container >    
    INLINE bool SortedArray<T, Container>::contains(const T &element) const
    {
		auto it  = std::lower_bound(begin(), end(), element);
		return it != end();
    }

    template< typename T, typename Container >    
    INLINE int SortedArray<T, Container>::find(const T &element) const
    {
		auto it  = std::lower_bound(begin(), end(), element);
		return (it != end()) ? std::distance(begin(), it) : INDEX_NONE;
    }

    template< typename T, typename Container >    
    INLINE void SortedArray<T, Container>::erase(uint32_t index, uint32_t count /*= 1*/)
    {
        m_array.erase(index, count);
    }

    template< typename T, typename Container >    
    INLINE void SortedArray<T, Container>::clear()
    {
        m_array.clear();
    }

    template< typename T, typename Container >    
    INLINE bool SortedArray<T, Container>::insert(const T &item)
    {
        auto it  = std::lower_bound(m_array.begin(), m_array.end(), item);
		if (it == m_array.end())
		{
			m_array.pushBack(item);
			return true;
		}

		if (*it == item)
			return false;
    	
        m_array.insert(std::distance(m_array.begin(), it), item);
		return true;
    }

    template< typename T, typename Container >
    INLINE bool SortedArray<T, Container>::remove(const T &item)
    {
        auto it  = std::lower_bound(m_array.begin(), m_array.end(), item);
        if (it != m_array.end())
        {
			if (*it == item)
			{
				m_array.erase(std::distance(m_array.begin(), it));
				return true;
			}
        }

        return false;
    }

	template< typename T, typename Container >
	ConstArrayIterator<T> SortedArray<T, Container>::begin() const
    {
		return m_array.begin();
    }

	template< typename T, typename Container >
	ConstArrayIterator<T> SortedArray<T, Container>::end() const
    {
		return m_array.end();
    }

	//! access operator
	template< typename T, typename Container >
	INLINE const T& SortedArray<T, Container>::operator[](uint32_t index) const
    {
		return m_array[index];	    
    }

    //--

} // base