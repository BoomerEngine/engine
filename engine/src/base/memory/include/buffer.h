/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

namespace base
{
    namespace mem
    {
        /// data free function
        typedef void (*TBufferFreeFunc)(PoolTag pool, void* memory, uint64_t size);

        /// storage for buffer, can be shared
        /// NOTE: storage is never empty
        TYPE_ALIGN(16, class) BASE_MEMORY_API BufferStorage : public base::NoCopy
        {
        public:
            // get pointer to data, read only access, should NOT be freed
            INLINE uint8_t* data() const { return m_offsetToPayload ? ((uint8_t*)this + m_offsetToPayload) : (uint8_t*)m_externalPayload; }

            // get size of the data in the buffer
            INLINE uint64_t size() const { return m_size; }

            // pool this memory was allocated from
            INLINE PoolTag pool() const { return m_pool; }

            // change size
            INLINE void adjustSize(uint64_t size) { m_size = size; }

            //--

            // add a reference count
            void addRef();

            // remove reference, releasing last reference will free the memory
            void releaseRef();

            //--

            // create a buffer storage with internal storage
            static BufferStorage* CreateInternal(PoolTag pool, uint64_t size, uint32_t alignment);

            // create a buffer with external storage
            static BufferStorage* CreateExternal(PoolTag pool, uint64_t size, TBufferFreeFunc freeFunc, void* externalPayload);

        private:
            ~BufferStorage() = delete;

            uint64_t m_size = 0;
            uint16_t m_offsetToPayload = 0;
            void* m_externalPayload = nullptr;
            PoolTag m_pool = POOL_TEMP;
            TBufferFreeFunc m_freeFunc = nullptr;
            std::atomic<uint32_t> m_refCount = 1;

            void freeData();
        };

    } // mem

    namespace rtti
    {
        class CustomType;
    } // rtti

    /// a buffer of memory with custom free function
    class BASE_MEMORY_API Buffer : public base::NoCopy
    {
    public:
        //--

        static const uint64_t BUFFER_SYSTEM_MEMORY_MIN_SIZE = 4 * 1024 * 1024; // buffer size that is allocated directly from system, bypassing any allocator we have
        static const uint64_t BUFFER_SYSTEM_MEMORY_LARGE_PAGES_SIZE = 128 * 1024 * 1024; // buffer size that is allocated directly from system, bypassing any allocator we have
        static const uint32_t BUFFER_DEFAULT_ALIGNMNET = 16; // buffers are usually big, this is useful alignment

        //--

        INLINE Buffer() {};
        INLINE Buffer(std::nullptr_t) {};
        INLINE Buffer(mem::BufferStorage* storage) : m_storage(storage) {};
        Buffer(const Buffer& other);
        Buffer(Buffer&& other);
        ~Buffer(); // frees the data

        Buffer& operator=(const Buffer& other);
        Buffer& operator=(Buffer&& other);

        // memcmp
        bool operator==(const Buffer& other) const;
        bool operator!=(const Buffer& other) const;

        //--

        // empty ?
        INLINE bool empty() const { return m_storage == nullptr; }

        // cast to bool check
        INLINE operator bool() const { return m_storage != nullptr; }

        // get pointer to data, read only access, should NOT be freed
        INLINE uint8_t* data() const { return m_storage ? m_storage->data() : nullptr; }

        // get size of the data in the buffer
        INLINE uint64_t size() const { return m_storage ? m_storage->size() : 0; }

        //--

        // free the data in the buffer
        void reset();

        // reinitialize with new memory
        // NOTE: can fail if we don't have enough memory
        bool init(PoolTag pool, uint64_t size, uint32_t alignment = 1, const void* dataToCopy = nullptr, uint64_t dataSizeToCopy = INDEX_MAX64);

        // reinitialize with zeros
        // NOTE: can fail if we don't have enough memory
        bool initWithZeros(PoolTag pool, uint64_t size, uint32_t alignment = 1);

        // change buffer size without reallocating memory (advanced shit)
        void adjustSize(uint32_t size);

        //--

        // print to stream (prints as BASE64)
        void print(IFormatStream& f) const;

        //--

        // create buffer from content
        static Buffer Create(PoolTag pool, uint64_t size, uint32_t alignment = 0, const void* dataToCopy = nullptr, uint64_t dataSizeToCopy = INDEX_MAX64);

        // create zero initialized buffer
        static Buffer CreateZeroInitialized(PoolTag pool, uint64_t size, uint32_t alignment = 0);

        // create buffer from external memory
        static Buffer CreateExternal(PoolTag pool, uint64_t size, void* externalData, mem::TBufferFreeFunc freeFunc = nullptr);

        // create buffer from system memory (bypassing allocator), do it regardless of the buffer's size
        static Buffer CreateInSystemMemory(PoolTag pool, uint64_t size, const void* dataToCopy = nullptr, uint64_t dataSizeToCopy = INDEX_MAX64);

        //--

    private:
        mem::BufferStorage* m_storage = nullptr; // refcounted

        static mem::BufferStorage* CreateBestStorageForSize(PoolTag pool, uint64_t size, uint32_t alignment);
        static mem::BufferStorage* CreateSystemMemoryStorage(PoolTag pool, uint64_t size);
    };

} // base