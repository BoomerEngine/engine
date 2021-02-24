/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "base/containers/include/blockPool.h"

BEGIN_BOOMER_NAMESPACE(rendering)

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

	INLINE uint32_t vertexDataSize() const { return base::BlockPool::GetBlockSize(vertexDataBlock); }
	INLINE uint32_t vertexDataOffset() const { return base::BlockPool::GetBlockOffset(vertexDataBlock); }
	INLINE uint32_t indexDataSize() const { return base::BlockPool::GetBlockSize(indexDataBlock); }
	INLINE uint32_t indexDataOffset() const { return base::BlockPool::GetBlockOffset(indexDataBlock); }

private:
	MemoryBlock vertexDataBlock;
	MemoryBlock indexDataBlock;

	friend class MeshChunkSharedStorage;
};

//---

/// global "shared buffer" for vertex/index data
class MeshChunkSharedStorage : public base::IReferencable
{
public:
	MeshChunkSharedStorage(IDevice* device);
	~MeshChunkSharedStorage();

	//--

	// push updates to GPU
	void pushUpdates(GPUCommandWriter& cmd);

	//--

	// allocate storage for BOTH vertex and index buffer at the same time, can fail
	bool allocateStorage(const base::Buffer& vertices, const base::Buffer& indices, MeshChunkSharedStorageAllocator& outAllocation);

	// free previously allocated storage
	void freeStorage(const MeshChunkSharedStorageAllocator& allocation);

	//--		

private:
	static const uint32_t VERTEX_ALIGNMNET = 64;
	static const uint32_t INDEX_ALIGNMNET = 4;

	//--

	BufferObjectPtr m_vertexStorage;
	BufferViewPtr m_vertexStorageSRV; // R32
	BufferObjectPtr m_indexStorage;
	BufferViewPtr m_indexStorageSRV; // R32

	//--

	base::SpinLock m_allocatorLock;
	base::BlockPool m_vertexBlockAllocator;
	base::BlockPool m_indexBlockAllocator;

	//--

	struct PendingUpload : public base::NoCopy
	{
		RTTI_DECLARE_POOL(POOL_RENDERING_FRAME)

	public:
		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;
		base::Buffer vertexData;
		base::Buffer indexData;
	};

	base::SpinLock m_pendingUploadsLock;
	base::Array<PendingUpload*> m_pendingUploads;

	//--
};

//---

END_BOOMER_NAMESPACE(rendering)