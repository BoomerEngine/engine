/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

namespace rendering
{
    namespace api
    {
        //---

        /// a transient buffer that is used during a given frame and then goes back to pool as soon as frame finishes on the GPU
        class RENDERING_API_COMMON_API IBaseTransientBuffer : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_RUNTIME)

        public:
			IBaseTransientBuffer(IBaseTransientBufferPool* owner, uint32_t size, TransientBufferType type);
            virtual ~IBaseTransientBuffer();

            //--

			INLINE PlatformPtr ptr() const { return m_apiPointer; }

            INLINE TransientBufferType type() const { return m_type; }
            INLINE uint32_t size() const { return m_size; }

            INLINE bool mapped() const { return m_mappedPtr != nullptr; }

            //--

            void returnToPool();

            void writeData(uint32_t offset, uint32_t size, const void* srcData);

            virtual void copyDataFrom(const IBaseTransientBuffer* srcBuffer, uint32_t srcOffset, uint32_t destOffset, uint32_t size);

            void flush();

            //--

		protected:
            uint8_t* m_mappedPtr = nullptr; // pointer to local data, if mapped
            uint32_t m_mappedWriteMinRange = 0; // first written byte
            uint32_t m_mappedWriteMaxRange = 0; // last written byte

			PlatformPtr m_apiPointer;

			virtual void flushInnerWrites(uint32_t offset, uint32_t size) = 0;

		private:
			uint32_t m_size; // size of the buffer

			TransientBufferType m_type; // type of the buffer
			IBaseTransientBufferPool* m_owner; // owning allocator
        };

        //---

        /// pool for frame-related temporary buffers
        /// each command buffers have unique buffers, destroyed when command buffer completes execution
        /// NOTE: only the storage buffer is mapped, all other buffers require a copy
        class RENDERING_API_COMMON_API IBaseTransientBufferPool : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_RUNTIME)

        public:
			IBaseTransientBufferPool(IBaseThread* owner, TransientBufferType type);
            virtual ~IBaseTransientBufferPool();

            INLINE TransientBufferType type() const { return m_type; }

			IBaseTransientBuffer* allocate(uint32_t requiredSize);

            void returnBuffer(IBaseTransientBuffer* buffer);

		protected:
			virtual IBaseTransientBuffer* createBuffer(uint32_t size) = 0;

        private:
            base::SpinLock m_lock;

			IBaseThread* m_owner;
            TransientBufferType m_type;

            uint32_t m_numAllocatedBuffers = 0;
            uint32_t m_tickCounter = 0;

            struct Entry
            {
                uint32_t size = 0;
                uint32_t timestamp = 0;
				IBaseTransientBuffer* buffer = 0;
            };

            base::Array<Entry> m_freeBuffers;
        };

        //---

    } // gl4
} // rendering