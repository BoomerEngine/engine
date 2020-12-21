/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMesh.h"
#include "renderingMeshFormat.h"
#include "renderingMeshService.h"
#include "renderingMeshChunkStorage.h"
#include "renderingMeshChunkProxy.h"

#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
	///---

	static base::ConfigProperty<uint32_t> cvMaxMeshletDataSizeKB("Rendering.Mesh", "MaxMeshletDataSizeKB", 1024);

    ///---

    RTTI_BEGIN_TYPE_CLASS(MeshService);
		RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<rendering::DeviceService>();
    RTTI_END_TYPE();

    ///----

    MeshService::MeshService()
    {}

	base::app::ServiceInitializationResult MeshService::onInitializeService(const base::app::CommandLine& cmdLine)
	{
		auto deviceService = base::GetService<DeviceService>();
		if (!deviceService)
			base::app::ServiceInitializationResult::FatalError;

		m_device = deviceService->device();

		m_meshletStorage = base::RefNew<MeshChunkSharedStorage>(m_device);
		return base::app::ServiceInitializationResult::Finished;
	}

    void MeshService::onShutdownService()
    {
		m_meshletStorage.reset();
    }

    void MeshService::onSyncUpdate()
    {
    }

	//--

	MeshChunkProxyPtr MeshService::createChunkProxy(const MeshChunk& data, const base::StringBuf& debugLabel)
	{
		DEBUG_CHECK_RETURN_EX_V(data.indexCount && data.vertexCount, "Empty chunk", nullptr);
		DEBUG_CHECK_RETURN_EX_V(data.unpackedIndexSize && data.unpackedVertexSize, "Nothing to allocate", nullptr);

		// unpack vertex data
		auto vertexData = data.packedVertexData;
		if (data.unpackedVertexSize != data.packedVertexData.size())
		{
			vertexData = UncompressVertexBuffer(data.packedVertexData.data(), data.packedVertexData.size(), data.vertexFormat, data.vertexCount);
			DEBUG_CHECK_RETURN_EX_V(vertexData, "Failed to unpack vertex data", nullptr);
		}

		// index vertex data
		auto indexData = data.packedIndexData;
		if (data.unpackedIndexSize != data.packedIndexData.size())
		{
			indexData = UncompressIndexBuffer(data.packedIndexData.data(), data.packedIndexData.size(), data.indexCount);
			DEBUG_CHECK_RETURN_EX_V(indexData, "Failed to unpack index data", nullptr);
		}

		// create chunk from unpacked data
		return createStandaloneProxy(data, vertexData, indexData, debugLabel);
	}

	MeshChunkProxyStandalonePtr MeshService::createStandaloneProxy(const MeshChunk& data, const base::Buffer& vertexData, const base::Buffer& indexData, const base::StringBuf& debugLabel)
	{
		BufferObjectPtr vertexBuffer;
		BufferObjectPtr indexBuffer;

		// allocate vertex data
		{
			BufferCreationInfo info;
			info.allowVertex = true;
			info.label = debugLabel;
			info.size = vertexData.size();
			vertexBuffer = m_device->createBuffer(info, new SourceDataProviderBuffer(vertexData));
		}

		// allocate index data
		{
			BufferCreationInfo info;
			info.allowIndex = true;
			info.label = debugLabel;
			info.size = indexData.size();
			indexBuffer = m_device->createBuffer(info, new SourceDataProviderBuffer(indexData));
		}

		MeshChunkProxy_Standalone::Setup setup;
		setup.debugLabel = debugLabel;
		setup.format = data.vertexFormat;
		setup.indexBuffer = indexBuffer;
		setup.vertexBuffer = vertexBuffer;
		setup.indexCount = data.indexCount;
		setup.vertexCount = data.vertexCount;
		setup.quantizationOffset = data.quantizationOffset;
		setup.quantizationScale = data.quantizationScale;

		return base::RefNew<MeshChunkProxy_Standalone>(setup);
	}

	//--

	void MeshService::uploadChanges(command::CommandWriter& cmd) const
	{
		m_meshletStorage->pushUpdates(cmd);
	}

	void MeshService::bindCommonMeshletData(command::CommandWriter& cmd) const
	{
		//m_meshletStorage->bin
	}

    ///--

} // rendering