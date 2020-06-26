/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "renderingMeshFormat.h"
#include "base/app/include/localService.h"
#include "rendering/driver/include/renderingBufferView.h"
#include "base/containers/include/blockPool.h"
#include "rendering/driver/include/renderingManagedBuffer.h"
#include "rendering/driver/include/renderingManagedBufferWithAllocator.h"

namespace rendering
{

    //---

    // rendering chunk data
    struct MeshRenderingChunkInfo
    {
        std::atomic<uint32_t> allocated = 0;

        uint32_t indexCount = 0;
        uint32_t vertexCount = 0;
        MeshVertexFormat format;
        ManagedBufferBlock vertexBlock;
        ManagedBufferBlock indexBlock;
    };

    // stats
    struct MeshChunkAllocationStats
    {
        uint32_t numAllocatedChunks = 0;
        uint32_t maxAllocatedChunks = 0;
        uint32_t numVertexBytes = 0;
        uint32_t maxVertexBytes = 0;
        uint32_t numIndexBytes = 0;
        uint32_t maxIndexBytes = 0;
    };

    //---

    // service holding and managing all baked (static) geometry data that is rendered using meshlets/mesh shaders
    class RENDERING_MESH_API MeshService : public base::app::ILocalService
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshService, base::app::ILocalService);

    public:
        MeshService();

        //--

        /// get chunk information
        INLINE const MeshRenderingChunkInfo& chunkInfo(MeshChunkRenderID index) const { return m_chunkInfos[index]; }

        /// get current stats
        INLINE const MeshChunkAllocationStats& stats() const { return m_stats; }

        //--

        /// register mesh chunk
        MeshChunkRenderID registerChunkData(const MeshChunk& data);

        /// unregister chunk data
        void unregisterChunkData(MeshChunkRenderID data);

        //--

        /// push changed for frame rendering
        void prepareForFrame(command::CommandWriter& cmd);

        /// prepare for drawing in a given command buffer - usually just bind the index buffer
        void prepareForDraw(command::CommandWriter& cmd) const;

        //--

    private:
        virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
        virtual void onShutdownService() override final;
        virtual void onSyncUpdate() override final;

        static const uint32_t MAX_MESH_CHUNKS_ABSOLUTE_LIMIT = 65535;

        uint32_t m_maxMeshChunks = 0;

        base::Array<MeshChunkRenderID> m_freeIds;
        base::SpinLock m_freeIdLock;

        base::Array<MeshRenderingChunkInfo> m_chunkInfos;
        base::SpinLock m_chunkInfosLock;
        MeshChunkAllocationStats m_stats;

        base::UniquePtr<ManagedBufferWithAllocator> m_managedIndices;
        base::UniquePtr<ManagedBufferWithAllocator> m_managedVertices;
        base::UniquePtr<ManagedBuffer> m_managedChunkInfo;

        bool m_closed = false;
    };

    //---

} // rendering