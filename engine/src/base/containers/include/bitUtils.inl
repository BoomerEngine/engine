/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once


#ifdef PLATFORM_MSVC
static INLINE uint32_t  __builtin_ctzll(uint64_t val)
{
    unsigned long ret = 0;
    _BitScanForward64(&ret, val);
    return ret;
}
#endif

namespace base
{

	//--

	namespace helper
	{
#if defined(PLATFORM_32BIT)
		static INLINE uint32_t BitScanForward(uint32_t bits)
		{
#ifdef PLATFORM_MSVC
			if (bits != 0)
			{
				uint32_t theIndex = 0;
				::_BitScanForward((unsigned long*)& theIndex, bits);
				return theIndex;
			}
			else
			{
				return 0;
			}
#else
			return mask == 0 ? 0 : __builtin_ctz(mask);
#endif
		}
#elif defined(PLATFORM_64BIT)
		static INLINE uint32_t BitScanForward(uint64_t bits)
		{
#ifdef PLATFORM_MSVC
			if (bits != 0)
			{
				uint32_t theIndex = 0;
				::_BitScanForward64((unsigned long*)& theIndex, bits);
				return theIndex;
			}
			else
			{
				return 0;
			}
#else
			return bits == 0 ? 0 : (uint32_t)__builtin_ctzll(bits);
#endif
		}
#endif

		struct BitTable
		{
			static const uint32_t NUM_BITS_IN_WORD = sizeof(BitWord) * 8;
			static const uint32_t BIT_WORD_SHIFT = 5;
			static const uint32_t BIT_INDEX_MASK = (1U << BIT_WORD_SHIFT) - 1;
			static const BitWord ALL_ONES = ~(BitWord)0;
			static const BitWord ALL_ZEROS = (BitWord)0;
		};
	}

	//--

	INLINE BitWord WordMask(uint32_t count)
	{
		return (count < helper::BitTable::NUM_BITS_IN_WORD) ? ((1U << count) - 1) : helper::BitTable::ALL_ONES;
	}

	INLINE bool TestBit(const void* bits, uint32_t index)
	{
		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;
		auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
		return 0 != (((const BitWord*)bits)[wordIndex] & (1U << bitIndex));
	}

	INLINE bool ClearBit(const void* bits, uint32_t index)
	{
		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;
		auto bitMask = (BitWord)1 << (index & helper::BitTable::BIT_INDEX_MASK);

		auto& word = ((BitWord*)bits)[wordIndex];
		auto oldValue = 0 != (word & bitMask);
		word &= ~bitMask;
		return oldValue;
	}

	INLINE bool SetBit(const void* bits, uint32_t index)
	{
		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;
		auto bitMask = (BitWord)1 << (index & helper::BitTable::BIT_INDEX_MASK);

		auto& word = ((BitWord*)bits)[wordIndex];
		auto oldValue = 0 != (word & bitMask);
		word |= bitMask;
		return oldValue;
	}

	INLINE int GetBitSetIndex(const void* bits, uint32_t maxBits)
	{
		auto words  = (const BitWord*) bits;
		auto maxWord = (maxBits + (helper::BitTable::NUM_BITS_IN_WORD - 1)) / helper::BitTable::NUM_BITS_IN_WORD;

		uint32_t wordBitIndex = 0;
		for (uint32_t i = 0; i < maxWord; ++i, wordBitIndex += helper::BitTable::NUM_BITS_IN_WORD)
		{
			if (words[i] != 0)
			{
				auto bitSetIndex = helper::BitScanForward(words[i]);
				return wordBitIndex + bitSetIndex;
			}
		}

		return INDEX_NONE;
	}

	INLINE uint32_t FindNextBitSet(const void* bits, uint32_t maxBits, uint32_t index)
	{
		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;
		auto words  = (const BitWord*)bits;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;

			auto bits = words[wordIndex] >> bitIndex;
			if (bits != 0)
			{
				index += helper::BitScanForward(bits);
				break;
			}

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
		}

