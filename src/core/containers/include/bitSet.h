/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "array.h"
#include "inplaceArray.h"
#include "bitUtils.h"

BEGIN_BOOMER_NAMESPACE()

/// set of bits
template< typename Container = Array<BitWord> >
class BitSet
{
public:
	BitSet();
	BitSet(const BitSet& other);
	BitSet(BitSet&& other);
	BitSet& operator=(const BitSet& other);
	BitSet& operator=(BitSet&& other);
	BitSet(uint32_t bitCount, EBitStateZero);
	BitSet(uint32_t bitCount, EBitStateOne);
	BitSet(const Container& other, uint32_t maxBitCount = INDEX_MAX); // suck up to maxBitCount
	BitSet(Container&& other, uint32_t maxBitCount = INDEX_MAX); // suck up to maxBitCount
	BitSet(const void* data, uint32_t maxBitCount = INDEX_MAX); // suck up to maxBitCount, TODO: add bit offset

	// is the bit set empty ? (no bit)
	bool empty() const;

	// get number of bits in use
	uint32_t size() const;

	// get bit capacity (always multiple of sizeof(BitWord) * 8)
	uint32_t capcity() const;

	// get index last valid bit index (size() - 1)
	int lastValidIndex() const;

	// get bit data
	void* data();

	// get raw data
	const void* data() const;

	// get bit data
	BitWord* typedData();

	// get bit data
	const BitWord* typedData() const;

	//--

	// reset container size to 0 but do not free memory
	void reset();

	// clear container
	// NOTE: do not mistake for clear(uint32_t) that clears a single bit
	void clear();

	// resize container to hold X bits, new bits are initialized to 0
	void resizeWithZeros(uint32_t bitCount);

	// resize container to hold X bits, new bits are initialized to 1
	void resizeWithOnes(uint32_t bitCount);

	//--

	// clear all bits
	void clearAll();

	// set all bits
	void enableAll();

	//--

	// get state of n-th bit
	bool operator[](uint32_t index) const;

	// set bit to 1, returns previous bit state
	bool set(uint32_t index);

	// set range of bits to 1, returns previous bit state
	void range(uint32_t index, uint32_t count);

	// clear bit to 0, returns previous bit state
	bool clear(uint32_t index);

	// clear range of bits to 1, returns previous bit state
	void clearRange(uint32_t index, uint32_t count);

	// set bit to given state, returns previous bit state
	bool toggle(uint32_t index, bool state);

	// clear range of bits to 1, returns previous bit state
	void toggleRange(uint32_t index, uint32_t count, bool state);

	//--

	// or with other set
	BitSet<Container>& operator|=(const BitSet<Container>& otherMask);

	// and the bits between sets
	BitSet<Container>& operator&=(const BitSet<Container>& otherMask);

	// xor the bits between sets
	BitSet<Container>& operator^=(const BitSet<Container>& otherMask);

	// remove the bits between sets
	BitSet<Container>& operator-=(const BitSet<Container>& otherMask);

    //---

    // are all bits in given test mask set in here ?
	bool isAllSet(const BitSet<Container>& otherMask) const;

    // is any bits from given test mask set in here ?
	bool isAnySet(const BitSet<Container>& otherMask) const;

    // is none bits from given test mask set in here ?
    bool isNoneSet(const BitSet<Container>& otherMask) const;
		
    // are all bits in the bit set set ? true for empty bitset
    bool isAllSet() const;

    // is any bits from given test mask set in here ? false for empty bitset
    bool isAnySet() const;

    // are none bit set ? true for empty bitset
    bool isNoneSet() const;

    //----

    // compare if two bitsets are equal
    bool operator==(const BitSet<Container>& other) const;
		
    // compare if two bitsets are not equal
    bool operator!=(const BitSet<Container>& other) const;

    //----

    // iterate over all bits that are set
	bool iterateSetBits(const std::function<bool(uint32_t index)>& enumFunc) const;

    // iterate over all bits that are clear
    bool iterateClearBits(const std::function<bool(uint32_t index)>& enumFunc) const;

	// 

private:
	uint32_t m_bitSize = 0;
	Container m_words;

	static const uint32_t NUM_BITS_IN_WORD = sizeof(BitWord) * 8;
	static const uint32_t BIT_WORD_SHIFT = 5;
	static const uint32_t BIT_INDEX_MASK = (1U << BIT_WORD_SHIFT) - 1;
	static const BitWord ALL_ONES = ~(BitWord)0;
	static const BitWord ALL_ZEROS = (BitWord)0;
};

/// simple bitset with static initial storage
template < uint32_t InitialSize >
class InplaceBitSet : public BitSet< InplaceArray<uint32_t, (InitialSize + 31) / 32> >
{
public:
	InplaceBitSet(EBitStateZero);
	InplaceBitSet(EBitStateOne);
};

END_BOOMER_NAMESPACE()

#include "bitSet.inl"