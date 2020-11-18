/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "glBuffer.h"

namespace rendering
{
    namespace gl4
    {
        //---

        /// a buffer allocated from transient memory
        class TempBuffer : public base::mem::GlobalPoolObject<POOL_API_RUNTIME>
        {
        public:
            TempBuffer(TempBufferPool* owner, uint32_t size, TempBufferType bufferType);
            ~TempBuffer();

            //--

            INLINE TempBufferType type() const { return m_type; }
            INLINE uint32_t size() const { return m_size; }

            INLINE bool mapped() const { return m_mappedPtr != nullptr; }

            //--

            void returnToPool();

            //--

            void writeData(uint32_t offset, uint32_t size, const void* srcData);
            void flushWrites();

            //--

            void copyDataFrom(const TempBuffer* srcBuffer, uint32_t srcOffset, uint32_t destOffset, uint32_t size);
            void flushCopies();

            //--

            ResolvedBufferView resolveUntypedView(uint32_t offset, uint32_t size) const;

        private:
            GLuint m_glBuffer; // buffer on the GPU
            uint32_t m_size; // size of the buffer

            uint8_t* m_mappedPtr; // pointer to local data, if mapped
            uint32_t m_mappedWriteMinRange; // first written byte
            uint32_t m_mappedWriteMaxRange; // last written byte

            TempBufferType m_type; // type of the buffer
            TempBufferPool* m_owner; // owning allocator

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

        //---

        /// pool for frame-related temporary buffers
        /// each command buffers have unique buffers, destroyed when command buffer completes execution
        /// NOTE: only the storage buffer is mapped, all other buffers require a copy
        class TempBufferPool : public base::mem::GlobalPoolObject<POOL_API_RUNTIME>
        {
        public:
            TempBufferPool(Device* drv, TempBufferType type);
            ~TempBufferPool();

            INLINE TempBufferType type() const { return m_type; }

            TempBuffer* allocate(uint32_t requiredSize);

            void returnBuffer(TempBuffer* buffer);

        private:
            base::SpinLock m_lock;

            Device* m_device;

            TempBufferType m_type;

            uint32_t m_numAllocatedBuffers = 0;
            uint32_t m_tickCounter = 0;

            struct Entry
            {
                uint32_t size = 0;
                uint32_t timestamp = 0;
                TempBuffer* buffer = 0;
            };

            base::Array<Entry> m_freeBuffers;
        };

        //---

    } // gl4
} // rendering