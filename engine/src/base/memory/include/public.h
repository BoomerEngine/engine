/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

// Glue headers and logic
#include "base_memory_glue.inl"

//-----------------------------------------------------------------------------

// Some memory pools are predefined
enum PredefinedPoolID
{
    POOL_DEFAULT = 0,
    POOL_TEMP,
    POOL_NEW,
    POOL_PERSISTENT,
    POOL_IO,
    POOL_CONTAINERS,
    POOL_SERIALIZATION,
    POOL_RTTI,
    POOL_SCRIPTS,
    POOL_NET,
    POOL_IMAGE,
    POOL_STRINGS,
    POOL_XML,
    POOL_STRINGIDS,
    POOL_PTR,
    POOL_OBJECTS,
    POOL_RESOURCES,
    POOL_MEM_BUFFER,
    POOL_DATA_BUFFER,
    POOL_ASYNC_BUFFER,
    POOL_SYSTEM_MEMORY,
};

//-----------------------------------------------------------------------------

// Some IDs
#include "poolID.h"

//-----------------------------------------------------------------------------

// This needs to be declared so we do not redefine new/delete operators
#define __PLACEMENT_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE

//-----------------------------------------------------------------------------

namespace base
{
    namespace mem
    {

        // print memory leaks
        extern BASE_MEMORY_API void DumpMemoryLeaks();

        // validate heap state
        extern BASE_MEMORY_API void ValidateHeap();

        // allocate memory from global persistent pool
        // NOTE: only one-time initialization stuff should go here (singletons, global maps, etc)
        // NOTE: this pool has limited size (few MBs)
        extern BASE_MEMORY_API void* PersistentAlloc(size_t size, size_t alignment = 8);

        //! Allocate memory
        extern BASE_MEMORY_API void* AllocateBlock(PoolID id, size_t size, size_t alignment = 8, const char* debugFileName = nullptr, uint32_t debugLine = 0, const char* debugTypeName = nullptr);

        //! Free allocated memory
        extern BASE_MEMORY_API void FreeBlock(void* mem, const char* debugFileName = nullptr, uint32_t debugLine = 0);

        //! Resize allocated memory block
        extern BASE_MEMORY_API void* ResizeBlock(PoolID id, void* mem, size_t size, size_t alignment = 8, const char* debugFileName = nullptr, uint32_t debugLine = 0, const char* debugTypeName = nullptr);

        //! Allocate page of memory directly from the system
        //! NOTE: this memory is not tracked
        extern BASE_MEMORY_API void* AAllocSystemMemory(size_t size, bool largePages);

        //! Free page of memory directly to the system
        extern BASE_MEMORY_API void AFreeSystemMemory(void* page, size_t size);

        //---

        //! Start tracking allocations happening on this thread
        extern BASE_MEMORY_API void StartThreadAllocTracking();

        //! Finish tracking allocation happening on this thread, report all unfreed allocations
        extern BASE_MEMORY_API void FinishThreadAllocTracking();

        //---

        //! Check overlap between two memory regions
        INLINE static bool CheckOverlap(const uint8_t* startA, const uint8_t* endA, const uint8_t* startB, const uint8_t* endB)
        {
            return !(startA >= endB || startB >= endA);
        }

        //--        

    } // namespace mem 
} // base

//-----------------------------------------------------------------------------
// GLOBAL MEMORY INTERFACE

// alloc memory via the new operator from the new pool
// this is the only legal way to allocate something that will be freed by the MemDelete
#ifdef BUILD_RELEASE
    #define MemNew(T, ...) NoAddRef<T>(new (base::mem::AllocateBlock(POOL_NEW, sizeof(T), __alignof(T))) T(__VA_ARGS__))
    #define MemNewPool(pool, T, ...) NoAddRef<T>(new (base::mem::AllocateBlock(pool, sizeof(T), __alignof(T))) T(__VA_ARGS__))
