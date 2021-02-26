/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "renderingMeshFormat.h"
#include "core/app/include/localService.h"
#include "core/containers/include/blockPool.h"
#include "gpu/device/include/renderingManagedBuffer.h"
#include "gpu/device/include/renderingManagedBufferWithAllocator.h"

BEGIN_BOOMER_NAMESPACE()

//---

class IMeshChunkProxy;
class MeshChunkSharedStorage;

//---

// service for managing mesh data buffers
class ENGINE_MESH_API MeshService : public app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshService, app::ILocalService);

public:
    MeshService();

    //--

    /// allocate chunk proxy that can be use to render mesh content
	MeshChunkProxyPtr createChunkProxy(const MeshChunk& data, const StringBuf& debugLabel = "");

    //--

    /// upload changes to any internal data structures
    void uploadChanges(gpu::CommandWriter& cmd) const;

    /// bind mesh data buffers (chunk info)
    void bindMeshData(gpu::CommandWriter& cmd) const;

    //--

private:
    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

	//--

	RefPtr<MeshChunkSharedStorage> m_meshletStorage = nullptr;

    gpu::IDevice* m_device = nullptr;

    //--

    gpu::BufferObjectPtr m_chunkInfoBuffer;
    gpu::BufferViewPtr m_chunkInfoBufferSRV;

#pragma pack(push)
#pragma pack(4)
    struct GPUChunkInfo
    {
        Vector4 quantizationMin;
        Vector4 quantizationMax;
        uint32_t vertexDataOffset;
        uint32_t indexDataOffset;
    };
#pragma pack(pop)

	//--

    SpinLock m_freeMeshChunkIDsLock;
    Array<MeshChunkID> m_freeMeshChunkIDs;

    void releaseMeshChunkId(MeshChunkID id);

    friend class IMeshChunkProxy;

    //---

	MeshChunkProxyStandalonePtr createStandaloneProxy(const MeshChunk& data, const Buffer& vertexData, const Buffer& indexData, const StringBuf& debugLabel, MeshChunkID id);
};

//---

END_BOOMER_NAMESPACE()
