/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "renderingMeshFormat.h"
#include "base/app/include/localService.h"
#include "base/containers/include/blockPool.h"
#include "rendering/device/include/renderingManagedBuffer.h"
#include "rendering/device/include/renderingManagedBufferWithAllocator.h"

namespace rendering
{

	//---

    class IMeshChunkProxy;
	class MeshChunkSharedStorage;

	//---

    // service for managing mesh data buffers
    class RENDERING_MESH_API MeshService : public base::app::ILocalService
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshService, base::app::ILocalService);

    public:
        MeshService();

        //--

        /// allocate chunk proxy that can be use to render mesh content
		MeshChunkProxyPtr createChunkProxy(const MeshChunk& data, const base::StringBuf& debugLabel = "");

        //--

        /// upload changes to any internal data structures
        void uploadChanges(command::CommandWriter& cmd) const;

        /// bind mesh data buffers (chunk info)
        void bindMeshData(command::CommandWriter& cmd) const;

        //--

    private:
        virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
        virtual void onShutdownService() override final;
        virtual void onSyncUpdate() override final;

		//--

		base::RefPtr<MeshChunkSharedStorage> m_meshletStorage = nullptr;

		IDevice* m_device = nullptr;

        //--

        BufferObjectPtr m_chunkInfoBuffer;
        BufferViewPtr m_chunkInfoBufferSRV;

#pragma pack(push)
#pragma pack(4)
        struct GPUChunkInfo
        {
            base::Vector4 quantizationMin;
            base::Vector4 quantizationMax;
            uint32_t vertexDataOffset;
            uint32_t indexDataOffset;
        };
#pragma pack(pop)

		//--

        base::SpinLock m_freeMeshChunkIDsLock;
        base::Array<MeshChunkID> m_freeMeshChunkIDs;

        void releaseMeshChunkId(MeshChunkID id);

        friend class IMeshChunkProxy;

        //---

		MeshChunkProxyStandalonePtr createStandaloneProxy(const MeshChunk& data, const base::Buffer& vertexData, const base::Buffer& indexData, const base::StringBuf& debugLabel, MeshChunkID id);
    };

    //---

} // rendering