/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "ansiAllocator.h"
#include "debugAllocator.h"
#include "linearAllocator.h"

#if defined(PLATFORM_POSIX)
    #include <sys/mman.h>
#elif defined(PLATFORM_WINDOWS)
    #include <Windows.h>
#endif
#include "poolStats.h"

namespace base
{
    namespace mem
    {
        //-----------------------------------------------------------------------------

#if defined(BUILD_RELEASE)
        typedef AnsiAllocator AllocatorClass;
        //typedef DebugAllocator AllocatorClass;
#else
        typedef AnsiAllocator AllocatorClass;
        //typedef DebugAllocator AllocatorClass;
#endif

        static AllocatorClass& GetAllocator()
        {
            static AllocatorClass* GTheAllocator = new AllocatorClass();
            return *GTheAllocator;
        }

        //-----------------------------------------------------------------------------

        void* PersistentAlloc(size_t size, size_t alignment)
        {
            static auto thePersistentPool  = new LinearAllocator(POOL_PERSISTENT);
            return thePersistentPool->alloc(size, alignment);
        }

        //-----------------------------------------------------------------------------

        void StartThreadAllocTracking()
        {
            //GetAllocator().startThreadTracking();
        }

        void FinishThreadAllocTracking()
        {
            //GetAllocator().finishThreadTracking();
        }

        void DumpMemoryLeaks()
        {
            GetAllocator().printLeaks();
        }

        void ValidateHeap()
        {
            GetAllocator().validateHeap(nullptr);
        }

        void* AllocateBlock(PoolID id, size_t size, size_t alignment, const char* debugFileName /*= nullptr*/, uint32_t debugLine /*= 0*/, const char* debugTypeName /*= nullptr*/)
        {
            if (size >= MAX_MEM_SIZE)
            {
                TRACE_ERROR("Trying to allocate buffer that is bigger than the allowed single allocation size on current platform");
                return nullptr;
            }

            return GetAllocator().allocate(id, size, alignment, debugFileName, debugLine, debugTypeName);
        }

        void FreeBlock(void* mem, const char* debugFileName /*= nullptr*/, uint32_t debugLine /*= 0*/)
        {
            return GetAllocator().deallocate(mem);
        }

        void* ResizeBlock(PoolID id, void* mem, size_t size, size_t alignment, const char* debugFileName /*= nullptr*/, uint32_t debugLine /*= 0*/, const char* debugTypeName /*= nullptr*/)
        {
            if (size >= MAX_MEM_SIZE)
            {
                TRACE_ERROR("Trying to allocate buffer that is bigger than the allowed single allocation size on current platform");
                return nullptr;
            }

            return GetAllocator().reallocate(id, mem, size, alignment, debugFileName, debugLine, debugTypeName);
        }

        //-----------------------------------------------------------------------------

        static size_t RoundUpToPageSize(size_t size, size_t pageSize)
        {
            return (size + (pageSize-1)) & ~(pageSize-1);
        }

#ifdef PLATFORM_WINAPI
        static size_t GetPageSize()
        {
            SYSTEM_INFO systemInfo;
            GetSystemInfo(&systemInfo);
            return systemInfo.dwPageSize;
        }
#endif

        void* AAllocSystemMemory(size_t size, bool largePages)
        {
#ifdef PLATFORM_WINAPI
            static auto largePageSize = GetLargePageMinimum();
            static auto smallPageSize = GetPageSize();
            auto pageSize = largePages ? largePageSize : smallPageSize;
            auto allocSize = RoundUpToPageSize(size, pageSize);
            auto ret  = VirtualAlloc(NULL, allocSize, MEM_COMMIT | MEM_RESERVE | (largePages ? MEM_LARGE_PAGES : 0), PAGE_READWRITE);
            if (largePages && ret == nullptr)
            {
                // retry without the large pages
                auto newAllocSize = RoundUpToPageSize(size, smallPageSize);
                ret = VirtualAlloc(NULL, newAllocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            }

            if (!ret)
            {
                FATAL_ERROR(TempString("System allocation of {} bytes failed", size));
            }
#elif defined(PLATFORM_POSIX)
            static auto pageSize = 4096;
            auto allocSize = RoundUpToPageSize(size, pageSize);
            auto ret  = mmap64(nullptr, allocSize, PROT_READ | PROT_WRITE,  MAP_PRIVATE | MAP_ANON, -1, 0);
            if (ret == MAP_FAILED)
            {
                FATAL_ERROR(TempString("System allocation of {} bytes failed, reason: {}", size, errno));
            }
#else
    #error "Implement this"
#endif

            PoolStats::GetInstance().notifyAllocation(POOL_SYSTEM_MEMORY, size);

            return ret;
        }

        void AFreeSystemMemory(void* page, size_t size)
        {
#ifdef PLATFORM_WINAPI
            VirtualFree(page, 0, MEM_RELEASE);
#elif defined(PLATFORM_POSIX)
            static auto pageSize = 4096;
            auto allocSize = RoundUpToPageSize(size, pageSize);
            munmap(page, allocSize);
#else
    #error "Implement this"
#endif

            PoolStats::GetInstance().notifyFree(POOL_SYSTEM_MEMORY, size);
        }

        //-----------------------------------------------------------------------------

    } // mem
} // base

