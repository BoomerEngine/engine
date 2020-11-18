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
enum PoolTag
{
    POOL_DEFAULT = 0,
    POOL_TEMP,
    POOL_NEW,
    POOL_PERSISTENT,
    POOL_IO,
    POOL_IO_OUTSTANDING,
    POOL_CONTAINERS,
    POOL_HASH_BUCKETS,
    POOL_SERIALIZATION,
    POOL_RTTI,
    POOL_SCRIPTS,
    POOL_SCRIPTED_OBJECT,
    POOL_SCRIPTED_DEFAULT_OBJECT,
    POOL_SCRIPT_CODE,
    POOL_SCRIPT_STREAM,
    POOL_NET,
    POOL_NET_MESSAGE,
    POOL_NET_MESSAGES,
    POOL_NET_REASSEMBLER,
    POOL_NET_REPLICATION,
    POOL_NET_MODEL,
    POOL_COOKING,
    POOL_IMPORT,
    POOL_IMAGE,
    POOL_STRINGS,
    POOL_PIPES,
    POOL_THREADS,
    POOL_PROCESS,
    POOL_DEPOT,
    POOL_MANAGED_DEPOT,
    POOL_EDITOR,
    POOL_STRING_BUILDER,
    POOL_INDEX_POOL,
    POOL_PATH_CACHE,
    POOL_REF_HOLDER,
    POOL_SCRIPT_COMPILER,
    POOL_RTTI_DATA,
    POOL_DATA_VIEW,
    POOL_DEBUG_GEOMETRY,
    POOL_XML,
    POOL_RENDERING_FRAME,
    POOL_RENDERING_SHADER_CACHE,
    POOL_RENDERING_PARAM_LAYOUT,
    POOL_RENDERING_TECHNIQUE,
    POOL_RENDERING_RUNTIME,
    POOL_TRIMESH,
    POOL_IMGUI,
    POOL_JIT,
    POOL_FIBERS,
    POOL_FONTS,
    POOL_GRAPH,
    POOL_INSTANCE_BUFFER,
    POOL_INSTANCE_LAYOUT,
    POOL_KEYS,
    POOL_UI_STYLES,
    POOL_EVENTS,
    POOL_PROXY,
    POOL_RENDERING_COMMANDS,
    POOL_RENDERING_COMMAND_BUFFER,
    POOL_DEFAULT_OBJECTS,
    POOL_OBJECTS,
    POOL_SHADER_COMPILATION,
    POOL_STREAMING,
    POOL_UI,
    POOL_ACTIONS,
    POOL_VARIANT,
    POOL_GAME,
    POOL_CANVAS,
    POOL_CONFIG,
    POOL_WINDOW,
    POOL_MEM_BUFFER,
    POOL_EXTERNAL_BUFFER_TAG,
    POOL_ZLIB,
    POOL_LZ4,
    POOL_ASYNC_BUFFER,
    POOL_SYSTEM_MEMORY,
    POOL_WAVEFRONT,
    POOL_MESH_BUILDER,
    POOL_HTTP_REQUEST,
    POOL_HTTP_REQUEST_DATA,
    POOL_COMPILED_SHADER_STRUCTURES,
    POOL_COMPILED_SHADER_DATA,
    POOL_RAW_RESOURCE,
    POOL_CONVEX_HULL,
    POOL_CONVEX_HULL_BUILDING,
    POOL_MATERIAL_DATA,
    POOL_TABLE_DATA,

    ///--

    POOL_PHYSICS_COLLISION,
    POOL_PHYSICS_TEMP,
    POOL_PHYSICS_SCENE,
    POOL_PHYSICS_RUNTIME,

    ///--

    POOL_API_STATIC_TEXTURES,
    POOL_API_VERTEX_BUFFER,
    POOL_API_INDEX_BUFFER,
    POOL_API_CONSTANT_BUFFER,
    POOL_API_STORAGE_BUFFER,
    POOL_API_INDIRECT_BUFFER,
    POOL_API_RENDER_TARGETS,
    POOL_API_TRANSIENT_BUFFER,
    POOL_API_BACKING_STORAGE,
    POOL_API_FRAMEBUFFERS,
    POOL_API_SAMPLERS,
    POOL_API_SHADERS,
    POOL_API_PROGRAMS,
    POOL_API_PIPELINES,
    POOL_API_OBJECTS,
    POOL_API_RUNTIME,

    POOL_MAX,
};

//-----------------------------------------------------------------------------

// This needs to be declared so we do not redefine new/delete operators
#define __PLACEMENT_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE

//-----------------------------------------------------------------------------

namespace base
{
    namespace mem
    {
        //--

        // print memory leaks
        extern BASE_MEMORY_API void DumpMemoryLeaks();

        // validate heap state
        extern BASE_MEMORY_API void ValidateHeap();

        //--

