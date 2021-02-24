/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMeshChunkProxy.h"
#include "rendering/device/include/renderingCommandWriter.h"

BEGIN_BOOMER_NAMESPACE(rendering)
	//---

	RTTI_BEGIN_TYPE_ENUM(MeshChunkType);
		RTTI_ENUM_OPTION(Standalone)
		RTTI_ENUM_OPTION(Meshlets)
	RTTI_END_TYPE();

	//---

	RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IMeshChunkProxy);
		RTTI_PROPERTY(m_type);
		RTTI_PROPERTY(m_format);
		RTTI_PROPERTY(m_indexCount);
		RTTI_PROPERTY(m_vertexCount);
	RTTI_END_TYPE();

	IMeshChunkProxy::IMeshChunkProxy(const Setup& setup, MeshChunkType type, MeshChunkID id)
		: m_type(type)
		, m_format(setup.format)
		, m_indexCount(setup.indexCount)
		, m_vertexCount(setup.vertexCount)
		, m_id(id)
	{
        m_quantizationOffset = setup.quantizationOffset;
        m_quantizationScale = setup.quantizationScale;
	}

	IMeshChunkProxy::~IMeshChunkProxy()
	{
	}

    //---

	RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshChunkProxy_Standalone);
	RTTI_END_TYPE();

	MeshChunkProxy_Standalone::MeshChunkProxy_Standalone(const Setup& setup, MeshChunkID id)
		: IMeshChunkProxy(setup, MeshChunkType::Standalone, id)
		, m_vertexBuffer(setup.vertexBuffer)
		, m_indexBuffer(setup.indexBuffer)
		, m_offsetInVertexBuffer(setup.offsetInVertexBuffer)
		, m_offsetInIndexBuffer(setup.offsetInIndexBuffer)
		, m_vertexBufferStride(setup.vertexBufferStride)
		, m_vertexBindPoint(MeshVertexFormatBindPointName(setup.format))
	{
	}

	MeshChunkProxy_Standalone::~MeshChunkProxy_Standalone()
	{}

	void MeshChunkProxy_Standalone::bind(GPUCommandWriter& cmd) const
	{
		cmd.opBindVertexBuffer(m_vertexBindPoint, m_vertexBuffer, m_offsetInVertexBuffer);

		if (m_indexBuffer)
			cmd.opBindIndexBuffer(m_indexBuffer, ImageFormat::R32_UINT, m_offsetInIndexBuffer);
	}

	void MeshChunkProxy_Standalone::draw(const GraphicsPipelineObject* pso, GPUCommandWriter& cmd, uint32_t numInstances/* = 1*/, uint32_t firstInstance /*= 0*/) const
	{
		if (m_indexBuffer)
			cmd.opDrawIndexedInstanced(pso, 0, 0, m_indexCount, firstInstance, numInstances);
		else
			cmd.opDrawInstanced(pso, 0, m_vertexCount, firstInstance, numInstances);
	}
 
	//---

	RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshChunkProxy_Meshlets);
	RTTI_END_TYPE();

	MeshChunkProxy_Meshlets::MeshChunkProxy_Meshlets(const Setup& setup, MeshChunkSharedStorage* storage, MeshChunkID id)
		: IMeshChunkProxy(setup, MeshChunkType::Meshlets, id)
		, m_allocation(setup.allocataions)
		, m_storage(storage)
	{}

	MeshChunkProxy_Meshlets::~MeshChunkProxy_Meshlets()
	{
		if (auto storage = m_storage.lock())
			storage->freeStorage(m_allocation);

		m_storage.reset();		
	}

	//---

} // rendering