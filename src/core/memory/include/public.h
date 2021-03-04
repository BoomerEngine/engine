/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

// Glue headers and logic
#include "core_memory_glue.inl"

//-----------------------------------------------------------------------------

// Some memory pools are predefined
enum PoolTag
{
#define BOOMER_DECLARE_POOL(name, group, size) name,
#include "poolNames.inl"
    POOL_MAX,
};

//-----------------------------------------------------------------------------

// This needs to be declared so we do not redefine new/delete operators
#define __PLACEMENT_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE

//-----------------------------------------------------------------------------

BEGIN_BOOMER_NAMESPACE()

//--

// print memory leaks
extern CORE_MEMORY_API void DumpMemoryLeaks();

// validate heap state
extern CORE_MEMORY_API void ValidateHeap();

//--

//! Allocate page of memory directly from the system
//! NOTE: this memory is not tracked
extern CORE_MEMORY_API void* AllocSystemMemory(size_t size, bool largePages);

//! Free page of memory directly to the system
extern CORE_MEMORY_API void FreeSystemMemory(void* page, size_t size);

//--

//! Allocate memory
extern CORE_MEMORY_API void* AllocateBlock(PoolTag id, size_t size, size_t alignment = 8, const char* typeName=nullptr);

//! Resize allocated memory block
extern CORE_MEMORY_API void* ResizeBlock(PoolTag id, void* mem, size_t size, size_t alignment = 8, const char* typeName=nullptr);

//! Free allocated memory
extern CORE_MEMORY_API void FreeBlock(void* mem);

//--

template< PoolTag PT = POOL_DEFAULT, typename T = void >
struct GlobalPool
{
    static const PoolTag TAG = PT;

    static ALWAYS_INLINE T* Alloc(size_t size, size_t alignment = 8)
    {
#if !defined(BUILD_FINAL)
        return (T*) AllocateBlock(PT, size, alignment, typeid(T).raw_name());
#else
        return (T*) AllocateBlock(PT, size, alignment);
#endif
    }

    static ALWAYS_INLINE T* AllocN(size_t count, size_t alignment = 0)
    {
#if !defined(BUILD_FINAL)
        return (T*) AllocateBlock(PT, count * sizeof(T), alignment ? alignment : alignof(T), typeid(T).raw_name());
#else
        return (T*) AllocateBlock(PT, count * sizeof(T), alignment ? alignment : alignof(T));
#endif
    }

    static ALWAYS_INLINE void Free(void* mem)
    {
        return FreeBlock(mem);
    }

    static ALWAYS_INLINE T* Resize(void* mem, size_t size, size_t alignment = 8)
    {
#if !defined(BUILD_FINAL)
        return (T*) ResizeBlock(PT, mem, size, alignment, typeid(T).raw_name());
#else
        return (T*) ResizeBlock(PT, mem, size, alignment);
#endif
    }
};

//--

#define RTTI_DECLARE_POOL(TAG) \
    public: \
    using ClassAllocationPool = ::boomer::GlobalPool<TAG>; \
    ALWAYS_INLINE static void* AllocClassMem(std::size_t size, std::size_t align) { return ClassAllocationPool::Alloc(size, align); } \
    ALWAYS_INLINE static void FreeClassMem(void* ptr) { ClassAllocationPool::Free(ptr); } \
    ALWAYS_INLINE void* operator new(std::size_t count) { return AllocClassMem(count, __STDCPP_DEFAULT_NEW_ALIGNMENT__); } \
    ALWAYS_INLINE void* operator new[](std::size_t count) { return AllocClassMem(count, __STDCPP_DEFAULT_NEW_ALIGNMENT__); } \
    ALWAYS_INLINE void* operator new(std::size_t count, std::align_val_t al) { return AllocClassMem(count, (size_t)al); } \
    ALWAYS_INLINE void* operator new[](std::size_t count, std::align_val_t al) { return AllocClassMem(count, (size_t)al); } \
    ALWAYS_INLINE void operator delete(void* ptr, std::size_t sz) { FreeClassMem(ptr); } \
    ALWAYS_INLINE void operator delete[](void* ptr, std::size_t sz) { FreeClassMem(ptr); } \
    ALWAYS_INLINE void operator delete(void* ptr, std::size_t sz, std::align_val_t al) { FreeClassMem(ptr); } \
    ALWAYS_INLINE void operator delete[](void* ptr, std::size_t sz, std::align_val_t al) { FreeClassMem(ptr); } \
    ALWAYS_INLINE void operator delete(void* ptr) { FreeClassMem(ptr); } \
    ALWAYS_INLINE void operator delete(void* ptr, std::align_val_t al) { FreeClassMem(ptr); } \
    ALWAYS_INLINE void operator delete[](void* ptr) { FreeClassMem(ptr); } \
    ALWAYS_INLINE void operator delete[](void* ptr, std::align_val_t al) { FreeClassMem(ptr); } \
    ALWAYS_INLINE void* operator new(std::size_t count, void* pt) { return pt; } \
    ALWAYS_INLINE void operator delete(void *pt, void* pt2) {} \
    private:\

//--

class LinearAllocator;
class PageAllocator;
class PageCollection;

// get default page allocator for given pool
extern CORE_MEMORY_API PageAllocator& DefaultPageAllocator(PoolTag pool = POOL_TEMP);

//--

END_BOOMER_NAMESPACE()

//--

using ClassAllocationPool = boomer::GlobalPool<POOL_OBJECT>;


