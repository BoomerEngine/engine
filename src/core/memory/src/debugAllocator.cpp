/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\ansi #]
***/

#include "build.h"
#include "debugAllocator.h"
#include "poolStatsInternal.h"

#include "core/system/include/scopeLock.h"

#ifndef BUILD_RELEASE
    #define FILL_PATTERNS
    #define COLLECT_STATS
#endif

//#define VALIDATE_EVERY_ALLOCATION

BEGIN_BOOMER_NAMESPACE_EX(mem)

// #define VALIDATE_EVERY_A

static const uint64_t GBreakOnAlloc = 0;

DebugAllocator::DebugAllocator()
    : m_heapStart(nullptr)
    , m_heapTail(nullptr)
    , m_numBlocks(0)
    , m_maxBlocks(0)
    , m_totalSize(0)
    , m_maxSize(0)
    , m_sequenceNumber(1)
    , m_numTrackedBlocks(0)
    , m_trackedTotalSize(0)
{
}

DebugAllocator::~DebugAllocator()
{
}

static INLINE void WriteWord(void* mem, size_t offset, uint32_t word)
{
    *(uint32_t*)OffsetPtr(mem, offset) = word;
}

void* DebugAllocator::allocate(PoolTag id, size_t size, size_t alignment, const char* typeName)
{
    // snap the alignment
    if (alignment == 0)
        alignment = 1;

    // snap the size
    if (size == 0)
        size = 1;

    // validate the heap
#ifdef VALIDATE_EVERY_ALLOCATION
    validateHeap(nullptr);
#endif

    // allocate larger block
    size_t introSize = sizeof(DebugHeader) + 4;
    size_t extraSize = (sizeof(DebugHeader) + 4 + 4);
    size_t totalSize = size + alignment + extraSize;
    void* systemPtr = malloc(totalSize);
    if (!systemPtr)
        return nullptr;

    // fill the memory with crap
#ifdef FILL_PATTERNS
    memset(systemPtr, 0xCC, totalSize);

    // mark head and tail of the memory
    WriteWord(systemPtr, 0, MARKER_START);
    WriteWord(systemPtr, totalSize-4, MARKER_TAIL);
#endif

    // get the aligned point
    void* usablePtr = AlignPtr(OffsetPtr(systemPtr, introSize), alignment); // align up
    void* headerPtr = (char*)usablePtr - sizeof(DebugHeader);
    ASSERT((char*)headerPtr > (char*)systemPtr);

    // write the memory marker
    auto headerData  = new (headerPtr) DebugHeader;
    headerData->m_size = size;
    headerData->m_systemSize = totalSize;
    headerData->m_alignmentOffset = range_cast<uint16_t>((ptrdiff_t)headerPtr - (ptrdiff_t)systemPtr);
    headerData->m_poolID = id;
    headerData->m_name = typeName;
    headerData->m_seqId = ++m_sequenceNumber;

    // break on allocation
    DEBUG_CHECK_EX(headerData->m_seqId != GBreakOnAlloc, "Selected allocation reached");

    // link blocks
    linkHeapBlock(headerData);

    // track stats
    prv::TheInternalPoolStats.notifyAllocation(id, size);

    // check that the block we've produced is valid
#ifdef FILL_PATTERNS
    headerData->checkMarkers();
#endif
    // return the usable pointer
    return usablePtr;
}

void DebugAllocator::deallocate(void* usablePtr)
{
    if (usablePtr == nullptr)
        return;

    // validate heap
#ifdef VALIDATE_EVERY_ALLOCATION
    validateHeap(usablePtr);
#endif

    // validate header
    auto headerData  = GetHeaderForAddress(usablePtr);

    // unlink from heap list
    unlinkHeapBlock(headerData);

    // track stats
    auto poolID = (PoolTag)headerData->m_poolID;
    prv::TheInternalPoolStats.notifyFree(poolID, headerData->m_size);

    // get base of the block and free the original system blcok
    auto systemPtr  = headerData->systemBlockBase();
#ifdef FILL_PATTERNS
    memset(systemPtr, 0xFB, headerData->m_systemSize);
#endif
    free(systemPtr);
}

