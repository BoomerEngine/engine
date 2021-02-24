/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\pools #]
***/

#pragma once

#include "base/system/include/algorithms.h"

BEGIN_BOOMER_NAMESPACE(base::mem)

//---

/// NOTE: pools, in general do not use PageAllocator that is intended for shorter lived payloads when pages are returned quickly (ie. in-frame processing)

/// A simple pool for elements of constant size
/// Elements are organized in blocks each with it's own free list done using bit mask
class BASE_MEMORY_API StructurePoolBase : public NoCopy
{
public:
    StructurePoolBase(PoolTag poolId, uint32_t elementSize, uint32_t elementAlignment, uint32_t minPageCount=1, uint32_t elementsPerPage=0); // 0-auto
	~StructurePoolBase(); // asserts if all elements are not freed

    // elements allocated so far
    INLINE uint32_t size() const { return m_numElements; }

    // size of single element
    INLINE uint32_t elementSize() const { return m_elementSize; }

    // alignment of single element
    INLINE uint32_t elementAlignment() const { return m_elementAlignment; }

    //--

    // allocate single element
    void* alloc();

    // free single element
    void free(void* ptr);

    //--

private:
    struct BlockHeader
    {
        uint32_t freeCount = 0; // free in this block

        BlockHeader* next = nullptr;
        BlockHeader* prev = nullptr;

        uint64_t freeMask[1]; // mask of free entries (continues)
    };

    BlockHeader* m_freeBlockList = nullptr; // we can still allocate from those
    BlockHeader* m_freeBlockTail = nullptr; // we can still allocate from those

    BlockHeader* m_fullBlockList = nullptr; // those are full

    uint32_t m_blockHeaderSize = 0;
    uint32_t m_blockTotalSize = 0;

    BlockHeader* findBlockForPtr(void* ptr); // TODO: optimize

    void unlink(BlockHeader* block);
    void linkToFreeList(BlockHeader* block);
    void linkToFullList(BlockHeader* block);

    PoolTag m_poolId;
        
    uint32_t m_elementSize = 0;
    uint32_t m_elementAlignment = 0;
    uint32_t m_elementsPerPage = 0;

    uint32_t m_numElements = 0;
    uint32_t m_numFreeElements = 0;


    uint32_t m_retainPages = 0;
};

//---

// structure pool for type "T"
template< typename T >
class StructurePool : public StructurePoolBase
{
public:
    INLINE StructurePool(PoolTag poolId = POOL_TEMP, uint32_t elementsPerPage = 0);

    // allocate single element, will call constructor
    template<typename... Args>
    INLINE T* create(Args&& ... args);

    // release single element
    INLINE void free(T* ptr);
};

//---

END_BOOMER_NAMESPACE(base::mem)

#include "structurePool.inl"