		return index;
	}

	INLINE uint32_t FindNextBitCleared(const void* bits, uint32_t maxBits, uint32_t index)
	{
		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;

		auto words  = (const BitWord*)bits;
		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;

			auto bits = (~words[wordIndex]) >> bitIndex; // inverse
			if (bits != 0)
			{
				index += helper::BitScanForward(bits);
				break;
			}

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT; 
		}

		return index;
	}

    INLINE bool CompareBits(const void* bitsA, const void* bitsB, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto wordsA  = (const BitWord*)bitsA;
		auto wordsB  = (const BitWord*)bitsB;
		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto wordMask = WordMask(count);

			auto wordA = (wordsA[wordIndex] >> bitIndex) & wordMask;
			auto wordB = (wordsB[wordIndex] >> bitIndex) & wordMask;
			if (wordA != wordB)
				break;

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}

		return index >= maxBits;
	}

	INLINE bool AllBitsSet(const void* bits, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto words  = (const BitWord*)bits;
		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto wordMask = WordMask(count);

			auto word = (words[wordIndex] >> bitIndex) & wordMask;
			if (word != wordMask)
				break;

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}

		return index >= maxBits;
	}

	INLINE bool AnyBitsSet(const void* bits, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto words  = (const BitWord*)bits;
		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto wordMask = WordMask(count);

			auto word = (words[wordIndex] >> bitIndex) & wordMask;
			if (word != 0)
				break;

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}

		return index < maxBits;
	}

	INLINE bool AllBitsSet(const void* bits, const void* maskBits, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto words  = (const BitWord*)bits;
		auto maskWords  = (const BitWord*)maskBits;
		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto bitMask = WordMask(count) << bitIndex;

			auto word = words[wordIndex] & maskWords[wordIndex];
			if ((word & bitMask) != bitMask)
				break;

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}

		return index >= maxBits;
	}

	INLINE bool AnyBitsSet(const void* bits, const void* maskBits, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto words  = (const BitWord*)bits;
		auto maskWords  = (const BitWord*)maskBits;
		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto bitMask = WordMask(count) << bitIndex;

			auto word = words[wordIndex] & maskWords[wordIndex];
			if ((word & bitMask) != 0)
				break;

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}

		return index < maxBits;
	}

	INLINE void SetBits(void* bits, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;
		auto words  = (BitWord*)bits;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto bitMask = WordMask(count) << bitIndex;

			words[wordIndex] |= bitMask;

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}
	}

	INLINE void OrBits(void* bits, const void* maskBits, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;
		auto words  = (BitWord*)bits;
		auto maskWords  = (const BitWord*)maskBits;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto bitMask = WordMask(count) << bitIndex;

			words[wordIndex] |= (maskWords[wordIndex] & bitMask);

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}
	}

	INLINE void AndBits(void* bits, const void* maskBits, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;
		auto words  = (BitWord*)bits;
		auto maskWords  = (const BitWord*)maskBits;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto bitMask = WordMask(count) << bitIndex;

			words[wordIndex] &= maskWords[wordIndex] | ~bitMask;

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}
	}

	INLINE void XorBits(void* bits, const void* maskBits, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;
		auto words  = (BitWord*)bits;
		auto maskWords  = (const BitWord*)maskBits;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto bitMask = WordMask(count) << bitIndex;

			words[wordIndex] ^= (maskWords[wordIndex] & bitMask);

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}
	}

	INLINE void ClearBits(void* bits, const void* maskBits, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;
		auto words  = (BitWord*)bits;
		auto maskWords  = (const BitWord*)maskBits;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto bitMask = WordMask(count) << bitIndex;

			words[wordIndex] &= ~maskWords[wordIndex] | ~bitMask;

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}
	}

	INLINE void ClearBits(void* bits, uint32_t index, uint32_t count)
	{
		auto maxBits = index + count;

		auto wordIndex = index >> helper::BitTable::BIT_WORD_SHIFT;
		auto words  = (BitWord*)bits;

		while (index < maxBits)
		{
			auto bitIndex = index & helper::BitTable::BIT_INDEX_MASK;
			auto bitMask = WordMask(count) << bitIndex;

			words[wordIndex] &= ~bitMask;

			index = (++wordIndex) << helper::BitTable::BIT_WORD_SHIFT;
			count -= (helper::BitTable::NUM_BITS_IN_WORD - bitIndex);
		}
	}

    //-----------------------------------------------------------------------------

} // base