#else
    #define MemNew(T, ...) NoAddRef<T>(new (base::mem::AllocateBlock(POOL_NEW, sizeof(T), __alignof(T), __FILE__, __LINE__, typeid(typename std::remove_cv<T>::type).name())) T(__VA_ARGS__))
    #define MemNewPool(pool, T, ...) NoAddRef<T>(new (base::mem::AllocateBlock(pool, sizeof(T), __alignof(T), __FILE__, __LINE__, typeid(typename std::remove_cv<T>::type).name())) T(__VA_ARGS__))
#endif

// alloc raw memory, supports tracking
#ifdef BUILD_RELEASE
    #define MemAlloc(pool, size, alignment) base::mem::AllocateBlock(pool, size, alignment, nullptr, 0, nullptr)
    #define MemRealloc(pool, ptr, size, alignment) base::mem::ResizeBlock(pool, ptr, size, alignment, nullptr, 0, nullptr)
    #define MemFree(ptr) base::mem::FreeBlock(ptr)
#else
    #define MemAlloc(pool, size, alignment) base::mem::AllocateBlock(pool, size, alignment, __FILE__, __LINE__)
    #define MemRealloc(pool, ptr, size, alignment) base::mem::ResizeBlock(pool, ptr, size, alignment, __FILE__, __LINE__)
    #define MemFree(ptr) base::mem::FreeBlock(ptr)
#endif

  // allocate in the global "leak" pool - for global that we do not intend to free, ever
  // NOTE: only one-time initialization stuff should go here
  // NOTE: this pool has limited size (few MBs)
#define PersistentNew(T, ...) (new (base::mem::PersistentAlloc(sizeof(T), __alignof(T))) T(__VA_ARGS__))

// delete stuff previously allocated via the MemNew
// NOTE: strictly only allocations done by the New
template< typename T >
INLINE void MemDelete(T* ptr)
{
    if (ptr != nullptr)
    {
        ptr->~T();
        MemFree((void*)ptr);
    }
}

//-----------------------------------------------------------------------------

// Public memory system
#include "buffer.h"

namespace base
{
    namespace mem
    {
        //--

        // type of the compression
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

        // compress a memory block, returns memory block that should be freed with MemFree
        extern BASE_MEMORY_API void* Compress(CompressionType ct, const void* uncompressedDataPtr, uint64_t uncompressedSize, uint64_t& outCompressedSize, PoolID pool = PoolID());

        // compress a memory block to buffer
        extern BASE_MEMORY_API Buffer Compress(CompressionType ct, const void* uncompressedDataPtr, uint64_t uncompressedSize, PoolID pool = PoolID());

        // compress a memory block to buffer
        extern BASE_MEMORY_API Buffer Compress(CompressionType ct, const Buffer& uncompressedData, PoolID pool = PoolID());

        // compress a memory block in-place, output buffer must be at least as big as the input data
        extern BASE_MEMORY_API bool Compress(CompressionType ct, const void* uncompressedDataPtr, uint64_t uncompressedSize, void* compressedDataPtr, uint64_t compressedDataMaxSize, uint64_t& outCompressedRealSize);

        // decompress a memory block into a buffer
        extern BASE_MEMORY_API bool Decompress(CompressionType ct, const void* compressedDataPtr, uint64_t compressedSize, void* decompressedDataPtr, uint64_t decompressedSize);

        // decompress a memory block into a buffer
        extern BASE_MEMORY_API Buffer Decompress(CompressionType ct, const void* compressedDataPtr, uint64_t compressedSize, uint64_t decompressedSize, PoolID pool = PoolID());

        //--

        class LinearAllocator;
        class PageAllocator;
        class PageCollection;

        // get default page allocator for given pool
        extern BASE_MEMORY_API PageAllocator& DefaultPageAllocator(PoolID pool = POOL_TEMP);

        //--

    } // mem
} // base

