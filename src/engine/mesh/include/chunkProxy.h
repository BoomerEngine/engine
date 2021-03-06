/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "format.h"
#include "chunkStorage.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// mesh chunk type
enum class MeshChunkType : uint8_t
{
	Standalone, // standalone mesh chunk with separate vertex/index buffers, culled by CPU
	Meshlets, // mesh chunk with meshlets, renderable via GPU culling pipeline

	// other:
	Terrain, // terrain representing part of terrain ?
};

//---

/// runtime proxy to part of renderable mesh's geometry
class ENGINE_MESH_API IMeshChunkProxy : public IReferencable
{
	RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IMeshChunkProxy);

public:
	struct Setup
	{
		MeshVertexFormat format;
		uint32_t indexCount = 0;
		uint32_t vertexCount = 0;

		Vector3 quantizationOffset;
		Vector3 quantizationScale;

		StringBuf debugLabel;
	};

	IMeshChunkProxy(const Setup& setup, MeshChunkType type, MeshChunkID id);
	virtual ~IMeshChunkProxy();

	// get global mesh chunk ID
	INLINE MeshChunkID id() const { return m_id; }

	// chunk type
	INLINE MeshChunkType type() const { return m_type; }

	// vertex format
	INLINE MeshVertexFormat format() const { return m_format; }

	// number of indices in the chunk
	INLINE uint32_t indexCount() const { return m_indexCount; }

	// number of vertices in the chunk
	INLINE uint32_t vertexCount() const { return m_vertexCount; }

	// mesh quantization offset
	INLINE const Vector3& quantizationOffset() const { return m_quantizationOffset; }

	// mesh quantization scale
	INLINE const Vector3& quantizationScale() const { return m_quantizationScale; }

protected:
	MeshChunkID m_id;

	MeshChunkType m_type;
	MeshVertexFormat m_format;
	uint32_t m_indexCount = 0;
	uint32_t m_vertexCount = 0;

	Vector3 m_quantizationOffset;
	Vector3 m_quantizationScale;

	StringBuf m_debugLabel;
};

//---

// standalone mesh chunk
class ENGINE_MESH_API MeshChunkProxy_Standalone : public IMeshChunkProxy
{
	RTTI_DECLARE_VIRTUAL_CLASS(MeshChunkProxy_Standalone, IMeshChunkProxy);

public:
	struct Setup : public IMeshChunkProxy::Setup
	{
		StringID vertexBindPoint;
		gpu::BufferObjectPtr vertexBuffer;
		gpu::BufferObjectPtr indexBuffer;
		uint32_t offsetInVertexBuffer = 0;
		uint32_t offsetInIndexBuffer = 0;
		uint32_t vertexBufferStride = 0;
	};

	//--

	MeshChunkProxy_Standalone(const Setup& setup, MeshChunkID id);
	virtual ~MeshChunkProxy_Standalone();

	// bind buffers related to this chunk
	void bind(gpu::CommandWriter& cmd) const;

	// draw this chunk
	void draw(const gpu::GraphicsPipelineObject* pso, gpu::CommandWriter& cmd, uint32_t numInstances = 1, uint32_t firstInstance = 0) const;

	//--

private:
	gpu::BufferObjectPtr m_vertexBuffer;
	gpu::BufferObjectPtr m_indexBuffer;
	uint32_t m_offsetInVertexBuffer = 0;
	uint32_t m_offsetInIndexBuffer = 0;
	uint32_t m_vertexBufferStride = 0;

	StringID m_vertexBindPoint;
};

//---

/// meshlet mesh chunk, renderable using compute shaders mesh/task shaders
/// NOTE: this chunk is NOT renderable otherwise
class ENGINE_MESH_API MeshChunkProxy_Meshlets : public IMeshChunkProxy
{
	RTTI_DECLARE_VIRTUAL_CLASS(MeshChunkProxy_Meshlets, IMeshChunkProxy);

public:
	struct Setup : public IMeshChunkProxy::Setup
	{
		MeshChunkSharedStorageAllocator allocataions;
	};

	MeshChunkProxy_Meshlets(const Setup& setup, MeshChunkSharedStorage* storage, MeshChunkID id);
	virtual ~MeshChunkProxy_Meshlets();

	INLINE uint32_t vertexDataSize() const { return m_allocation.vertexDataSize(); }
	INLINE uint32_t vertexDataOffset() const { return m_allocation.vertexDataOffset(); }
	INLINE uint32_t indexDataSize() const { return m_allocation.indexDataSize(); }
	INLINE uint32_t indexDataOffset() const { return m_allocation.indexDataOffset(); }

private:
	MeshChunkSharedStorageAllocator m_allocation;

	RefWeakPtr<MeshChunkSharedStorage> m_storage;
};

//---

END_BOOMER_NAMESPACE()
