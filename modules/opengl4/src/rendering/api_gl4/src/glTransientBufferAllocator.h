/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\transient #]
***/

#pragma once

#include "glImage.h"
#include "glBuffer.h"

namespace rendering
{
    namespace gl4
    {
        /// type of transient buffer
        enum class TransientBufferType : uint8_t
        {
            Staging, // for making copies into another data
            Constants, // constant data
            Geometry, // vertex/index
            Storage, // general storage buffer
        };

        /// space in the transient buffer
        /// NOTE: must be used within a given submission frame
        class TransientBuffer;
;
        /// allocation context, used by single command buffer while allocating parameters
        /// each command buffers have unique buffers, destroyed when command buffer completes execution
        /// NOTE: only the storage buffer is mapped, all other buffers require a copy
        class TransientBufferAllocator
        {
        public:
            TransientBufferAllocator(Driver* drv, uint32_t initialPageSize, uint32_t pageSizeIncrement, TransientBufferType bufferType);
            ~TransientBufferAllocator();

            INLINE TransientBufferType bufferType() const { return m_bufferType; }
            INLINE uint32_t pageSize() const { return m_pageSize; }

            //--

            // allocate a buffer
            TransientBuffer* allocate(uint32_t requiredSize);

            //--

        private:
            // owning driver
            Driver* m_driver;

            // type of the buffer we manage
            TransientBufferType m_bufferType;

            // size of the page
            uint32_t m_pageSize;
            uint32_t m_pageSizeIncrement;

            // free buffers
            base::Array<TransientBuffer*> m_freeBuffers;

            // number of allocated buffers
            volatile uint32_t m_numAllocatedBuffers;

            // access lock
            base::SpinLock m_lock;

            //--

            void returnBuffer(TransientBuffer* buffer);

            //--

            friend class TransientBuffer;
        };

        /// a buffer allocated from transient memory
        class TransientBuffer : public base::NoCopy
        {
        public:
            TransientBuffer(TransientBufferAllocator* owner, uint32_t size, TransientBufferType bufferType);
            ~TransientBuffer();

            INLINE TransientBufferType type() const { return m_type; }
            INLINE uint32_t size() const { return m_size; }

            INLINE bool mapped() const { return m_mappedPtr; }

            //--

            // we are done with the buffer, return buffer to pool
            void returnToPool();

            //--

            // write data, requires the buffer to be mapped
            void writeData(uint32_t offset, uint32_t size, const void* srcData);

            // signal that all writes were completed
            void flushWrites();

            //--

            // copy data into other transient buffer, usually this is done from the Storage one to others
            void copyDataFrom(const TransientBuffer* srcBuffer, uint32_t srcOffset, uint32_t destOffset, uint32_t size);

            // signal that all of the copies are finished
            void flushCopies();

            //--

            // resolve untyped buffer view of the buffer
            ResolvedBufferView resolveUntypedView(uint32_t offset, uint32_t size) const;

            // resolve typed buffer view of the buffer
            ResolvedFormatedView resolveTypedView(uint32_t offset, uint32_t size, ImageFormat format) const;

        private:
            GLuint m_glBuffer; // buffer on the GPU
            uint32_t m_size; // size of the buffer

            uint8_t* m_mappedPtr; // pointer to local data, if mapped
            uint32_t m_mappedWriteMinRange; // first written byte
            uint32_t m_mappedWriteMaxRange; // last written byte

            TransientBufferType m_type; // type of the buffer
            TransientBufferAllocator* m_allocator; // owning allocator

            struct TypedView
            {
                GLuint m_glTextureView;
                ImageFormat m_dataFormat;
                uint32_t m_dataOffset;

                INLINE TypedView(GLuint glTextureView, ImageFormat format, uint32_t offset)
                    : m_glTextureView(glTextureView)
                    , m_dataFormat(format)
                    , m_dataOffset(offset)
                {}
            };

            mutable base::Array<TypedView> m_typedViews;
        };

    } // gl4
} // driver
