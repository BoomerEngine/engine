/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\ansi #]
***/

#include "build.h"
#include "ansiAllocator.h"

#ifdef PLATFORM_POSIX
    #include <malloc.h>
#endif

BEGIN_BOOMER_NAMESPACE_EX(mem)

//--

void AnsiAllocator::printLeaks()
{
    // nothing
}

void AnsiAllocator::validateHeap(void* freedPtr)
{
    // nothing
}

void* AnsiAllocator::allocate(PoolTag id, size_t size, size_t alignment, const char* typeName)
{
#if defined(PLATFORM_MSVC) && defined(BUILD_DEBUG)
    return _aligned_malloc_dbg(size, alignment, typeName, 0);
#elif defined(PLATFORM_MSVC)
    return _aligned_malloc(size, alignment);
#else
    return aligned_alloc(alignment, size);
#endif
}

void AnsiAllocator::deallocate(void* mem)
{
    if (mem != nullptr)
    {
#if defined(PLATFORM_MSVC) && defined(BUILD_DEBUG)
        _aligned_free_dbg(mem);
#elif defined(PLATFORM_MSVC)
        _aligned_free_dbg(mem);
        //_aligned_free(mem);
#else
        return free(mem);
#endif
    }
}

void* AnsiAllocator::reallocate(PoolTag id, void* mem, size_t newSize, size_t alignment, const char* typeName)
{
    if (newSize == 0)
    {
        deallocate(mem);
        return nullptr;
    }
    else if (mem == nullptr)
    {
        return allocate(id, newSize, alignment, typeName);
    }
    else
    {
#if defined(PLATFORM_MSVC)
#if defined(BUILD_DEBUG)
        void* ret = _aligned_realloc_dbg(mem, newSize, alignment, typeName, 0);
#else
        void* ret = _aligned_realloc(mem, newSize, alignment);
#endif
#elif defined(PLATFORM_POSIX)
        void* ret = allocate(id, newSize, alignment);
        size_t currentSize = malloc_usable_size(mem);
        memcpy(ret, mem, std::min<size_t>(currentSize, newSize));
        deallocate(mem);
#else
#error "Please provide ALIGNED realloc on this platform"
#endif
        ASSERT(ret != nullptr);
        return ret;
    }
}

//--

END_BOOMER_NAMESPACE_EX(mem)
