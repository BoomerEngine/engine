/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiTransientBuffer.h"
#include "base/memory/include/poolStats.h"

namespace rendering
{
    namespace api
    {

        //---

        base::ConfigProperty<uint32_t> cvMinTempBufferSize("Rendering", "MinTempBufferSizeKB", 256);
        base::ConfigProperty<uint32_t> cvTempBufferPageSize("Rendering", "TempBufferPageSizeKB", 64);

        //---

        IBaseTransientBuffer::IBaseTransientBuffer(IBaseTransientBufferPool* owner, uint32_t size, TransientBufferType bufferType)
            : m_owner(owner)
            , m_size(size)
            , m_type(bufferType)
        {
			base::mem::PoolStats::GetInstance().notifyAllocation(POOL_API_TRANSIENT_BUFFER, m_size);
        }

        IBaseTransientBuffer::~IBaseTransientBuffer()
        {
            base::mem::PoolStats::GetInstance().notifyFree(POOL_API_TRANSIENT_BUFFER, m_size);
        }

        void IBaseTransientBuffer::returnToPool()
        {
            // reset the dirty state
            m_mappedWriteMinRange = m_size;
            m_mappedWriteMaxRange = 0;

            // return to pool
            m_owner->returnBuffer(this);
        }

        void IBaseTransientBuffer::writeData(uint32_t offset, uint32_t size, const void* srcData)
        {
            ASSERT_EX(m_mappedPtr != nullptr, "Writes can only happen to a mapped buffer");
            ASSERT_EX(offset + size <= m_size, "Out of range write");
            memcpy(m_mappedPtr + offset, srcData, size);
            m_mappedWriteMinRange = std::min<uint32_t>(m_mappedWriteMinRange, offset);
            m_mappedWriteMaxRange = std::max<uint32_t>(m_mappedWriteMaxRange, offset + size);
        }

        void IBaseTransientBuffer::flush()
        {
            ASSERT_EX(m_mappedPtr != nullptr, "Writes can only happen to a mapped buffer");

            if (m_mappedWriteMaxRange > m_mappedWriteMinRange)
            {
                auto updateSize = m_mappedWriteMaxRange - m_mappedWriteMinRange;
				flushInnerWrites(m_mappedWriteMinRange, updateSize);

				m_mappedWriteMinRange = m_size;
				m_mappedWriteMaxRange = 0;
            }
        }

        void IBaseTransientBuffer::copyDataFrom(const IBaseTransientBuffer* srcBuffer, uint32_t srcOffset, uint32_t destOffset, uint32_t size)
        {
            ASSERT_EX(m_mappedPtr == nullptr, "Copy can only happen into unmapped buffer");
            ASSERT_EX(srcOffset + size <= srcBuffer->size(), "Out of bounds read");
            ASSERT_EX(destOffset + size <= this->size(), "Out of bounds write");
        }

        //--

		IBaseTransientBufferPool::IBaseTransientBufferPool(IBaseThread* drv, TransientBufferType type)
            : m_owner(drv)
            , m_type(type)
        {
        }

        IBaseTransientBufferPool::~IBaseTransientBufferPool()
        {
            ASSERT_EX(m_numAllocatedBuffers == 0, "There are still allocated transient buffers, not all frames have finished");

            for (const auto& buf : m_freeBuffers)
                delete buf.buffer;
        }

        void IBaseTransientBufferPool::returnBuffer(IBaseTransientBuffer* buffer)
        {
            auto lock = base::CreateLock(m_lock);

            ASSERT_EX(m_numAllocatedBuffers != 0, "Returning a buffer when there's nothing to return");
            m_numAllocatedBuffers -= 1;

            auto& entry = m_freeBuffers.emplaceBack();
            entry.timestamp = m_tickCounter++;
            entry.size = buffer->size();
            entry.buffer = buffer;
        }

        IBaseTransientBuffer* IBaseTransientBufferPool::allocate(uint32_t requiredSize)
        {
            DEBUG_CHECK_RETURN_V(requiredSize, nullptr);

            auto lock = base::CreateLock(m_lock);

            // round the buffer up
            const auto pageSize = base::NextPow2(std::max<uint32_t>(cvTempBufferPageSize.get() << 10, 4096));
            const auto minSize = std::max<uint32_t>(pageSize, cvMinTempBufferSize.get() << 10);
            const auto roundedSize = base::Align<uint32_t>(std::max<uint32_t>(requiredSize, minSize), pageSize);

            // find smallest buffer that fits
            uint32_t totalUnusedSize = 0;
            {
                uint32_t bestEntry = INDEX_MAX;
                uint32_t bestSize = INDEX_MAX;
                for (auto i : m_freeBuffers.indexRange())
                {
                    const auto& entry = m_freeBuffers[i];
                    if (requiredSize <= entry.size)
                    {
                        if (entry.size <= bestSize) // prefer newest buffers over older
                        {
                            bestSize = entry.size;
                            bestEntry = i;
                        }
                    }

                    totalUnusedSize += entry.size;
                }

                // use best buffer
                if (bestSize != INDEX_MAX)
                {
                    auto* buf = m_freeBuffers[bestEntry].buffer;
                    m_freeBuffers.erase(bestEntry); // keep order (buffers are sorted LRU)

                    m_numAllocatedBuffers += 1;
                    return buf;
                }
            }

            TRACE_INFO("GL: Allocating temporary buffer {}, {} free buffers, {} unused space", MemSize(roundedSize), m_freeBuffers.size(), MemSize(totalUnusedSize));

            // remove free buffers that are unused to free some memory for a new one we are going to created
            {
                uint32_t freedSoFar = 0;
                uint32_t numFreedBuffer = 0;

                for (auto i : m_freeBuffers.indexRange())
                {
                    if (freedSoFar >= roundedSize)
                        break;

                    const auto& entry = m_freeBuffers[i];
                    freedSoFar += entry.size;

                    delete entry.buffer;
                    numFreedBuffer += 1;
                }

                if (numFreedBuffer)
                {
                    m_freeBuffers.erase(0, numFreedBuffer);
                    TRACE_INFO("GL: Deleted {} unused buffers containing {}. Still have {} buffers ({})", 
                        numFreedBuffer, MemSize(freedSoFar), m_freeBuffers.size(), totalUnusedSize - freedSoFar);
                }
            }

            // create new buffer
            m_numAllocatedBuffers += 1;
            return createBuffer(roundedSize);
        }

        //--

    } // api
} // rendering
