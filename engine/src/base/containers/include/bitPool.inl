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
	template< typename Container >
    INLINE BitPool<Container>::BitPool()
    {}

	template< typename Container >
    INLINE void BitPool<Container>::reset()
    {
        m_freeIndices.enableAll();
		m_numAllocated = 0;
        m_searchIndex = 0;
    }

	template< typename Container >
	INLINE uint32_t BitPool<Container>::size() const
	{
		return m_numAllocated;
	}

	template< typename Container >
	INLINE uint32_t BitPool<Container>::capacity() const
	{
		return m_freeIndices.size();
	}

	template< typename Container >
	INLINE void BitPool<Container>::reseve(uint32_t numIds)
	{
		if (numIds > m_freeIndices.size())
			m_freeIndices.resizeWithOnes(numIds);
	}

	template< typename Container >
	INLINE uint32_t BitPool< Container >::allocate()
    {
        // search for a next bit that is set indicating a free place
        auto firstFreeIndex = m_freeIndices.findNextBitSet(m_searchIndex);
        if (firstFreeIndex == m_freeIndices.size())
        {
			// all bits allocated, resize
			if (m_numAllocated == m_freeIndices.size())
			{
				auto nextCapacity = std::max<uint32_t>(1024, m_freeIndices.size() * 2);
				m_freeIndices.resizeWithOnes(nextCapacity);
			}

			// wrap around
			firstFreeIndex = m_freeIndices.findNextBitSet(0);
        }

        // mark the index found as allocated
		ASSERT_EX(m_freeIndices[firstFreeIndex], "Trying to allocate bit that is not free");
		m_freeIndices.clear(firstFreeIndex);
		m_searchIndex = firstFreeIndex + 1;
        m_numAllocated += 1;

        // return allocated free index
        return firstFreeIndex;
    }

	template< typename Container >
	INLINE void BitPool< Container >::release(uint32_t id)
    {
        ASSERT_EX(!m_freeIndices[id], "Trying to free index that is already freed. Possible memory corruption");

        // mark index as freed
        m_freeIndices.set(id);
        m_numAllocated -= 1;

        // extend the search region
        if (id < m_searchIndex)
            m_searchIndex = id;
    }

} // base