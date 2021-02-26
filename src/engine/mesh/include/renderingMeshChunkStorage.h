/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "core/containers/include/blockPool.h"

BEGIN_BOOMER_NAMESPACE()

//---

class MeshChunkSharedStorage;

//---

/// space in shared mesh storage
struct MeshChunkSharedStorageAllocator
{
public:
	INLINE MeshChunkSharedStorageAllocator() {}
	INLINE MeshChunkSharedStorageAllocator(const MeshChunkSharedStorageAllocator& other) = default;
	INLINE MeshChunkSharedStorageAllocator& operator=(const MeshChunkSharedStorageAllocator& other) = default;

	INLINE operator bool() const { return vertexDataBlock && indexDataBlock; }

	INLINE uint32_t vertexDataSize() const { return BlockPool::GetBlockSize(vertexDataBlock); }
	INLINE uint32_t vertexDataOffset() const { return BlockPool::GetBlockOffset(vertexDataBlock); }
	INLINE uint32_t indexDataSize() const { return BlockPool::GetBlockSize(indexDataBlock); }
	INLINE uint32_t indexDataOffset() const { return BlockPool::GetBlockOffset(indexDataBlock); }

private:
	MemoryBlock vertexDataBlock;
	MemoryBlock indexDataBlock;

	friend class MeshChunkSharedStorage;
};

//---

/// global "shared buffer" for vertex/index data
class MeshChunkSharedStorage : public IReferencable
{
public:
	MeshChunkSharedStorage(gpu::IDevice* device);
	~MeshChunkSharedStorage();

	//--

	// push updates to GPU
	void pushUpdates(gpu::CommandWriter& cmd);

	//--

	// allocate storage for BOTH vertex and index buffer at the same time, can fail
	bool allocateStorage(const Buffer& vertices, const Buffer& indices, MeshChunkSharedStorageAllocator& outAllocation);

	// free previously allocated storage
	void freeStorage(const MeshChunkSharedStorageAllocator& allocation);

	//--		

private:
	static const uint32_t VERTEX_ALIGNMNET = 64;
	static const uint32_t INDEX_ALIGNMNET = 4;

	//--

	gpu::BufferObjectPtr m_vertexStorage;
	gpu::BufferViewPtr m_vertexStorageSRV; // R32
	gpu::BufferObjectPtr m_indexStorage;
	gpu::BufferViewPtr m_indexStorageSRV; // R32

	//--

	SpinLock m_allocatorLock;
	BlockPool m_vertexBlockAllocator;
	BlockPool m_indexBlockAllocator;

	//--

	struct PendingUpload : public NoCopy
	{
		RTTI_DECLARE_POOL(POOL_RENDERING_FRAME)

	public:
		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;
		Buffer vertexData;
		Buffer indexData;
	};

	SpinLock m_pendingUploadsLock;
	Array<PendingUpload*> m_pendingUploads;

	//--
};

//---

END_BOOMER_NAMESPACE()
