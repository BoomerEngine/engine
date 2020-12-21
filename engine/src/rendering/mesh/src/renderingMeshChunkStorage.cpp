/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMeshChunkStorage.h"
#include "rendering/device/include/renderingResources.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingBuffer.h"
#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
	//---

	// 1GB of initial pool - 1GB of vram for meshes is 3x more than The Witcher 3 used, so it's PLENTy of realistic storage
	base::ConfigProperty<uint32_t> cvMaxVertexPoolSizeMB("Rendering.Meshes", "InitialMeshletVertexPoolSizeMB", 700);
	base::ConfigProperty<uint32_t> cvMaxIndexPoolSizeMB("Rendering.Meshes", "InitialMeshletIndexPoolSizeMB", 300);

	// TODO: different pool size (or size boost) for editor

    //---

	MeshChunkSharedStorage::MeshChunkSharedStorage(IDevice* device)
	{
		// determine size
		const auto vertexBufferSize = std::max<uint32_t>(cvMaxVertexPoolSizeMB.get(), 1) << 20;
		const auto indexBufferSize = std::max<uint32_t>(cvMaxVertexPoolSizeMB.get(), 1) << 20;

		// create vertex buffer
		{
			BufferCreationInfo info;
			info.allowCopies = true;
			info.allowDynamicUpdate = true;
			info.allowShaderReads = true;
			//info.allowVertex = true; // not used as real vertex buffer
			info.label = "MeshChunkStorageVertices";
			info.size = vertexBufferSize;
			m_vertexStorage = device->createBuffer(info);
			m_vertexStorageSRV = m_vertexStorage->createView(ImageFormat::R32_UINT);
		}

		// create index buffer
		{
			BufferCreationInfo info;
			info.allowCopies = true;
			info.allowDynamicUpdate = true;
			info.allowShaderReads = true;
			//info.allowIndex = true;  // not used as real index buffer
			info.label = "MeshChunkStorageIndices";
			info.size = indexBufferSize;
			m_indexStorage = device->createBuffer(info);
			m_indexStorageSRV = m_indexStorage->createView(ImageFormat::R32_UINT);
		}

		// initialize allocators
		const auto minVertexBlock = 4096;
		const auto minIndexBlock = 256;
		m_vertexBlockAllocator.setup(vertexBufferSize, 1 + (vertexBufferSize / minVertexBlock), minVertexBlock);
		m_indexBlockAllocator.setup(indexBufferSize, 1 + (indexBufferSize / minIndexBlock), minIndexBlock);
	}

	MeshChunkSharedStorage::~MeshChunkSharedStorage()
	{
		m_indexStorageSRV.reset();
		m_indexStorage.reset();

		m_vertexStorageSRV.reset();
		m_vertexStorage.reset();
	}

	//--

	void MeshChunkSharedStorage::pushUpdates(command::CommandWriter& cmd)
	{
		PC_SCOPE_LVL1(UpdateMeshSharedStorage);

		// get update list
		m_pendingUploadsLock.acquire();
		auto updates = std::move(m_pendingUploads);
		m_pendingUploadsLock.release();

		// post updates
		if (!updates.empty())
		{
			// transition to copy dest
			// TODO: for platforms when only range can be transformed expose this
			cmd.opTransitionLayout(m_vertexStorage, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);
			cmd.opTransitionLayout(m_indexStorage, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);

			// TODO: transfer buffer ownership to command list to avoid copies...
			for (const auto* update : updates)
			{
				cmd.opUpdateDynamicBuffer(m_vertexStorage, update->vertexOffset, update->vertexData.size(), update->vertexData.data());
				cmd.opUpdateDynamicBuffer(m_indexStorage, update->indexOffset, update->indexData.size(), update->indexData.data());
			}

			// transition back
			// TODO: for platforms when only range can be transformed expose this
			cmd.opTransitionLayout(m_vertexStorage, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_indexStorage, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
		}
	}

	//--

	bool MeshChunkSharedStorage::allocateStorage(const base::Buffer& vertices, const base::Buffer& indices, MeshChunkSharedStorageAllocator& outAllocation)
	{
		DEBUG_CHECK_RETURN_EX_V(!vertices.empty(), "No vertex data", false);
		DEBUG_CHECK_RETURN_EX_V(!indices.empty(), "No index data", false);
		DEBUG_CHECK_RETURN_EX_V(!outAllocation, "Allocation result is not empty", false);

		// allocate both blocks at ones
		MemoryBlock vertexBlock = nullptr;
		MemoryBlock indexBlock = nullptr;

		{
			auto lock = CreateLock(m_allocatorLock);
			if (base::BlockAllocationResult::OK != m_vertexBlockAllocator.allocateBlock(vertices.size(), VERTEX_ALIGNMNET, vertexBlock))
				return false;

			if (base::BlockAllocationResult::OK != m_indexBlockAllocator.allocateBlock(indices.size(), INDEX_ALIGNMNET, indexBlock))
			{
				m_vertexBlockAllocator.freeBlock(vertexBlock);
				return false;
			}
		}

		// create pending allocation
		{
			auto* pending = new PendingUpload();
			pending->indexData = indices;
			pending->indexOffset = base::BlockPool::GetBlockOffset(indexBlock);
			pending->vertexData = vertices;
			pending->vertexOffset = base::BlockPool::GetBlockOffset(vertexBlock);

			auto lock = CreateLock(m_pendingUploadsLock);
			m_pendingUploads.pushBack(pending);
		}

		outAllocation.indexDataBlock = indexBlock;
		outAllocation.vertexDataBlock = vertexBlock;
		return true;
	}

	void MeshChunkSharedStorage::freeStorage(const MeshChunkSharedStorageAllocator& allocation)
	{
		DEBUG_CHECK_RETURN_EX(allocation, "Freeing invalid allocation");

		auto lock = CreateLock(m_allocatorLock);
		m_vertexBlockAllocator.freeBlock(allocation.vertexDataBlock);
		m_indexBlockAllocator.freeBlock(allocation.indexDataBlock);
	}

    //---
 
} // rendering