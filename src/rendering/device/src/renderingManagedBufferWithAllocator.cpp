/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"

#include "renderingManagedBufferWithAllocator.h"
#include "renderingCommandWriter.h"
#include "renderingDeviceApi.h"
#include "renderingDeviceService.h"
#include "renderingResources.h"

BEGIN_BOOMER_NAMESPACE(rendering)

//---

ManagedBufferWithAllocator::ManagedBufferWithAllocator(IDevice* dev, const BufferCreationInfo& info, uint32_t alignment)
    : m_alignment(alignment)
{
    auto creationInfo = info;

    const auto blockCount = std::min<uint32_t>(65536, info.size / 4096);
    m_bufferAllocator.setup(info.size, blockCount, 4096);

    creationInfo.allowCopies = true;
    creationInfo.allowDynamicUpdate = true;

    m_bufferObject = dev->createBuffer(creationInfo);
}

ManagedBufferWithAllocator::~ManagedBufferWithAllocator()
{
    for (auto* pendingUpload : m_pendingUploads)
    {
        ManagedBufferBlock ret;
        ret.block = pendingUpload->block;
        ret.dataOffset = pendingUpload->offset;
        ret.dataSize = pendingUpload->size;
        freeBlock(ret);
        //m_bufferAllocator.freeBlock(pendingUpload->block);
    }
    m_pendingUploads.clearPtr();

    for (uint32_t i=0; i<MAX_FREE_FRAMES; i++)
        advanceFrame();

    for (uint32_t i = 0; i < MAX_FREE_FRAMES; ++i)
        m_pendingFrees->clearPtr();

    m_bufferObject.reset();
}

void ManagedBufferWithAllocator::advanceFrame()
{
    PC_SCOPE_LVL1(ManagedBufferWithAllocatorAdvance);

    // free blocks
    m_pendingFreesLock.acquire();

    auto finalFrees = std::move(m_pendingFrees[0]);
    for (uint32_t i = 1; i < MAX_FREE_FRAMES; ++i)
        m_pendingFrees[i-1] = std::move(m_pendingFrees[i]);

    m_pendingFreesLock.release();

    // do the actual free
    if (!finalFrees.empty())
    {
        auto lock = CreateLock(m_bufferAllocatorLock);

        auto prevBlockCount = m_bufferAllocator.numAllocatedBlocks();
        auto prevByteCount = m_bufferAllocator.numAllocatedBytes();

        uint64_t freedSize = 0;
        for (auto* entry : finalFrees)
        {
            freedSize += m_bufferAllocator.GetBlockSize(entry->block);
            m_bufferAllocator.freeBlock(entry->block);
            delete entry;
        }

        auto curBlockCount = m_bufferAllocator.numAllocatedBlocks();
        auto curByteCount = m_bufferAllocator.numAllocatedBytes();

        TRACE_INFO("Freed {} ({} block(s)) from managed buffer. {}->{}, {}->{}",
            finalFrees.size(), MemSize(freedSize),
            prevBlockCount, curBlockCount,
            MemSize(prevByteCount), MemSize(curByteCount));
    }
}

void ManagedBufferWithAllocator::update(GPUCommandWriter& cmd)
{
    // record data uploads
    {
        m_pendingUploadsLock.acquire();
        auto finalUploads = std::move(m_pendingUploads);
        m_pendingUploadsLock.release();

        if (!finalUploads.empty())
        {
            uint64_t totalUploadedSize = 0;
            for (auto* entry : finalUploads)
            {
                cmd.opUpdateDynamicBuffer(m_bufferObject, entry->offset, entry->size, entry->data.data());
                totalUploadedSize += entry->size;
            }

            TRACE_SPAM("Uploaded {} ({} block(s))", MemSize(totalUploadedSize), finalUploads.size());
        }
    }
}

bool ManagedBufferWithAllocator::allocateBlock(uint32_t dataSize, ManagedBufferBlock& outBlock)
{
    MemoryBlock allocatedBlock = nullptr;

    // nothing to allocated
    if (!dataSize)
        return false;

    // allocate the block
    {
        auto lock = base::CreateLock(m_bufferAllocatorLock);
        if (base::BlockAllocationResult::OK != m_bufferAllocator.allocateBlock(dataSize, m_alignment, allocatedBlock))
            return false;
    }

    // return block
    outBlock.block = allocatedBlock;
    outBlock.dataOffset = base::BlockPool::GetBlockOffset(allocatedBlock);
    outBlock.dataSize = dataSize;
    return true;
}

bool ManagedBufferWithAllocator::allocateBlock(const base::Buffer& data, ManagedBufferBlock& outBlock)
{
    MemoryBlock allocatedBlock = nullptr;

    // nothing to allocated
    if (data.empty())
        return false;

    // allocate the block
    {
        auto lock = base::CreateLock(m_bufferAllocatorLock);
        if (base::BlockAllocationResult::OK != m_bufferAllocator.allocateBlock(data.size(), m_alignment, allocatedBlock))
            return false;
    }

    // create the upload request
    {
        auto lcok = base::CreateLock(m_pendingUploadsLock);
        auto upload = new PendingUpload;
        upload->block = allocatedBlock;
        upload->offset = base::BlockPool::GetBlockOffset(allocatedBlock);;
        upload->size = data.size();
        upload->data = data;
        m_pendingUploads.pushBack(upload);
    }

    // return block
    outBlock.block = allocatedBlock;
    outBlock.dataOffset = base::BlockPool::GetBlockOffset(allocatedBlock);
    outBlock.dataSize = data.size();
    return true;
}

void ManagedBufferWithAllocator::freeBlock(const ManagedBufferBlock& block)
{
    // invalid block to free
    if (!block.block || !block.dataSize)
        return;

    auto lock = CreateLock(m_pendingFreesLock);

    auto pendingFree = new PendingFree;
    pendingFree->block = block.block;
    m_pendingFrees[MAX_FREE_FRAMES - 1].pushBack(pendingFree);
}

//---

END_BOOMER_NAMESPACE(rendering)