void DebugAllocator::linkHeapBlock(DebugHeader* block)
{
    ScopeLock<Mutex> lock(m_lock);

#ifdef COLLECT_STATS
    // update local stats
    m_numBlocks += 1;
    m_maxBlocks = std::max(m_maxBlocks, m_numBlocks);
    m_totalSize += block->m_size;
    m_maxSize = std::max(m_maxSize, m_totalSize);

    // update tracking stats
    if (block->m_poolID != POOL_PERSISTENT)
    {
        m_numTrackedBlocks += 1;
        m_trackedTotalSize += block->m_size;
    }
#endif

    // link block
    block->m_prev = m_heapTail;
    block->m_next = nullptr;
    if (m_heapTail)
        m_heapTail->m_next = block;
    m_heapTail = block;
    if (!m_heapStart)
        m_heapStart = block;
}

void DebugAllocator::unlinkHeapBlock(DebugHeader* block)
{
    ScopeLock<Mutex> lock(m_lock);

#ifdef COLLECT_STATS
    // update local stats
    ASSERT(m_numBlocks > 0);
    m_numBlocks -= 1;
    ASSERT(block->m_size <= m_totalSize);
    m_totalSize -= block->m_size;

    // update tracking stats
    if (block->m_poolID != POOL_PERSISTENT)
    {
        ASSERT(m_numTrackedBlocks > 0);
        m_numTrackedBlocks -= 1;
        ASSERT(m_trackedTotalSize >= block->m_size);
        m_trackedTotalSize -= block->m_size;
    }
#endif

    // unlink block
    if (block->m_prev)
        block->m_prev->m_next = block->m_next;
    if (block->m_next)
        block->m_next->m_prev = block->m_prev;
    if (block == m_heapStart)
        m_heapStart = block->m_next;
    if (block == m_heapTail)
        m_heapTail = block->m_prev;
    block->m_prev = nullptr;
    block->m_next = nullptr;
}

void* DebugAllocator::reallocate(PoolTag id, void* usablePtr, size_t newSize, size_t alignmemnt, const char* typeName)
{
    if (newSize == 0)
    {
        deallocate(usablePtr);
        return nullptr;
    }
    else if (usablePtr == nullptr)
    {
        return allocate(id, newSize, alignmemnt, typeName);
    }
    else
    {
        auto headerData  = GetHeaderForAddress(usablePtr);
        auto oldSize  = headerData->m_size;

        void* ret = allocate(id, newSize, alignmemnt, typeName);
        memcpy(ret, usablePtr, std::min<size_t>(oldSize, newSize));
        deallocate(usablePtr);
        return ret;
    }
}

void DebugAllocator::validateHeap(void* pointerOnHeap)
{
    ScopeLock<Mutex> lock(m_lock);

    auto numBlocksVisited  = 0U;
    auto pointerSeen  = (pointerOnHeap == nullptr);
    for (auto block  = m_heapStart; block != nullptr; block = block->m_next)
    {
        numBlocksVisited += 1;

#ifdef FILL_PATTERNS
        block->checkMarkers();
#endif

        if (!pointerSeen)
        {
            if (block->memoryBaseFromBlock() == pointerOnHeap)
                pointerSeen = true;
        }
    }

    ASSERT_EX(numBlocksVisited == m_numBlocks, "Heap linked list is broken");
    ASSERT_EX(pointerSeen, "Pointer is not from any allocated block, double free?");
}

