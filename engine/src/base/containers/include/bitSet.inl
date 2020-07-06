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
	INLINE BitSet<Container>::BitSet()
	{}

	template< typename Container >
	INLINE BitSet<Container>::BitSet(const BitSet<Container>& other)
		: m_words(other.m_words)
		, m_bitSize(other.m_bitSize)
	{}

	template< typename Container >
	INLINE BitSet<Container>::BitSet(BitSet<Container>&& other)
		: m_words(std::move(other.m_words))
		, m_bitSize(other.m_bitSize)
	{
		other.m_bitSize = 0;
	}

	template< typename Container >
	INLINE BitSet<Container>::BitSet(uint32_t bitCount, EBitStateZero)
	{
		resizeWithZeros(bitCount);
	}

	template< typename Container >
	INLINE BitSet<Container>::BitSet(uint32_t bitCount, EBitStateOne)
	{
		resizeWithOnes(bitCount);
	}

	template< typename Container >
	INLINE BitSet<Container>::BitSet(const Container& other, uint32_t maxBitCount)
		: m_words(other.m_words)
		, m_bitSize(std::min(other.m_bitSize, maxBitCount))
	{
	}

	template< typename Container >
	INLINE BitSet<Container>::BitSet(Container&& other, uint32_t maxBitCount)
		: m_words(std::move(other.m_words))
	{
		m_bitSize = std::min(other.m_bitSize, maxBitCount);
		other.m_bitSize = 0;
	}

	template< typename Container >
	INLINE BitSet<Container>::BitSet(const void* data, uint32_t maxBitCount)
		: m_words(data, (maxBitCount + NUM_BITS_IN_WORD - 1) / NUM_BITS_IN_WORD)
		, m_bitSize(maxBitCount)
	{}

	template< typename Container >
	INLINE BitSet<Container>& BitSet<Container>::operator=(const BitSet<Container>& other)
	{
		if (this != &other)
		{
			m_words = other.m_words;
			m_bitSize = other.m_bitSize;
		}
		return *this;
	}

	template< typename Container >
	INLINE BitSet<Container>& BitSet<Container>::operator=(BitSet<Container>&& other)
	{
		if (this != &other)
		{
			m_words = std::move(m_words);
			m_bitSize = other.m_bitSize;
			other.m_bitSize = 0;
		}
		return *this;
	}

	template< typename Container >
	INLINE uint32_t BitSet<Container>::size() const
	{
		return m_bitSize;
	}

	template< typename Container >
	INLINE int BitSet<Container>::lastValidIndex() const
	{
		return (int)m_bitSize - 1;
	}

	template< typename Container >
	INLINE bool BitSet<Container>::empty() const
	{
		return (0 == m_bitSize);
	}

	template< typename Container >
	INLINE uint32_t BitSet<Container>::capcity() const
	{
		return m_words.capacity() * NUM_BITS_IN_WORD;
	}

	template< typename Container >
	INLINE void* BitSet<Container>::data()
	{
		return m_words.data();
	}

	template< typename Container >
	INLINE const void* BitSet<Container>::data() const
	{
		return m_words.data();
	}

	template< typename Container >
	INLINE BitWord* BitSet<Container>::typedData()
	{
		return m_words.typedData();
	}

	template< typename Container >
	INLINE const BitWord* BitSet<Container>::typedData() const
	{
		return m_words.typedData();
	}

	template< typename Container >
	INLINE void BitSet<Container>::reset()
	{
		m_bitSize = 0;
	}

	template< typename Container >
	INLINE void BitSet<Container>::clear()
	{
		m_words.clear();
		m_bitSize = 0;
	}

	template< typename Container >
	INLINE void BitSet<Container>::resizeWithZeros(uint32_t newBitCount)
	{
		if (newBitCount > m_bitSize)
		{
			auto numWords  = (newBitCount + NUM_BITS_IN_WORD - 1) / NUM_BITS_IN_WORD;
			m_words.resize(numWords);
			ClearBits(data(), m_bitSize, newBitCount - m_bitSize);
		}

		m_bitSize = newBitCount;
	}

	template< typename Container >
	INLINE void BitSet<Container>::resizeWithOnes(uint32_t newBitCount)
	{
		if (newBitCount > m_bitSize)
		{
			auto numWords  = (newBitCount + NUM_BITS_IN_WORD - 1) / NUM_BITS_IN_WORD;
			m_words.resize(numWords);
			SetBits(data(), m_bitSize, newBitCount - m_bitSize);
		}

		m_bitSize = newBitCount;
	}

	template< typename Container >
	INLINE void BitSet<Container>::clearRange(uint32_t index, uint32_t count)
	{
		ASSERT(index + count <= m_bitSize);
		ClearBits(data(), index, count);
	}

	template< typename Container >
	INLINE void BitSet<Container>::clearAll()
	{
		ClearBits(data(), 0, m_bitSize);
	}

	template< typename Container >
	INLINE void BitSet<Container>::range(uint32_t index, uint32_t count)
	{
		ASSERT(index + count <= m_bitSize);
		SetBits(data(), index, count);
	}

	template< typename Container >
	INLINE void BitSet<Container>::enableAll()
	{
		SetBits(data(), 0, m_bitSize);
	}

	template< typename Container >
	INLINE bool BitSet<Container>::operator[](const uint32_t bit) const
	{
		auto bitMask  = 1u << (bit & 31);
		return 0 != (m_words[bit / 32] & bitMask);
	}

	template< typename Container >
	INLINE bool BitSet<Container>::set(uint32_t bit)
	{
		auto bitMask  = 1u << (bit & 31);
		auto& word = m_words[bit / 32];
		auto oldValue  = 0 != (word & bitMask);
		word |= bitMask;
		return oldValue;
	}

	template< typename Container >
	INLINE bool BitSet<Container>::clear(uint32_t bit)
	{
		auto bitMask  = 1u << (bit & 31);
		auto& word = m_words[bit / 32];
		auto oldValue  = 0 != (word & bitMask);
		word &= ~bitMask;
		return oldValue;
	}

	template< typename Container >
	INLINE BitSet<Container>& BitSet<Container>::operator|=(const BitSet<Container>& otherMask)
	{
		OrBits(m_words.data(), otherMask.m_words.data(), 0, std::min(m_bitSize, otherMask.m_bitSize));
		return *this;
	}

	template< typename Container >
	INLINE BitSet<Container>& BitSet<Container>::operator&=(const BitSet<Container>& otherMask)
	{
		AndBits(m_words.data(), otherMask.m_words.data(), 0, std::min(m_bitSize, otherMask.m_bitSize));
		return *this;
	}

	template< typename Container >
	INLINE BitSet<Container>& BitSet<Container>::operator^=(const BitSet<Container>& otherMask)
	{
		XorBits(m_words.data(), otherMask.m_words.data(), 0, std::min(m_bitSize, otherMask.m_bitSize));
		return *this;
	}

	template< typename Container >
	INLINE BitSet<Container>& BitSet<Container>::operator-=(const BitSet<Container>& otherMask)
	{
		ClearBits(m_words.data(), otherMask.m_words.data(), 0, std::min(m_bitSize, otherMask.m_bitSize));
		return *this;
	}

	template< typename Container >
	INLINE bool BitSet<Container>::isAllSet(const BitSet<Container>& otherMask) const
	{
		return AllBitsSet(m_words.data(), otherMask.m_words.data(), 0, std::min(m_bitSize, otherMask.m_bitSize));
	}

	template< typename Container >
	INLINE bool BitSet<Container>::isAnySet(const BitSet<Container>& otherMask) const
	{
		return AnyBitsSet(m_words.data(), otherMask.m_words.data(), 0, std::min(m_bitSize, other.m_bitSize));
	}

	template< typename Container >
	INLINE bool BitSet<Container>::isNoneSet(const BitSet<Container>& otherMask) const
	{
		return !isAnySet(otherMask);
	}

	template< typename Container >
	INLINE bool BitSet<Container>::isAllSet() const
	{
		return AllBitsSet(m_words, 0, m_bitSize);
	}

	template< typename Container >
	INLINE bool BitSet<Container>::isAnySet() const
	{
		return AnyBitsSet(m_words, 0, m_bitSize);
	}

	template< typename Container >
	INLINE bool BitSet<Container>::isNoneSet() const
	{
		return !AnyBitsSet(m_words, 0, m_bitSize);
	}

	template< typename Container >
	INLINE bool BitSet<Container>::operator==(const BitSet<Container>& other) const
	{
		if (m_bitSize != other.m_bitSize)
			return false;
		return CompareBits(m_words.data(), other.m_words.data(), 0, m_bitSize);
	}

	template< typename Container >
	INLINE bool BitSet<Container>::operator!=(const BitSet<Container>& other) const
	{
		return !operator==(other);
	}

	//--

    template< typename Container >
	INLINE bool BitSet<Container>::iterateSetBits(const std::function<bool(uint32_t index)>& enumFunc) const
	{
		auto* ptr = m_words.typedData();
		auto* ptrEnd = m_words.typedData() + m_words.size();
		uint32_t index = 0;
        while (ptr < ptrEnd)
        {
            auto mask = *ptr;
            while (mask)
            {
                uint32_t localIndex = __builtin_ctzll(mask) + index;
                if (enumFunc(localIndex))
                    return true;

                mask ^= mask & -mask;
            }

            index += 64;
            ptr += 1;
        }

		return false;
	}

    template< typename Container >
	INLINE bool BitSet<Container>::iterateClearBits(const std::function<bool(uint32_t index)>& enumFunc) const
	{
        auto* ptr = m_words.typedData();
        auto* ptrEnd = m_words.typedData() + m_words.size();
        uint32_t index = 0;
        while (ptr < ptrEnd)
        {
            auto mask = ~*ptr;
            while (mask)
            {
                uint32_t localIndex = __builtin_ctzll(mask) + index;
                if (enumFunc(localIndex))
                    return true;

                mask ^= mask & -mask;
            }

            index += 64;
            ptr += 1;
        }

        return false;
	}

	//--

	template < uint32_t InitialSize >
	INLINE InplaceBitSet<InitialSize>::InplaceBitSet(EBitStateZero)
	{
		resizeWithZeros(capcity());
	}

	template < uint32_t InitialSize >
	INLINE InplaceBitSet<InitialSize>::InplaceBitSet(EBitStateOne)
	{
		resizeWithOnes(capcity());
	}

} // base
