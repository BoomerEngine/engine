/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\pool #]
***/

#include "build.h"
#include "structurePool.h"

namespace base
{
    namespace mem
    {

        //---

        namespace helper
        {
            static uint32_t FindBestNumberOfElementsPerPage(uint32_t constHeaderSize, uint32_t elementSize, uint32_t elementAlignment, uint32_t elementsPerPage)
            {
                const auto defaultPageSize = 4096;

                // how many pages we need for all elements if we use minimal amount (64)
                elementSize = std::max<uint32_t>(elementSize, elementAlignment);
                auto minPageSize = Align<uint64_t>(constHeaderSize + (64 * elementSize), defaultPageSize);

                // how many elements can the min page really fit (fill it up to the full page size)
                elementsPerPage = (minPageSize - constHeaderSize) / elementSize;
                for (;;)
                {
                    auto numBitWords = (elementsPerPage + 63) / 64;
                    auto headerSize = Align<uint32_t>(constHeaderSize + sizeof(uint64_t) * numBitWords, elementAlignment);

                    auto newElementsPerPage = (minPageSize - headerSize) / elementSize;
                    if (newElementsPerPage == elementsPerPage)
                        return newElementsPerPage;

                    elementsPerPage = newElementsPerPage;
                }
            }

            static bool CheckMaskWord(const uint64_t* mask, uint32_t index)
            {
                const auto wordIndex = index / 64;
                const auto bitMask = 1ULL << (index & 63);
                return 0 != (mask[wordIndex] & bitMask);
            }

            static void ClearMaskWord(uint64_t* mask, uint32_t index)
            {
                const auto wordIndex = index / 64;
                const auto bitMask = 1ULL << (index & 63);
                mask[wordIndex] &= ~bitMask;
            }

            static void SetMaskWord(uint64_t* mask, uint32_t index)
            {
                const auto wordIndex = index / 64;
                const auto bitMask = 1ULL << (index & 63);
                mask[wordIndex] |= bitMask;
            }

            static uint64_t FindFirstBitSetAndClearIt(uint64_t* mask, uint32_t count)
            {
                auto numWords = (count + 63) / 64;
                auto* curPtr = mask;
                auto* endPtr = mask + numWords;
                while (curPtr < endPtr)
                {
                    if (*curPtr != 0)
                    {
                        uint32_t bitIndex = __builtin_ctzll(*curPtr);
                        uint32_t totalBitIndex = bitIndex + ((curPtr - mask) * 64);
                        DEBUG_CHECK_EX(totalBitIndex < count, "Bit outside allowed range");
                        *curPtr &= ~(1ULL << bitIndex);
                        return totalBitIndex;
                    }

                    ++curPtr;
                }

                DEBUG_CHECK(!"No bits found");
                return 0;
            }
        }


        //---

        StructurePoolBase::StructurePoolBase(PoolID poolId, uint32_t elementSize, uint32_t elementAlignment, uint32_t minPageCount, uint32_t elementsPerPage)
            : m_poolId(poolId)
            , m_elementSize(std::max<uint32_t>(elementSize, elementAlignment))
            , m_elementAlignment(elementAlignment)
            , m_elementsPerPage(elementsPerPage)
        {
            // calculate best count of elements per block
            if (m_elementsPerPage == 0)
            {
                const auto constHeaderSize = sizeof(BlockHeader) - sizeof(uint64_t);
                m_elementsPerPage = helper::FindBestNumberOfElementsPerPage(constHeaderSize, elementSize, elementAlignment, elementsPerPage);
            }

            auto numMaskWords = (m_elementsPerPage + 63) / 64;
            m_blockHeaderSize = Align<uint32_t>(sizeof(BlockHeader) + sizeof(uint64_t) * (numMaskWords - 1), m_elementAlignment);
            m_blockTotalSize = m_blockHeaderSize + (m_elementSize * m_elementsPerPage);
        }

        StructurePoolBase::~StructurePoolBase()
        {
            DEBUG_CHECK_EX(m_numElements == 0, "There are still some elements allocated from structure pool");
        }