void DebugAllocator::printLeaks()
{
    struct ListEntry
    {
        uint64_t m_locationHash;
        uint64_t m_typeHash;
        const DebugHeader* m_block;

        INLINE void setup(const DebugHeader* block)
        {
            m_block = block;
            m_locationHash = 0;// block->locationHash();
            m_typeHash = block->typeHash();
        }

        INLINE bool operator<(const ListEntry& entry) const
        {
            // sort by size first
            if (m_block->m_size != entry.m_block->m_size)
                return m_block->m_size > entry.m_block->m_size;

            // sort by block type
            if (m_typeHash != entry.m_typeHash)
                return m_typeHash < entry.m_typeHash;

            // finally, sort by location
            return m_locationHash < entry.m_locationHash;
        }
    };

    // lock access - no allocations possible
    ScopeLock<Mutex> lock(m_lock);

    // report stats
    TRACE_INFO("Max allocated {} blocks, {} max size", m_maxBlocks, m_maxSize);

    // report leaks
    if (m_trackedTotalSize > 0)
    {
        TRACE_INFO("Leaked {} blocks ({} bytes total):", m_numTrackedBlocks, m_trackedTotalSize);

        // prepare sorting table
        auto blockList = (ListEntry *)AllocSystemMemory(sizeof(ListEntry) * (m_numTrackedBlocks+1), false);
        {
            uint32_t index = 0;
            for (auto block = m_heapStart; block != nullptr; block = block->m_next)
            {
                if (block->m_poolID != POOL_PERSISTENT) // yup, some leaks we know
                {
                    if (index > m_numTrackedBlocks)
                    {
                        TRACE_ERROR("Number of tracked blocks inconsistent with actual block count");
                        break;
                    }

                    blockList[index].setup(block);
                    index += 1;
                }
            }
        }

        // sort the list
        std::sort(blockList, blockList + m_numTrackedBlocks);

        // final block
        blockList[m_numTrackedBlocks].m_block = nullptr;
        blockList[m_numTrackedBlocks].m_typeHash = 0;
        blockList[m_numTrackedBlocks].m_locationHash = 0;

        // print the leaks
        uint32_t maxLines = 100;
        uint32_t lineIndex = 0;
        uint32_t prevCount = 0;
        uint64_t prevTotalSize = 0;
        uint64_t prevTypeHash = 0;
        uint64_t prevLocationHash = 0;
        uint32_t firstSeqID = 0;
        for (uint32_t i=0; i<=m_numTrackedBlocks; ++i)
        {
            // print block information
            auto& entry = blockList[i];
            if (prevCount && ((entry.m_locationHash != prevLocationHash) || (entry.m_typeHash != prevTypeHash)))
            {
                auto& prevEntry = blockList[i-1];
                auto prevBlock  = prevEntry.m_block;
                auto pool = (PoolTag)prevBlock->m_poolID;

                TRACE_INFO("Block[{}]: Seq {}, Count {}, TotalSize {}, Pool {} @ {}",
                            lineIndex, firstSeqID, prevCount, prevTotalSize, (int)pool, prevBlock + 1);

                prevCount = 0;
                prevLocationHash = 0;
                prevTypeHash = 0;
                prevTotalSize = 0;
                lineIndex += 1;

                // cannot print any more
                if (lineIndex == maxLines)
                {
                    auto left = m_numTrackedBlocks - i;
                    if (left > 0)
                    {
                        TRACE_INFO("And {} more.... (please fix the current one first!)", left);
                    }
                    break;
                }
            }

            // start new block
            auto block  = entry.m_block;
            if (!block)
                break;

            // remember sequence ID for first leaked block (easier to find)
            if (prevCount == 0)
                firstSeqID = block->m_seqId;
            else
                firstSeqID = std::min(firstSeqID, block->m_seqId);

            // update counts
            prevCount += 1;
            prevTotalSize += block->m_size;
            prevLocationHash = entry.m_locationHash;
            prevTypeHash = entry.m_typeHash;
        }
    }
    else
    {
        TRACE_INFO("No memory leaks detected");
    }
}

END_BOOMER_NAMESPACE_EX(mem)
