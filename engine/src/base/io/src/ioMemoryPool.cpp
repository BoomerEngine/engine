/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\memory #]
***/

#include "build.h"
#include "ioMemoryPool.h"

#include "base/system/include/scopeLock.h"

namespace base
{
    namespace io
    {

        MemoryPool::MemoryPool()
            : m_maxMemorySize(128U << 20)
            , m_statUsedSize(0)
            , m_statRequestedSize(0)
            , m_statBlockCount(0)
            , m_statRequestCount(0)
            , m_memRequestPool(POOL_IO, 1024)
        {
        }

        MemoryPool::~MemoryPool()
        {
            //DEBUG_CHECK_EX(m_statUsedSize.value() == 0, "IO memory pool still in use when releasing");
            //DEBUG_CHECK_EX(m_statBlockCount.value() == 0, "IO memory pool still in use when releasing");
            //DEBUG_CHECK_EX(m_statRequestedSize.value() == 0, "IO memory pool still has pending requests when releasing");
        }

        Buffer MemoryPool::allocBlockAsync(uint32_t size)
        {
            // lock access
            m_lock.acquire();

            // we need to allocate BOTH buffers at the same time
            {
                // try to directly allocate, this won't fail if we have enough memory (common case)
                // also in this case we wont yield the fiber
                Buffer compressedData, uncompressedData;
                if (tryAllocate_NoLock(0, size, compressedData, uncompressedData))
                {
                    TRACE_SPAM("Memory allocation ({}) succeeded", size);
                    m_lock.release();
                    return uncompressedData;
                }
            }

            // we don't have enough memory ATM, create a pending request
            Buffer compressedData, uncompressedData;
            serviceRequestAsync(0, size, compressedData, uncompressedData);
            return uncompressedData;
        }

        bool MemoryPool::allocBlocksAsync(uint32_t compressedSize, uint32_t decompressedSize, Buffer& outCompressedBuffer, Buffer& outDecompressedBuffer)
        {
            // we need to allocate BOTH buffers at the same time
            {
                ScopeLock<SpinLock> lock(m_lock);

                // try to directly allocate, this won't fail if we have enough memory (common case)
                // also in this case we wont yield the fiber
                if (tryAllocate_NoLock(compressedSize, decompressedSize, outCompressedBuffer, outDecompressedBuffer))
                {
                    TRACE_INFO("Memory allocation ({}+{}) succeeded", compressedSize + decompressedSize);
                    return true;
                }
            }

            // we don't have enough memory ATM, create a pending request
            return serviceRequestAsync(compressedSize, decompressedSize, outCompressedBuffer, outDecompressedBuffer);
        }

        //---

        static void FreeIOMemory(mem::PoolID pool, void* memory, uint64_t size)
        {
            MemFree(memory);
        }

        Buffer MemoryPool::allocExternalBuffer_NoLock(uint32_t size)
        {
            // zero size allocation
            if (size == 0)
                return Buffer(); // empty buffer is valid result of 0 size allocation

            // allocate memory
            auto memory = MemAlloc(POOL_IO, size, 1);

            // prepare free function so we know where to return the shit
            // NOTE: the free function will try to kick off some more allocation
            auto freeFunc  = [this](void* memory, uint64_t size)
            {
                MemFree(memory);
            };

            // when this buffer is destroyed the pool will try to kick of next allocations
            // this way we have a nice throttling on the amount of the IO memory used
            return Buffer::CreateExternal(POOL_IO, size, memory, &FreeIOMemory);
        }

        Buffer MemoryPool::allocBuffer_NoLock(uint32_t size)
        {
            ASSERT(m_statUsedSize.load() + size <= m_maxMemorySize);

            // zero size allocation
            if (size == 0)
                return Buffer(); // empty buffer is valid result of 0 size allocation

            // allocate memory
            // TODO: use actual memory pool
            auto memory  = MemAlloc(POOL_IO, size, 1);

            // update stats
            ++m_statBlockCount;
            auto newSize  = m_statUsedSize += size;
            ASSERT(newSize <= m_maxMemorySize);

            // when this buffer is destroyed the pool will try to kick of next allocations
            // this way we have a nice throttling on the amount of the IO memory used
            return Buffer::CreateExternal(POOL_IO, size, memory, &FreeIOMemory);
        }

