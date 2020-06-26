/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers\dynamic #]
***/

#pragma once

#include "bitSet.h"

namespace base
{

    /// allocates "bits" from a bit set, good for allocating IDs with minimal storage requirements
    template< typename Container = BitSet< Array<BitWord> > >
    class BitPool
    {
    public:
		BitPool();

		//--

		// get number of allocated IDs
		uint32_t size() const;

		// get the capacity of the allocator
		uint32_t capacity() const;

		//--

        // reset the allocator - release all IDs
        void reset();

		// reserve space for N ids
		void reseve(uint32_t numIds);

        // get index of first free bit that it not set
		// NOTE: this resizes the bit container if it's full
        uint32_t allocate();

        // release allocated ID
        void release(uint32_t id);

    private:
        Container m_freeIndices;
        uint32_t m_searchIndex = 0;
        uint32_t m_numAllocated = 0;
    };

} // base

#include "bitPool.inl"
