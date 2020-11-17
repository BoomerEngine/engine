/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "buffer.h"

#pragma optimize("",off)

namespace base
{
    namespace mem
    {

        //--

        void BufferStorage::addRef()
        {
            ++m_refCount;
        }

        void BufferStorage::releaseRef()
        {
            if (0 == --m_refCount)
                freeData();
        }

        void BufferStorage::freeData()
        {
            if (m_externalPayload && m_freeFunc)
                m_freeFunc(m_pool, m_externalPayload, m_size);

            mem::FreeBlock(this);
        }

        BufferStorage* BufferStorage::CreateInternal(PoolTag pool, uint64_t size, uint32_t alignment)
        {
            DEBUG_CHECK_EX(alignment <= 4096, "Alignment requirement on buffer if really big");

            auto totalSize = size + sizeof(BufferStorage);
            if (alignment > alignof(BufferStorage))
                totalSize += (alignment - alignof(BufferStorage));

            auto* mem = (uint8_t*)mem::AllocateBlock(pool, totalSize, alignment, "BufferStorage");
            auto* payloadPtr = AlignPtr(mem + sizeof(BufferStorage), alignment);
            auto offset = payloadPtr - mem;
            DEBUG_CHECK_EX(offset <= 65535, "Offset to payload is to big");

            auto* ret = new (mem) BufferStorage();
            ret->m_pool = pool;
            ret->m_refCount = 1;
            ret->m_offsetToPayload = (uint16_t)offset;
            ret->m_size = size;
            return ret;
        }

        static void DefaultMemoryFreeFunc(PoolTag pool, void* memory, uint64_t size)
        {
            mem::FreeBlock(memory);
        }

        BufferStorage* BufferStorage::CreateExternal(PoolTag pool, uint64_t size, TBufferFreeFunc freeFunc, void *externalPayload)
        {
            if (!freeFunc)
                freeFunc = &DefaultMemoryFreeFunc;

            DEBUG_CHECK_EX(freeFunc && externalPayload, "Invalid setup for external buffer");

            auto* ret = new ( mem::GlobalPool<POOL_EXTERNAL_BUFFER_TAG, BufferStorage>::AllocN(1) ) BufferStorage();
            ret->m_pool = pool;
            ret->m_refCount = 1;
            ret->m_offsetToPayload = 0;
            ret->m_externalPayload = externalPayload;
            ret->m_freeFunc = freeFunc;
            ret->m_size = size;
            return ret;
        }

    } // mem

    //--

    Buffer::Buffer(const Buffer& other)
        : m_storage(other.m_storage)
    {
        if (m_storage)
            m_storage->addRef();
    }

    Buffer::Buffer(Buffer&& other)
    {
        m_storage = other.m_storage;
        other.m_storage = nullptr;
    }

    Buffer::~Buffer()
    {
        reset();
    }

    Buffer& Buffer::operator=(const Buffer& other)
    {
        if (this != &other)
        {
            auto old = m_storage;
            m_storage = other.m_storage;
            if (m_storage)
                m_storage->addRef();
            if (old)
                old->releaseRef();
        }
        return *this;
    }

    Buffer& Buffer::operator=(Buffer&& other)
    {
        if (this != &other)
        {
            auto old = m_storage;
            m_storage = other.m_storage;
            other.m_storage = nullptr;
            if (old)
                old->releaseRef();
        }
        return *this;
    }

    bool Buffer::operator==(const Buffer& other) const
    {
        if (m_storage == other.m_storage)
            return true;
        if (size() != other.size())
            return false;
        return 0 == memcmp(data(), other.data(), size());
    }

    bool Buffer::operator!=(const Buffer& other) const
    {
        return !operator==(other);
    }

    void Buffer::reset()
    {
        if (m_storage)
            m_storage->releaseRef();
        m_storage = nullptr;
    }

    void Buffer::adjustSize(uint32_t newBufferSize)
    {
        DEBUG_CHECK_EX(newBufferSize <= m_storage->size(), "Can't adjust buffer to be bigger, sorry");
        if (newBufferSize <= m_storage->size())
            m_storage->adjustSize(newBufferSize);
    }