        bool MemoryPool::tryAllocate_NoLock(uint32_t compressedSize, uint32_t decompressedSize, Buffer& outCompressedBuffer, Buffer& outDecompresedBuffer)
        {
            ASSERT(m_statUsedSize.load() <= m_maxMemorySize);

            // if we can never allocate the buffers from the pool try to allocate them right away (we may crash but otherwise we would got endless loading)
            if (compressedSize + decompressedSize > 0)//m_maxMemorySize)
            {
                outCompressedBuffer = allocExternalBuffer_NoLock(compressedSize);
                outDecompresedBuffer = allocExternalBuffer_NoLock(decompressedSize);
                return true;
            }

            // do we have enough free memory ?
            auto memoryNeeded  = compressedSize + decompressedSize;
            if (m_statUsedSize.load() + memoryNeeded > m_maxMemorySize)
            {
                TRACE_WARNING("Trying to allocate a {} + {} bytes of IO memory that is bigger than the currently free IO pool size {} of {}", compressedSize, decompressedSize, m_statUsedSize.load(), m_maxMemorySize);
                return false;
            }

            // allocate buffers (this does not fail)
            outCompressedBuffer = allocBuffer_NoLock(compressedSize);
            outDecompresedBuffer = allocBuffer_NoLock(decompressedSize);

            // we are done
            return true;
        }

        void MemoryPool::tryAllocatePendingReuests()
        {
            ScopeLock<SpinLock> lock(m_lock);

            // memory was released, try to service as many allocation requests as possible
            while (!m_requests.empty())
            {
                ASSERT(m_statUsedSize.load() <= m_maxMemorySize);

                // do we have memory now ?
                Buffer compressedBuffer, decompressedBuffer;
                auto request  = m_requests.top();
                if (!tryAllocate_NoLock(request->m_compressedSize, request->m_decompressedSize, compressedBuffer, decompressedBuffer))
                {
                    TRACE_INFO("Cannot yet service the request");
                    break;
                }

                // request was accepted
                TRACE_INFO("Request can be serviced from just released memory");
                m_requests.pop();

                // write allocated compressed buffer
                if (request->m_outCompressedBuffer)
                    *request->m_outCompressedBuffer = compressedBuffer;

                // write allocated decompression buffer
                if (request->m_outDecompressedBuffer)
                    *request->m_outDecompressedBuffer = decompressedBuffer;

                // update stats
                m_statRequestedSize -= request->m_compressedSize;
                m_statRequestedSize -= request->m_decompressedSize;
                --m_statRequestCount;

                // unlock the waiting job
                Fibers::GetInstance().signalCounter(request->m_signal);
            }
        }

        void MemoryPool::releaseMemory(void* mem, uint64_t size)
        {
            DEBUG_CHECK_EX(mem != nullptr, "Empty memory should not be released here");
            DEBUG_CHECK_EX(size != 0, "Empty memory should not be released here");

            // stats
            TRACE_SPAM("Freed IO memory of size {}", size);

            // free the memory itself
            MemFree(mem);

            // update stats
            ASSERT(size <= m_statUsedSize.load());
            ASSERT(m_statUsedSize.load() <= m_maxMemorySize);
            m_statUsedSize -= (size_t)size;
            ASSERT(m_statBlockCount.load() > 0);
            m_statBlockCount--;

            // kickoff pending results
            tryAllocatePendingReuests();
        }

        bool MemoryPool::serviceRequestAsync(uint32_t compressedSize, uint32_t decompressedSize, Buffer& outCompressedBuffer, Buffer& outDecompressedBuffer) CAN_YIELD
        {
            TRACE_INFO("Memory allocation ({}+{}) blocked, adding to waiting list", compressedSize + decompressedSize);

            // allocate request from pool
            auto req = m_memRequestPool.create();

            req->m_compressedSize = compressedSize;
            req->m_decompressedSize = decompressedSize;
            req->m_outCompressedBuffer = &outCompressedBuffer;
            req->m_outDecompressedBuffer = &outDecompressedBuffer;
            req->m_signal = Fibers::GetInstance().createCounter("MemoryReadySignal");

            m_statRequestedSize += compressedSize;
            m_statRequestedSize += decompressedSize;
            m_statRequestCount++;

            m_requests.push(req);
            m_lock.release();

            TRACE_INFO("Waiting for io memory of size {} + {} to be freed", compressedSize, decompressedSize);
            Fibers::GetInstance().waitForCounterAndRelease(req->m_signal);
            TRACE_INFO("Memory of size {} + {} was freed and allocated", compressedSize, decompressedSize);

            m_lock.acquire();
            m_memRequestPool.free(req);
            m_lock.release();

            m_statRequestedSize -= compressedSize;
            m_statRequestedSize -=decompressedSize;
            --m_statRequestCount;

            return true;
        }

    } // io
} // bas
