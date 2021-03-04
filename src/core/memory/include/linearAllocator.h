/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\pages #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

///--

/// Simple local linear allocator, all memory is allocated from page collection
/// NOTE: this allocator is STRICTLY single threaded but you can share the PageCollection
class CORE_MEMORY_API LinearAllocator : public NoCopy
{
public:
    LinearAllocator(PoolTag pool = POOL_TEMP, uint32_t tailReserve = 0); // use default page allocator for given pool, all memory will be released by OUR destructor
    LinearAllocator(PageAllocator& pageAllocator, uint32_t tailReserve = 0); // use specific page allocator
    ~LinearAllocator();

    /// get the memory pool
    INLINE PoolTag poolID() const { return m_poolId; }

    /// get number of allocations done
    INLINE uint32_t numAllocations() const { return m_numAllocations; }

    /// get used allocated memory
    INLINE uint64_t totalUsedMemory() const { return m_totalUsedMemory; }

    /// get total wasted memory
    INLINE uint64_t totalWastedMemory() const { return m_totalWastedMemory; }

    /// do we own the page collection ? if so all pages will be release when THIS ALLOCATOR is destroyed
    INLINE bool ownsPageCollection() const { return m_ownsPageCollection; }

    /// get the page collection 
    INLINE PageCollection& pageCollection() const { return *m_pageCollection; }

    /// get the page allocator
    INLINE PageAllocator& pageAllocator() const { return *m_pageAllocator; }

    /// check if given amount of data will fit without allocating new page
    INLINE bool fits(uint32_t bytes) const { return m_curBuffer + bytes <= m_curBufferEnd; }

    /// "peak" at current write pointer without actually advancing it
    /// NOTE: this is legal only if you specified "tailReserve" > 0 and you can only safely peek that many bytes
    /// Main purpose is to be able to insert "a moving guard" after everything we have written, used especially in CommandBuffers 
    INLINE uint8_t* peak() { return m_curBuffer; }
    INLINE const uint8_t* peak() const { return m_curBuffer; }

    //--

    /// free all allocated memory 
    /// NOTE: works only if we OWN our page collection
    void clear();

    // Allocate memory, larger blocks may be allocated as outstanding allocations
    // NOTE: there's NO GUARANTEE of any continuity between blocks, just they they are allocated fast
    void* alloc(uint64_t size, uint32_t align);

    //--

    // allocate memory for an object, if it requires destruction we also add a proper cleanup function to call later
    template< typename T, typename... Args >
    INLINE T* create(Args && ... args)
    {
        void* mem = alloc(sizeof(T), alignof(T));
        T* ptr = new (mem) T(std::forward< Args >(args)...);
        if (!std::is_trivially_destructible<T>::value)
            deferCleanup<T>(ptr);
        return ptr;
    }

    // allocate memory for an object ensure that there's no cleanup
    template< typename T, typename... Args >
    INLINE T* createNoCleanup(Args&& ... args)
    {
        void* mem = alloc(sizeof(T), alignof(T));
        T* ptr = new (mem) T(std::forward< Args >(args)...);
        return ptr;
    }

    // create a copy of a string, useful for parsers
    template< typename T >
    INLINE T* strcpy(const T* txt, uint32_t length = INDEX_MAX)
    {
        // null string is not copied
        if (!txt)
            return nullptr;

        // calculate length
        if (length == INDEX_MAX)
        {
            length = 0;
            while (0 != txt[length]) ++length;
        }

        // allocate memory and copy data
        auto ptr  = (T*)alloc(sizeof(T)*(length+1), alignof(T));
        memcpy(ptr, txt, sizeof(T)*length);
        ptr[length] = 0;
        return (T*)ptr;
    }

    //--

    template< typename T >
    struct TypeDestroyer
    {
        static void CallDestructor(void* ptr)
        {
            ((T*)ptr)->~T();
        }
    };

    typedef void (*TCleanupFunc)(void* userData);
    void deferCleanup(TCleanupFunc func, void* userData);

    template< typename T >
    INLINE void deferCleanup(T* ptr)
    {
        deferCleanup(&TypeDestroyer<T>::CallDestructor, ptr);
    }

    //--

    // self-initialize a linear allocator allocate a page from page allocator and build page collection in first bytes than linear allocator)
    // NOTE: to release this allocator use "clear()" - this will release all memory pages also the one that hosts the LinearAllocator itself
    static LinearAllocator* CreateFromAllocator(PageAllocator& pageAllocator, uint32_t tailReserve=0);

    //--

private:
    LinearAllocator(PageCollection* pageCollection, uint8_t* initialBufferPtr, uint8_t* initialBufferEnd, uint32_t tailReserve); // use specific page collection, the pages allocated by this linear allocated will NOT be released when its destroyed but rather then the page collection is

    uint8_t *m_curBuffer = nullptr;
    uint8_t *m_curBufferEnd = nullptr;

    uint32_t m_numAllocations = 0;
    uint64_t m_totalUsedMemory = 0;
    uint64_t m_totalWastedMemory = 0;
    uint32_t m_tailReserve = 0;

    PoolTag m_poolId;

    PageAllocator* m_pageAllocator = nullptr;
    PageCollection* m_pageCollection = nullptr;

    bool m_ownsPageCollection = false;           
};

///--

END_BOOMER_NAMESPACE()