        void* StructurePoolBase::alloc()
        {
            // allocate from a free block
            if (auto* block = m_freeBlockList)
            {
                DEBUG_CHECK_EX(block->freeCount > 0, "Full block in free list");
                DEBUG_CHECK_EX(m_numFreeElements > 0, "Counting error");

                auto elementIndex = helper::FindFirstBitSetAndClearIt(block->freeMask, m_elementsPerPage);
                DEBUG_CHECK_EX(!helper::CheckMaskWord(block->freeMask, elementIndex), "Bit still set");

                auto* ptr = (uint8_t*)block + m_blockHeaderSize + (elementIndex * m_elementSize);
                ASSERT_EX(AlignPtr(ptr, m_elementAlignment) == ptr, "Wrong pool element alignment");

                block->freeCount -= 1;
                m_numElements += 1;
                m_numFreeElements -= 1;

                if (block->freeCount > 0)
                    return ptr;

                unlink(block);
                linkToFullList(block);
                return ptr;
            }

            // allocate a new block
            auto* newBlock = (BlockHeader*) AllocateBlock(m_poolId, m_blockTotalSize, m_elementAlignment);
            if (!newBlock) // out of memory
                return nullptr;

            newBlock->next = nullptr;
            newBlock->prev = nullptr;
            newBlock->freeCount = m_elementsPerPage;
            m_numFreeElements += m_elementsPerPage;

            // mark all entries as free
            auto numMaskWords = (m_elementsPerPage + 63) / 64;
            memset(newBlock->freeMask, 0xFF, numMaskWords * sizeof(uint64_t));

            linkToFreeList(newBlock);
            return alloc();
        }

        StructurePoolBase::BlockHeader* StructurePoolBase::findBlockForPtr(void* ptr)
        {
            auto* testPtr = (const uint8_t*)ptr;

            auto* cur = m_fullBlockList;
            while (cur)
            {
                auto* startPtr = (const uint8_t*)cur;
                auto* endPtr = startPtr + m_blockTotalSize;
                if (testPtr >= startPtr && testPtr <= endPtr)
                    return cur;
                cur = cur->next;
            }

            cur = m_freeBlockList;
            while (cur)
            {
                auto* startPtr = (const uint8_t*)cur;
                auto* endPtr = startPtr + m_blockTotalSize;
                if (testPtr >= startPtr && testPtr <= endPtr)
                    return cur;
                cur = cur->next;
            }

            DEBUG_CHECK_EX(!ptr, "Pointer outside any managed block");
            return nullptr;
        }

        void StructurePoolBase::linkToFullList(BlockHeader* block)
        {
            DEBUG_CHECK(block->next == nullptr);
            DEBUG_CHECK(block->prev == nullptr);

            block->next = m_fullBlockList;
            if (m_fullBlockList)
                m_fullBlockList->prev = block;
            m_fullBlockList = block;
        }

        void StructurePoolBase::linkToFreeList(BlockHeader* block)
        {
            DEBUG_CHECK(block->next == nullptr);
            DEBUG_CHECK(block->prev == nullptr);

            if (nullptr != m_freeBlockTail)
            {
                block->prev = m_freeBlockTail;
                m_freeBlockTail->next = block;
                m_freeBlockTail = block;
            }
            else
            {
                m_freeBlockTail = block;
                m_freeBlockList = block;
            }
        }

        void StructurePoolBase::unlink(BlockHeader* block)
        {
            if (block->prev)
                block->prev->next = block->next;
            else if (block == m_freeBlockList)
                m_freeBlockList = block->next;
            else if (block == m_fullBlockList)
                m_fullBlockList = block->next;

            if (block->next)
                block->next->prev = block->prev;
            else if (block == m_freeBlockTail)
                m_freeBlockTail = block->prev;

            block->next = nullptr;
            block->prev = nullptr;
        }

        void StructurePoolBase::free(void* ptr)
        {
            DEBUG_CHECK_EX(ptr != nullptr, "Freeing null is not legal in structure pool");

            auto* block = findBlockForPtr(ptr);
            auto* firstElem = (uint8_t*)block + m_blockHeaderSize;
            auto elementOffset = (ptrdiff_t)((uint8_t*)ptr - firstElem);
            DEBUG_CHECK_EX(elementOffset >= 0, "Freed element outside block range");
            DEBUG_CHECK_EX(elementOffset % m_elementAlignment == 0, "Freed element not aligned");
            auto elementIndex = elementOffset / m_elementSize;
            DEBUG_CHECK_EX(elementIndex < m_elementsPerPage, "Element index beyond range");
            DEBUG_CHECK_EX(!helper::CheckMaskWord(block->freeMask, elementIndex), "Element marked as free, should be marked as allocated");
            DEBUG_CHECK_EX(block->freeCount < m_elementsPerPage, "Block has no count for free elements");
            DEBUG_CHECK_EX(m_numElements > 0, "Invalid element count");

            helper::SetMaskWord(block->freeMask, elementIndex);
            block->freeCount += 1;
            m_numFreeElements += 1;
            m_numElements -= 1;

            if (block->freeCount > 1)
                return;

            unlink(block);
            linkToFreeList(block);
        }

        //---

    } // mem
} // base
