/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "renderingBufferView.h"
#include "renderingDeviceObject.h"
#include "base/containers/include/blockPool.h"

namespace rendering
{
    //---

    struct ManagedBufferBlock
    {
        uint32_t bufferIndex = 0;
        uint32_t bufferOffset = 0;
        uint32_t dataSize = 0;
        MemoryBlock block = nullptr;
    };

    //---

    /// a buffer that supports allocations and tracking of it's sub parts
    class RENDERING_DRIVER_API ManagedBufferWithAllocator : public IDeviceObject
    {
    public:
        ManagedBufferWithAllocator(const BufferCreationInfo& info, uint32_t alignment, float growthFactor = 2.0f, uint64_t maxSize = 0);
        virtual ~ManagedBufferWithAllocator();

        //---

        /// buffer
        INLINE const BufferView& bufferView() const { return m_bufferView; }

        //---

        /// advance frame, doeas a final free on the blocks
        void advanceFrame();

        /// prepare buffer for use, write update commands
        void update(command::CommandWriter& cmd);

        //---

        /// allocate place in the buffer
        bool allocateBlock(uint32_t dataSize, ManagedBufferBlock& outBlock);

        /// allocate place in the buffer and upload data there
        bool allocateBlock(const base::Buffer& data, ManagedBufferBlock& outBlock);

        /// free previously allocated block 
        /// NOTE: the block will be freed only after it's not longer used by GPU
        void freeBlock(const ManagedBufferBlock& block);

        //---

    private:
        BufferView m_bufferView;
        base::BlockPool m_bufferAllocator;
        base::SpinLock m_bufferAllocatorLock;
        uint32_t m_bufferIndex = 1;

        uint32_t m_alignment;

        BufferCreationInfo m_creationInfo;

        float m_growthFactor = 2.0f;
        uint64_t m_maxSize;

        base::Mutex m_lock;

        //--

        struct PendingUpload : public base::mem::GlobalPoolObject<POOL_RENDERING_FRAME>
        {
            uint32_t offset = 0;
            uint32_t size = 0;
            MemoryBlock block;
            base::Buffer data;
        };

        base::Array<PendingUpload*> m_pendingUploads;
        base::SpinLock m_pendingUploadsLock;

        //--

        struct PendingFree : public base::mem::GlobalPoolObject<POOL_RENDERING_FRAME>
        {
            MemoryBlock block;
        };

        static const uint32_t MAX_FREE_FRAMES = 3;
        base::Array<PendingFree*> m_pendingFrees[MAX_FREE_FRAMES];
        base::SpinLock m_pendingFreesLock;

        //--

        virtual void handleDeviceReset() override final;
        virtual void handleDeviceRelease() override final;
        virtual base::StringBuf describe() const override final;
    };

    //--

} // rendering