    static void SystemMemoryFreeFunc(PoolTag pool, void* memory, uint64_t size)
    {
        mem::FreeSystemMemory(memory, size);
    }

    static void DefaultMemoryFreeFunc(PoolTag pool, void* memory, uint64_t size)
    {
        mem::FreeBlock(memory);
    }

    mem::BufferStorage*  Buffer::CreateSystemMemoryStorage(PoolTag pool, uint64_t size)
    {
        bool useLargePages = (size >= BUFFER_SYSTEM_MEMORY_LARGE_PAGES_SIZE);
        void* systemMemory = mem::AllocSystemMemory(size, useLargePages);
        if (!systemMemory)
        {
            TRACE_ERROR("Failed to allocate {} of system memory", MemSize(size));
            return nullptr;
        }

        return mem::BufferStorage::CreateExternal(pool, size, &SystemMemoryFreeFunc, systemMemory);
    }

    mem::BufferStorage* Buffer::CreateBestStorageForSize(PoolTag pool, uint64_t size, uint32_t alignment)
    {
        if (alignment == 0)
            alignment = BUFFER_DEFAULT_ALIGNMNET;

        if (size >= BUFFER_SYSTEM_MEMORY_MIN_SIZE)
        {
            return CreateSystemMemoryStorage(pool, size);
        }
        else
        {
            return mem::BufferStorage::CreateInternal(pool, size, alignment);
        }
    }

    bool Buffer::init(PoolTag pool, uint64_t size, uint32_t alignment /*= 1*/, const void* dataToCopy /*= nullptr*/, uint64_t dataSizeToCopy /*= INDEX_MAX64*/)
    {
        if (size == 0)
        {
            reset();
            return true;
        }

        if (auto* storage = CreateBestStorageForSize(pool, size, alignment))
        {
            reset();
            m_storage = storage;

            if (dataToCopy != nullptr)
            {
                auto copySize = std::min<uint64_t>(size, dataSizeToCopy);
                memcpy(m_storage->data(), dataToCopy, copySize);
            }

            return true;
        }

        return false;
    }

    bool Buffer::initWithZeros(PoolTag pool, uint64_t size, uint32_t alignment /*= 1*/)
    {
        if (size == 0)
        {
            reset();
            return true;
        }

        if (auto* storage = CreateBestStorageForSize(pool, size, alignment))
        {
            reset();
            m_storage = storage;
            memzero(m_storage->data(), size);
            return true;
        }

        return false;
    }

    void Buffer::print(IFormatStream& f) const
    {
        f.appendBase64(data(), size());
    }

    //--

    Buffer Buffer::Create(PoolTag pool, uint64_t size, uint32_t alignment /*= 4*/, const void* dataToCopy /*= nullptr*/, uint64_t dataSizeToCopy /*= INDEX_MAX64*/)
    {
        Buffer ret;
        ret.init(pool, size, alignment, dataToCopy, dataSizeToCopy);
        return ret;
    }

     Buffer Buffer::CreateZeroInitialized(PoolTag pool, uint64_t size, uint32_t alignment /*= 4*/)
     {
         Buffer ret;
         ret.initWithZeros(pool, size, alignment);
         return ret;
     }

    Buffer Buffer::CreateExternal(PoolTag pool, uint64_t size, void* externalData, mem::TBufferFreeFunc freeFunc/* = nullptr*/)
    {
        if (!size || !externalData)
            return Buffer();

        return Buffer(mem::BufferStorage::CreateExternal(pool, size, freeFunc, externalData));
    }

    Buffer Buffer::CreateInSystemMemory(PoolTag pool, uint64_t size, const void* dataToCopy /*= nullptr*/, uint64_t dataSizeToCopy /*= INDEX_MAX64*/)
    {
        if (size == 0)
            return Buffer();

        Buffer ret;

        if (auto* storage = CreateSystemMemoryStorage(pool, size))
        {
            ret.m_storage = storage;

            if (dataToCopy != nullptr)
            {
                auto copySize = std::min<uint64_t>(size, dataSizeToCopy);
                memcpy(ret.data(), dataToCopy, copySize);
            }
        }

        return ret;
    }

    //--

} // base