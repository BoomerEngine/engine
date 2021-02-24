/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: memory #]
***/

#pragma once

#include "base/system/include/spinLock.h"
#include "base/containers/include/queue.h"
#include "base/fibers/include/fiberSystem.h"
#include "base/memory/include/structurePool.h"

BEGIN_BOOMER_NAMESPACE(base::fibers)

// a generic memory pool for the operations
// TODO: rewrite once we create the mem::Pool stuff
class BASE_FIBERS_API SharedMemoryPool : public NoCopy
{
public:
    SharedMemoryPool(PoolTag poolID, uint64_t size);
    ~SharedMemoryPool();

    //--

    // allocate single block from the memory pool
    CAN_YIELD Buffer allocBlockAsync(uint32_t size);

    // allocate two blocks from the memory pool
    CAN_YIELD bool allocBlocksAsync(uint32_t compressedSize, uint32_t decompressedSize, Buffer& outCompressedBuffer, Buffer& outDecompressedBuffer);

private:
    PoolTag m_poolID;
    uint64_t m_maxMemorySize;

    std::atomic<uint64_t> m_statUsedSize;
    std::atomic<uint64_t> m_statRequestedSize;
    std::atomic<uint32_t> m_statBlockCount;
    std::atomic<uint32_t> m_statRequestCount;

    struct MemRequest
    {
        uint32_t m_compressedSize;
        uint32_t m_decompressedSize;
        Buffer* m_outCompressedBuffer;
        Buffer* m_outDecompressedBuffer;
        fibers::WaitCounter m_signal;
    };

    mem::StructurePool<MemRequest> m_memRequestPool;
    SpinLock m_lock;

    bool serviceRequestAsync(uint32_t compressedSize, uint32_t decompressedSize, Buffer& outCompressedBuffer, Buffer& outDecompressedBuffer) CAN_YIELD;

    typedef Queue< MemRequest* > TPendingRequests;
    TPendingRequests m_requests;

    Buffer allocBuffer_NoLock(uint32_t size);
    Buffer allocExternalBuffer_NoLock(uint32_t size);

    bool tryAllocate_NoLock(uint32_t compressedSize, uint32_t decompressedSize, Buffer& outCompressedBuffer, Buffer& outDecompresedBuffer);
    void tryAllocatePendingReuests();

    void releaseMemory(void* mem, uint64_t size);
};

//---

END_BOOMER_NAMESPACE(base::fibers)
