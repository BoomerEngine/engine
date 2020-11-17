/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\transient #]
***/

#include "build.h"
#include "glTransientBufferAllocator.h"
#include "glDriver.h"
#include "glUtils.h"

namespace rendering
{
    namespace gl4
    {

        //--

        TransientBufferAllocator::TransientBufferAllocator(Driver* drv, uint32_t initialPageSize, uint32_t pageSizeIncrement, TransientBufferType bufferType)
            : m_driver(drv)
            , m_pageSize(initialPageSize)
            , m_pageSizeIncrement(pageSizeIncrement)
            , m_bufferType(bufferType)
            , m_numAllocatedBuffers(0)
        {
        }

        TransientBufferAllocator::~TransientBufferAllocator()
        {
            ASSERT_EX(m_numAllocatedBuffers == 0, "There are still allocated transient buffers, not all frames have finished");

            // delete pooled buffers
            for (auto ptr  : m_freeBuffers)
                MemDelete(ptr);
            m_freeBuffers.clear();
        }

        void TransientBufferAllocator::returnBuffer(TransientBuffer* buffer)
        {
            auto lock = base::CreateLock(m_lock);

            // uncount free buffers
            ASSERT_EX(m_numAllocatedBuffers != 0, "Returning a buffer when there's nothing to return");
            m_numAllocatedBuffers -= 1;

            // if the buffer is smaller than the current minimal page size we consider it to small
            if (buffer->size() < m_pageSize)
            {
                TRACE_INFO("Transient buffer discarded since it's too small now");
                MemDelete(buffer);
            }
            else
            {
                // add to free pool
                m_freeBuffers.pushBack(buffer);
            }
        }

        TransientBuffer* TransientBufferAllocator::allocate(uint32_t requiredSize)
        {
            auto lock = base::CreateLock(m_lock);

            // adjust the page size
            if (requiredSize > m_pageSize)
            {
                // get the next ok size
                auto prevPageSize = m_pageSize;
                while (requiredSize > m_pageSize)
                    m_pageSize += m_pageSizeIncrement;

                TRACE_INFO("Adjusted transient buffer size {}->{} (type {})", prevPageSize, m_pageSize, (uint8_t) m_bufferType);

                // our all current free buffers are to be discarded
                for (auto ptr  : m_freeBuffers)
                    MemDelete(ptr);
                m_freeBuffers.clear();
            }

            // use the free buffer if we have one
            if (!m_freeBuffers.empty())
            {
                auto buffer = m_freeBuffers.back();
                m_freeBuffers.popBack();
                ASSERT(buffer->size() >= requiredSize);
                m_numAllocatedBuffers += 1;
                return buffer;
            }

            // create a new transient buffer
            auto buffer  = MemNew(TransientBuffer, this, m_pageSize, m_bufferType);
            m_numAllocatedBuffers += 1;
            return buffer;
        }

        //---

