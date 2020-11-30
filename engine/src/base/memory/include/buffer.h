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

    /// data free function
    typedef void (*TBufferFreeFunc)(PoolTag pool, void* memory, uint64_t size);

    /// storage for buffer, can be shared
    /// NOTE: storage is never empty
    class BufferStorage;

    //--

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
        INLINE Buffer(BufferStorage* storage) : m_storage(storage) {};
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

        //--

        // get pointer to data, read only access, should NOT be freed
        uint8_t* data() const;

        // get size of the data in the buffer
        uint64_t size() const;

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
        static Buffer CreateExternal(PoolTag pool, uint64_t size, void* externalData, TBufferFreeFunc freeFunc = nullptr);

        // create buffer from system memory (bypassing allocator), do it regardless of the buffer's size
        static Buffer CreateInSystemMemory(PoolTag pool, uint64_t size, const void* dataToCopy = nullptr, uint64_t dataSizeToCopy = INDEX_MAX64);

		//--

    private:
        BufferStorage* m_storage = nullptr; // refcounted

        static BufferStorage* CreateBestStorageForSize(PoolTag pool, uint64_t size, uint32_t alignment);
        static BufferStorage* CreateSystemMemoryStorage(PoolTag pool, uint64_t size);
    };

    //--

    enum class CompressionType : uint8_t
    {
        // no compression, the compression API will work but probably it's not the best use of the API :)
        Uncompressed = 0,

        // zlib compression with internal header to store the size of the compressed data (THIS IS MAKING THE DATA INCOMPATBILE WITH NORMAL ZLIB)
        // NOTE: zlib is zlow
        Zlib,

        // LZ4, fast decompression, includes header
        LZ4,

        // High-compression LZ4, fast decompression, includes header
        LZ4HC,

        // Last compression type
        MAX,
    };

    // compress a memory block, returns memory block that should be freed with FreeBlock
    extern BASE_MEMORY_API void* Compress(CompressionType ct, const void* uncompressedDataPtr, uint64_t uncompressedSize, uint64_t& outCompressedSize, PoolTag pool);

    // compress a memory block to buffer
    extern BASE_MEMORY_API Buffer Compress(CompressionType ct, const void* uncompressedDataPtr, uint64_t uncompressedSize, PoolTag pool);

    // compress a memory block to buffer
    extern BASE_MEMORY_API Buffer Compress(CompressionType ct, const Buffer& uncompressedData, PoolTag pool);

    // compress a memory block in-place, output buffer must be at least as big as the input data
    extern BASE_MEMORY_API bool Compress(CompressionType ct, const void* uncompressedDataPtr, uint64_t uncompressedSize, void* compressedDataPtr, uint64_t compressedDataMaxSize, uint64_t& outCompressedRealSize);

    // decompress a memory block into a buffer
    extern BASE_MEMORY_API bool Decompress(CompressionType ct, const void* compressedDataPtr, uint64_t compressedSize, void* decompressedDataPtr, uint64_t decompressedSize);

    // decompress a memory block into a buffer
    extern BASE_MEMORY_API Buffer Decompress(CompressionType ct, const void* compressedDataPtr, uint64_t compressedSize, uint64_t decompressedSize, PoolTag pool);

    //--

} // base