        //! Allocate page of memory directly from the system
        //! NOTE: this memory is not tracked
        extern BASE_MEMORY_API void* AllocSystemMemory(size_t size, bool largePages);

        //! Free page of memory directly to the system
        extern BASE_MEMORY_API void FreeSystemMemory(void* page, size_t size);

        //--

        //! Allocate memory
        extern BASE_MEMORY_API void* AllocateBlock(PoolTag id, size_t size, size_t alignment = 8, const char* typeName=nullptr);

        //! Resize allocated memory block
        extern BASE_MEMORY_API void* ResizeBlock(PoolTag id, void* mem, size_t size, size_t alignment = 8, const char* typeName=nullptr);

        //! Free allocated memory
        extern BASE_MEMORY_API void FreeBlock(void* mem);

        //---

        //! Start tracking allocations happening on this thread
        extern BASE_MEMORY_API void StartThreadAllocTracking();

        //! Finish tracking allocation happening on this thread, report all unfreed allocations
        extern BASE_MEMORY_API void FinishThreadAllocTracking();

        //--

        template< PoolTag TAG = POOL_DEFAULT, typename T = void >
        class GlobalPool
        {
        public:
            static ALWAYS_INLINE T* Alloc(size_t size, size_t alignment = 8)
            {
#if !defined(BUILD_FINAL)
                return (T*) AllocateBlock(TAG, size, alignment, typeid(T).raw_name());
#else
                return (T*) AllocateBlock(TAG, size, alignment);
#endif
            }

            static ALWAYS_INLINE T* AllocN(size_t count, size_t alignment = 0)
            {
#if !defined(BUILD_FINAL)
                return (T*) AllocateBlock(TAG, count * sizeof(T), alignment ? alignment : alignof(T), typeid(T).raw_name());
#else
                return (T*) AllocateBlock(TAG, count * sizeof(T), alignment ? alignment : alignof(T));
#endif
            }

            static ALWAYS_INLINE void Free(void* mem)
            {
                return FreeBlock(mem);
            }

            static ALWAYS_INLINE T* Resize(void* mem, size_t size, size_t alignment = 8)
            {
#if !defined(BUILD_FINAL)
                return (T*) ResizeBlock(TAG, mem, size, alignment, typeid(T).raw_name());
#else
                return (T*) ResizeBlock(TAG, mem, size, alignment);
#endif
            }
        };

        //--

        /// base class for objects that we want to allocate from GlobalPool via new/delete
        template< PoolTag TAG = POOL_NEW >
        class GlobalPoolObject
        {
        public:
            ALWAYS_INLINE void* operator new(std::size_t count)
            {
                return GlobalPool<TAG>::Alloc(count, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
            }

            ALWAYS_INLINE void* operator new[](std::size_t count)
            {
                return GlobalPool<TAG>::Alloc(count, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
            }

            ALWAYS_INLINE void* operator new(std::size_t count, std::align_val_t al)
            {
                return GlobalPool<TAG>::Alloc(count, (size_t)al);
            }

            ALWAYS_INLINE void* operator new[](std::size_t count, std::align_val_t al)
            {
                return GlobalPool<TAG>::Alloc(count, (size_t)al);
            }

            ALWAYS_INLINE void operator delete(void* ptr, std::size_t sz)
            {
                GlobalPool<TAG>::Free(ptr);
            }

            ALWAYS_INLINE void operator delete[](void* ptr, std::size_t sz)
            {
                GlobalPool<TAG>::Free(ptr);
            }

            ALWAYS_INLINE void operator delete(void* ptr, std::size_t sz, std::align_val_t al)
            {
                GlobalPool<TAG>::Free(ptr);
            }

            ALWAYS_INLINE void operator delete[](void* ptr, std::size_t sz, std::align_val_t al)
            {
                GlobalPool<TAG>::Free(ptr);
            }

            ALWAYS_INLINE void operator delete(void* ptr)
            {
                GlobalPool<TAG>::Free(ptr);
            }

            ALWAYS_INLINE void operator delete(void* ptr, std::align_val_t al)
            {
                GlobalPool<TAG>::Free(ptr);
            }

            ALWAYS_INLINE void operator delete[](void* ptr)
            {
                GlobalPool<TAG>::Free(ptr);
            }

            ALWAYS_INLINE void operator delete[](void* ptr, std::align_val_t al)
            {
                GlobalPool<TAG>::Free(ptr);
            }

            ALWAYS_INLINE void* operator new(std::size_t count, void* pt)
            {
                return pt;
            }

            ALWAYS_INLINE void operator delete(void *pt, void* pt2)
            {
            }
        };

        //--

        class LinearAllocator;
        class PageAllocator;
        class PageCollection;

        // get default page allocator for given pool
        extern BASE_MEMORY_API PageAllocator& DefaultPageAllocator(PoolTag pool = POOL_TEMP);

        //--

    } // mem
} // base

