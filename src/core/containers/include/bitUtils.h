/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

/// bit word for the all of the bit storages
/// NOTE: 32-bit even on 64-bit systems
typedef uint32_t BitWord;

/// special bit state enums
enum class EBitStateZero { ZERO };
enum class EBitStateOne { ONE };

// test if single bit is set
bool TestBit(const void* bits, uint32_t index);

// clear single bit, returns previous value
bool ClearBit(const void* bits, uint32_t index);

// set single bit, returns previous value
bool SetBit(const void* bits, uint32_t index);

// get index of first bit set in the set of bits
int FindFirstBitSet(const void* bits, uint32_t maxBits);

// get index of first bit cleared in the set of bits
int FindFirstBitClear(const void* bits, uint32_t maxBits);

// get index of next bit set in the set
uint32_t FindNextBitSet(const void* bits, uint32_t maxBits, uint32_t searchStart);

// get index of next bit clear in the set
uint32_t FindNextBitCleared(const void* bits, uint32_t maxBits, uint32_t searchStart);

// compare bits between two bit sets
bool CompareBits(const void* bitsA, const void* bitsB, uint32_t firstBit, uint32_t numBits);

// returns true if ALL bits in the range is set
bool AllBitsSet(const void* bits, uint32_t firstBit, uint32_t numBits);

// returns true if ANY bits in the range is set
bool AnyBitsSet(const void* bits, uint32_t firstBit, uint32_t numBits);

// returns true if ALL bits in the range is set
bool AllBitsSet(const void* bits, const void* maskBits, uint32_t firstBit, uint32_t numBits);

// returns true if ANY bits in the range is set
bool AnyBitsSet(const void* bits, const void* maskBits, uint32_t firstBit, uint32_t numBits);

// or the bits from the given masks and in given range
void OrBits(void* bits, const void* maskBits, uint32_t firstBit, uint32_t numBits);

// and the bits from the given masks and in given range
void AndBits(void* bits, const void* maskBits, uint32_t firstBit, uint32_t numBits);

// xor the bits from the given masks and in given range
void XorBits(void* bits, const void* maskBits, uint32_t firstBit, uint32_t numBits);

// negate the bits from the given masks and in given range
void ClearBits(void* bits, const void* maskBits, uint32_t firstBit, uint32_t numBits);

// clear range of bits
void ClearBits(void* bits, uint32_t firstBit, uint32_t numBits);

// set range of bits
void SetBits(void* bits, uint32_t firstBit, uint32_t numBits);

//--

END_BOOMER_NAMESPACE()

#include "bitUtils.inl"