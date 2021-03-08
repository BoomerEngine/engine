/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "mesh.h"
#include "format.h"
#include "service.h"
#include "chunkStorage.h"
#include "chunkProxy.h"

#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE()

///---

static ConfigProperty<uint32_t> cvMaxMeshletDataSizeKB("Rendering.Mesh", "MaxMeshletDataSizeKB", 1024);

///---

RTTI_BEGIN_TYPE_CLASS(MeshService);
	RTTI_METADATA(app::DependsOnServiceMetadata).dependsOn<gpu::DeviceService>();
RTTI_END_TYPE();

///----

MeshService::MeshService()
{}

app::ServiceInitializationResult MeshService::onInitializeService(const app::CommandLine& cmdLine)
{
	auto deviceService = GetService<DeviceService>();
	if (!deviceService)
		app::ServiceInitializationResult::FatalError;

	m_freeMeshChunkIDs.resize(65536);
	for (uint32_t i = 0; i < m_freeMeshChunkIDs.size(); ++i)
		m_freeMeshChunkIDs[i] = (m_freeMeshChunkIDs.size() - 1) - i;

	m_device = deviceService->device();

	m_meshletStorage = RefNew<MeshChunkSharedStorage>(m_device);
	return app::ServiceInitializationResult::Finished;
}

void MeshService::onShutdownService()
{
	m_meshletStorage.reset();
}

void MeshService::onSyncUpdate()
{
}

//--

void MeshService::releaseMeshChunkId(MeshChunkID id)
{
	auto lock = CreateLock(m_freeMeshChunkIDsLock);
	m_freeMeshChunkIDs.pushBack(id);
}

MeshChunkProxyPtr MeshService::createChunkProxy(const MeshChunk& data, const StringBuf& debugLabel)
{
	DEBUG_CHECK_RETURN_EX_V(data.indexCount && data.vertexCount, "Empty chunk", nullptr);

	// allocate ID
	MeshChunkID id = 0;
	{
		auto lock = CreateLock(m_freeMeshChunkIDsLock);
		DEBUG_CHECK_RETURN_EX_V(!m_freeMeshChunkIDs.empty(), "To many mesh chunks", nullptr);
		id = m_freeMeshChunkIDs.back();
		m_freeMeshChunkIDs.popBack();
	}

	// unpack vertex data
	auto vertexData = data.packedVertexData.decompress();
    DEBUG_CHECK_RETURN_EX_V(vertexData, "Failed to decompress vertex data", nullptr);

	vertexData = UncompressVertexBuffer(vertexData.data(), vertexData.size(), data.vertexFormat, data.vertexCount);
	DEBUG_CHECK_RETURN_EX_V(vertexData, "Failed to unpack vertex data", nullptr);

	// index vertex data
	auto indexData = data.packedIndexData.decompress();
	DEBUG_CHECK_RETURN_EX_V(indexData, "Failed to decompress index data", nullptr);

	indexData = UncompressIndexBuffer(indexData.data(), indexData.size(), data.indexCount);
	DEBUG_CHECK_RETURN_EX_V(indexData, "Failed to unpack index data", nullptr);

	// create chunk from unpacked data
	return createStandaloneProxy(data, vertexData, indexData, debugLabel, id);
}

MeshChunkProxyStandalonePtr MeshService::createStandaloneProxy(const MeshChunk& data, const Buffer& vertexData, const Buffer& indexData, const StringBuf& debugLabel, MeshChunkID id)
{
	gpu::BufferObjectPtr vertexBuffer;
	gpu::BufferObjectPtr indexBuffer;

	// allocate vertex data
	{
		gpu::BufferCreationInfo info;
		info.allowVertex = true;
		info.label = debugLabel;
		info.size = vertexData.size();
		vertexBuffer = m_device->createBuffer(info, new gpu::SourceDataProviderBuffer(vertexData));
	}

	// allocate index data
	{
		gpu::BufferCreationInfo info;
		info.allowIndex = true;
		info.label = debugLabel;
		info.size = indexData.size();
		indexBuffer = m_device->createBuffer(info, new gpu::SourceDataProviderBuffer(indexData));
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

	return RefNew<MeshChunkProxy_Standalone>(setup, id);
}

//--

void MeshService::uploadChanges(gpu::CommandWriter& cmd) const
{
	m_meshletStorage->pushUpdates(cmd);
}

void MeshService::bindMeshData(gpu::CommandWriter& cmd) const
{
	//m_meshletStorage->bin
}

///--

END_BOOMER_NAMESPACE()