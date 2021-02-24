/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "base/containers/include/blockPool.h"

BEGIN_BOOMER_NAMESPACE(rendering)

//---

struct ManagedBufferBlock
{
    uint32_t dataOffset = 0;
    uint32_t dataSize = 0;
    MemoryBlock block = nullptr;
};

//---

/// a buffer that supports allocations and tracking of it's sub parts
class RENDERING_DEVICE_API ManagedBufferWithAllocator : public base::NoCopy
{
    RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME)

public:
    ManagedBufferWithAllocator(IDevice* dev, const BufferCreationInfo& info, uint32_t alignment);
    ~ManagedBufferWithAllocator();

    //---

    /// buffer object
    INLINE const BufferObjectPtr& bufferObject() { return m_bufferObject; }

    //---

    /// advance frame, does a final free on the blocks
    void advanceFrame();

    /// prepare buffer for use, write update commands
    void update(GPUCommandWriter& cmd);

    //---

    /// allocate place in the buffer, does not update content
    bool allocateBlock(uint32_t dataSize, ManagedBufferBlock& outBlock);

    /// allocate place in the buffer and upload data there
    bool allocateBlock(const base::Buffer& data, ManagedBufferBlock& outBlock);

    /// free previously allocated block 
    /// NOTE: the block will be freed only after it's not longer used by GPU
    void freeBlock(const ManagedBufferBlock& block);

    //---

private:
    BufferObjectPtr m_bufferObject;

    base::BlockPool m_bufferAllocator;
    base::SpinLock m_bufferAllocatorLock;

    uint32_t m_alignment = 0;

    base::Mutex m_lock;

    //--

    struct PendingUpload : public base::NoCopy
    {
        RTTI_DECLARE_POOL(POOL_RENDERING_FRAME)

    public:
        uint32_t offset = 0;
        uint32_t size = 0;
        MemoryBlock block;
        base::Buffer data;
    };

    base::Array<PendingUpload*> m_pendingUploads;
    base::SpinLock m_pendingUploadsLock;

    //--

    struct PendingFree : public base::NoCopy
    {
        RTTI_DECLARE_POOL(POOL_RENDERING_FRAME)

    public:
        MemoryBlock block;
    };

    static const uint32_t MAX_FREE_FRAMES = 3;
    base::Array<PendingFree*> m_pendingFrees[MAX_FREE_FRAMES];
    base::SpinLock m_pendingFreesLock;
};

//--

END_BOOMER_NAMESPACE(rendering)

