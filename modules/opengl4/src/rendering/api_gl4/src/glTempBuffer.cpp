/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "glDevice.h"
#include "glTempBuffer.h"

namespace rendering
{
    namespace gl4
    {

        //---

        base::ConfigProperty<uint32_t> cvMinTempBufferSize("Rendering.GL4", "MinTempBufferSizeKB", 256);
        base::ConfigProperty<uint32_t> cvTempBufferPageSize("Rendering.GL4", "TempBufferPageSizeKB", 64);

        //---

        TempBuffer::TempBuffer(TempBufferPool* owner, uint32_t size, TempBufferType bufferType)
            : m_owner(owner)
            , m_size(size)
            , m_type(bufferType)
            , m_mappedPtr(nullptr)
            , m_mappedWriteMinRange(size)
            , m_mappedWriteMaxRange(0)
        {
            // create the buffer
            GLuint buffer = 0;
            GL_PROTECT(glCreateBuffers(1, &buffer));
            m_glBuffer = buffer;

            // label the buffer
            if (bufferType == TempBufferType::Storage)
                GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, -1, "TransientStorageBuffer"))
            else if (bufferType == TempBufferType::Staging)
                GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, -1, "TransientStagingBuffer"))
            else if (bufferType == TempBufferType::Geometry)
                GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, -1, "TransientGeometryBuffer"));

            // determine buffer usage flags
            GLuint usageFlags = 0;
            if (m_type == TempBufferType::Staging)
                usageFlags |= GL_CLIENT_STORAGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT;
            else if (m_type == TempBufferType::Geometry)
                usageFlags |= GL_CLIENT_STORAGE_BIT;//GL_DYNAMIC_STORAGE_BIT;

            // setup data with buffer
            GL_PROTECT(glNamedBufferStorage(buffer, m_size, nullptr, usageFlags));
            base::mem::PoolStats::GetInstance().notifyAllocation(POOL_API_TRANSIENT_BUFFER, m_size);

            // map the buffer
            if (bufferType == TempBufferType::Staging)
            {
                GL_PROTECT(m_mappedPtr = (uint8_t*)glMapNamedBufferRange(m_glBuffer, 0, m_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
                ASSERT_EX(m_mappedPtr != nullptr, "Buffer mapping failed");
            }
        }

        TempBuffer::~TempBuffer()
        {
            // unmap
            if (m_mappedPtr)
            {
                GL_PROTECT(glUnmapNamedBuffer(m_glBuffer));
                m_mappedPtr = nullptr;
            }

            // destroy buffer
            if (0 != m_glBuffer)
            {
                GL_PROTECT(glDeleteBuffers(1, &m_glBuffer));
                m_glBuffer = 0;
            }

            // release all typed views
            for (const auto& view : m_typedViews)
                GL_PROTECT(glDeleteTextures(1, &view.m_glTextureView));
            m_typedViews.clear();

            // notify about buffer being freed
            base::mem::PoolStats::GetInstance().notifyFree(POOL_API_TRANSIENT_BUFFER, m_size);
        }

        void TempBuffer::returnToPool()
        {
            // reset the dirty state
            m_mappedWriteMinRange = m_size;
            m_mappedWriteMaxRange = 0;

            // return to pool
            m_owner->returnBuffer(this);
        }

        void TempBuffer::writeData(uint32_t offset, uint32_t size, const void* srcData)
        {
            ASSERT_EX(m_mappedPtr != nullptr, "Writes can only happen to a mapped buffer");
            ASSERT_EX(offset + size <= m_size, "Out of range write");
            memcpy(m_mappedPtr + offset, srcData, size);
            m_mappedWriteMinRange = std::min<uint32_t>(m_mappedWriteMinRange, offset);
            m_mappedWriteMaxRange = std::max<uint32_t>(m_mappedWriteMaxRange, offset + size);
        }

        void TempBuffer::flushWrites()
        {
            ASSERT_EX(m_mappedPtr != nullptr, "Writes can only happen to a mapped buffer");

            if (m_mappedWriteMaxRange > m_mappedWriteMinRange)
            {
                auto updateSize = m_mappedWriteMaxRange - m_mappedWriteMinRange;
                GL_PROTECT(glFlushMappedNamedBufferRange(m_glBuffer, m_mappedWriteMinRange, updateSize));
            }
        }

        void TempBuffer::copyDataFrom(const TempBuffer* srcBuffer, uint32_t srcOffset, uint32_t destOffset, uint32_t size)
        {
            ASSERT_EX(m_mappedPtr == nullptr, "Copy can only happen into unmapped buffer");
            ASSERT_EX(srcOffset + size <= srcBuffer->size(), "Out of bounds read");
            ASSERT_EX(destOffset + size <= this->size(), "Out of bounds write");
            GL_PROTECT(glCopyNamedBufferSubData(srcBuffer->m_glBuffer, m_glBuffer, srcOffset, destOffset, size));
        }

        void TempBuffer::flushCopies()
        {
            // nothing ? memory barrier maybe ?
        }

        ResolvedBufferView TempBuffer::resolveUntypedView(uint32_t offset, uint32_t size) const
        {
            ASSERT(offset + size <= m_size);
            return ResolvedBufferView(m_glBuffer, offset, size);
        }

        //--

        TempBufferPool::TempBufferPool(Device* drv, TempBufferType type)
            : m_device(drv)
            , m_type(type)
        {
        }

        TempBufferPool::~TempBufferPool()
        {
            ASSERT_EX(m_numAllocatedBuffers == 0, "There are still allocated transient buffers, not all frames have finished");

            for (const auto& buf : m_freeBuffers)
                delete buf.buffer;
        }

        void TempBufferPool::returnBuffer(TempBuffer* buffer)
        {
            auto lock = base::CreateLock(m_lock);

            ASSERT_EX(m_numAllocatedBuffers != 0, "Returning a buffer when there's nothing to return");
            m_numAllocatedBuffers -= 1;

            auto& entry = m_freeBuffers.emplaceBack();
            entry.timestamp = m_tickCounter++;
            entry.size = buffer->size();
            entry.buffer = buffer;
        }

        TempBuffer* TempBufferPool::allocate(uint32_t requiredSize)
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
            return new TempBuffer(this, roundedSize, m_type);
        }

        //--

    } // gl4
} // rendering
