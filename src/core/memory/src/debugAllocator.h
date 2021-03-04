/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\ansi #]
***/

#pragma once

#include "core/system/include/spinLock.h"
#include "core/system/include/mutex.h"

BEGIN_BOOMER_NAMESPACE()

/// Simple debug allocator with simple overrun/underrun checks and stats
class DebugAllocator
{
public:
    DebugAllocator();
    ~DebugAllocator();

    static const uint32_t DEFAULT_ALIGNMNET = 8;

    //! Allocate memory
    void* allocate(PoolTag id, size_t size, size_t alignmemnt, const char* typeName);

    //! Deallocate memory
    void deallocate(void* mem);

    //! Resize allocated memory block
    void* reallocate(PoolTag id, void* mem, size_t newSize, size_t alignmemnt, const char* typeName);

    //---

    // print memory leaks to the output
    void printLeaks();

    // validate heap status
    void validateHeap(void* pointerOnHeap);

public:
    static const uint32_t MARKER_A = 0xABACADAE;
    static const uint32_t MARKER_B = 0xEAEBECED;
    static const uint32_t MARKER_START = 0x55555555;
    static const uint32_t MARKER_TAIL = 0xDEADF00D;

    struct DebugHeader
    {
        uint32_t m_markerA;
        uint16_t m_alignmentOffset;
        uint8_t m_poolID;
        size_t m_size;
        size_t m_systemSize;
        uint32_t m_seqId;
        DebugHeader* m_prev;
        DebugHeader* m_next;
        const char* m_name;
        uint32_t m_markerB;
                
        INLINE DebugHeader()
            : m_markerA(MARKER_A)
            , m_alignmentOffset(0)
            , m_poolID(0)
            , m_size(0)
            , m_systemSize(0)
            , m_seqId(0)
            , m_prev(nullptr)
            , m_next(nullptr)
            , m_name(nullptr)
            , m_markerB(MARKER_B)
        {}

        INLINE void* memoryBaseFromBlock() const
        {
            return (char*)this + sizeof(DebugHeader);
        }

        INLINE void* systemBlockBase() const
        {
            return (char*)this - (ptrdiff_t)m_alignmentOffset;
        }

        INLINE void* systemBlockEnd() const
        {
            return (char*)systemBlockBase() + m_systemSize;
        }

        INLINE void checkMarkers()
        {
            // check local markers
            ASSERT_EX(m_markerA == MARKER_A, "Memory header corruption");
            ASSERT_EX(m_markerB == MARKER_B, "Memory header corruption");

            // check the start marker
            auto startPtr  = (const uint32_t*)systemBlockBase();
            ASSERT_EX(*startPtr == MARKER_START, "Start block marker corrupted");

            // check the end marker
            auto endPtr  = (const uint32_t*)systemBlockEnd() - 1;
            ASSERT_EX(*endPtr == MARKER_TAIL, "End block marker corrupted");
        }

        INLINE uint64_t typeHash() const
        {
            if (!m_name)
                return 0;

            uint64_t hval = UINT64_C(0xcbf29ce484222325);
            auto ch  = m_name;
            while (*ch)
            {
                hval ^= (uint64_t)*ch++;
                hval *= UINT64_C(0x100000001b3);
            }

            return hval;
        }
    };

    Mutex m_lock;
    uint32_t m_numBlocks;
    uint32_t m_maxBlocks;
    uint64_t m_totalSize;
    uint64_t m_maxSize;

    uint32_t m_numTrackedBlocks;
    uint64_t m_trackedTotalSize;

    DebugHeader* m_heapStart;
    DebugHeader* m_heapTail;

    std::atomic<uint32_t> m_sequenceNumber;

    INLINE static DebugHeader* GetHeaderForAddress(void* mem)
    {
        auto headerData  = (DebugHeader*)((char*)mem - sizeof(DebugHeader));
        headerData->checkMarkers();
        return headerData;
    }

    void printHeap();

    void linkHeapBlock(DebugHeader* block);
    void unlinkHeapBlock(DebugHeader* block);
};

END_BOOMER_NAMESPACE()