        TransientBuffer::TransientBuffer(TransientBufferAllocator* owner, uint32_t size, TransientBufferType bufferType)
            : m_allocator(owner)
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
            if (bufferType == TransientBufferType::Storage) 
                GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, -1, "TransientStorageBuffer"))
            else if (bufferType == TransientBufferType::Staging)
                GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, -1, "TransientStagingBuffer"))
            else if (bufferType == TransientBufferType::Geometry)
                GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, -1, "TransientGeometryBuffer"));

            // determine buffer usage flags
            GLuint usageFlags = 0;
            if (m_type == TransientBufferType::Staging)
                usageFlags |= GL_CLIENT_STORAGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT;
            else if (m_type == TransientBufferType::Geometry)
                usageFlags |= GL_CLIENT_STORAGE_BIT;//GL_DYNAMIC_STORAGE_BIT;

            // setup data with buffer
            GL_PROTECT(glNamedBufferStorage(buffer, m_size, nullptr, usageFlags));
            base::mem::PoolStats::GetInstance().notifyAllocation(POOL_API_TRANSIENT_BUFFER, m_size);

            // map the buffer
            if (bufferType == TransientBufferType::Staging)
            {
                GL_PROTECT(m_mappedPtr = (uint8_t*)glMapNamedBufferRange(m_glBuffer, 0, m_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
                ASSERT_EX(m_mappedPtr != nullptr, "Buffer mapping failed");
            }
        }

        TransientBuffer::~TransientBuffer()
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

        void TransientBuffer::returnToPool()
        {
            // reset the dirty state
            m_mappedWriteMinRange = m_size;
            m_mappedWriteMaxRange = 0;

            // return to pool
            m_allocator->returnBuffer(this);
        }

        void TransientBuffer::writeData(uint32_t offset, uint32_t size, const void* srcData)
        {
            ASSERT_EX(m_mappedPtr != nullptr, "Writes can only happen to a mapped buffer");
            ASSERT_EX(offset + size <= m_size, "Out of range write");
            memcpy(m_mappedPtr + offset, srcData, size);
            m_mappedWriteMinRange = std::min<uint32_t>(m_mappedWriteMinRange, offset);
            m_mappedWriteMaxRange = std::max<uint32_t>(m_mappedWriteMaxRange, offset + size);
        }

        void TransientBuffer::flushWrites()
        {
            ASSERT_EX(m_mappedPtr != nullptr, "Writes can only happen to a mapped buffer");

            if (m_mappedWriteMaxRange > m_mappedWriteMinRange)
            {
                auto updateSize = m_mappedWriteMaxRange - m_mappedWriteMinRange;
                GL_PROTECT(glFlushMappedNamedBufferRange(m_glBuffer, m_mappedWriteMinRange, updateSize));
            }
        }

        void TransientBuffer::copyDataFrom(const TransientBuffer* srcBuffer, uint32_t srcOffset, uint32_t destOffset, uint32_t size)
        {
            ASSERT_EX(m_mappedPtr == nullptr, "Copy can only happen into unmapped buffer");
            ASSERT_EX(srcOffset + size <= srcBuffer->size(), "Out of bounds read");
            ASSERT_EX(destOffset + size <= this->size(), "Out of bounds write");
            GL_PROTECT(glCopyNamedBufferSubData(srcBuffer->m_glBuffer, m_glBuffer, srcOffset, destOffset, size));
        }

        void TransientBuffer::flushCopies()
        {
            // nothing ? memory barier maybe ?
        }

        ResolvedBufferView TransientBuffer::resolveUntypedView(uint32_t offset, uint32_t size) const
        {
            ASSERT(offset + size <= m_size);
            return ResolvedBufferView(m_glBuffer, offset, size);
        }

        ResolvedFormatedView TransientBuffer::resolveTypedView(uint32_t offset, uint32_t size, ImageFormat dataFormat) const
        {
            // skip on invalid buffers
            if (m_glBuffer == 0)
                return ResolvedFormatedView();

            // we should have a typed view of the buffer
            ASSERT_EX(dataFormat != ImageFormat::UNKNOWN, "Trying to resolve a typed view without specifying a type");

            // translate format
            auto imageFormat = TranslateImageFormat(dataFormat);

            // use existing view
            for (auto& typedView : m_typedViews)
                if (typedView.m_dataFormat == dataFormat && typedView.m_dataOffset == offset)
                    return ResolvedFormatedView(typedView.m_glTextureView, imageFormat);

            // create a view
            GLuint glTextureView = 0;
            GL_PROTECT(glCreateTextures(GL_TEXTURE_BUFFER, 1, &glTextureView));
            GL_PROTECT(glTextureBufferRange(glTextureView, imageFormat, m_glBuffer, offset, size));

            // save
            m_typedViews.emplaceBack(glTextureView, dataFormat, offset);
            return ResolvedFormatedView(glTextureView, imageFormat);
        }

    } // gl4
} // driver
