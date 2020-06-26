/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#include "build.h"
#include "renderingMesh.h"
#include "renderingMeshFormat.h"
#include "renderingMeshService.h"
#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingCommandWriter.h"

namespace rendering
{

    ///---

    RTTI_BEGIN_TYPE_CLASS(MeshService);
    RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<rendering::DeviceService>();
    RTTI_END_TYPE();

    ///----

    base::ConfigProperty<uint32_t> cvInitialVertexPoolSizeMB("Rendering.MeshPool", "InitialVertexPoolSizeMB", 512);
    base::ConfigProperty<uint32_t> cvInitialIndexPoolSizeMB("Rendering.MeshPool", "InitialIndexPoolSizeMB", 512);
    base::ConfigProperty<uint32_t> cvMaxVertexPoolSizeMB("Rendering.MeshPool", "InitialVertexPoolSizeMB", 2048);
    base::ConfigProperty<uint32_t> cvMaxIndexPoolSizeMB("Rendering.MeshPool", "InitialIndexPoolSizeMB", 1024);

    base::ConfigProperty<uint32_t> cvMaxUniqueMeshChunks("Rendering.MeshPool", "MaxUniqueMeshChunks", 65535);

    ///----

#pragma pack(push)
#pragma pack(4)
    struct GPUMeshChunkInfo
    {
        uint32_t firstWordInVertexBuffer;
        uint32_t numWordsPerVertex;
        uint32_t firstWordInIndexBuffer;
        uint32_t numVertices;
        uint32_t numIndices;
        uint32_t padding0;
        uint32_t padding1;
        uint32_t padding2;
        base::Vector4 quantizationOffset; // w unused
        base::Vector4 quantizationScale; // w unused
    };
#pragma pack(pop)

    ///----

    MeshService::MeshService()
    {}

    base::app::ServiceInitializationResult MeshService::onInitializeService(const base::app::CommandLine& cmdLine)
    {
        m_maxMeshChunks = std::clamp<uint32_t>(cvMaxUniqueMeshChunks.get(), 256, MAX_MESH_CHUNKS_ABSOLUTE_LIMIT);
        TRACE_INFO("Rendering will register {} max mesh chunks", m_maxMeshChunks);

        auto deviceService = base::GetService<DeviceService>();
        if (!deviceService)
            base::app::ServiceInitializationResult::FatalError;

        auto device = deviceService->device();
        
        {
            BufferCreationInfo info;
            info.allowIndex = true;
            info.allowUAV = true;
            info.allowShaderReads = true;
            info.label = "MeshChunkIndices";
            info.size = cvInitialIndexPoolSizeMB.get() << 20;
            m_managedIndices.create(info, 256);
        }

        {
            BufferCreationInfo info;
            info.allowVertex = true;
            info.allowUAV = true;
            info.allowShaderReads = true;
            info.label = "MeshChunkVertices";
            info.size = cvInitialVertexPoolSizeMB.get() << 20;
            m_managedVertices.create(info, 256);
        }

        {
            BufferCreationInfo info;
            info.allowVertex = false;
            info.allowUAV = true;
            info.allowShaderReads = true;
            info.label = "MeshChunkInfo";
            info.size = sizeof(GPUMeshChunkInfo) * m_maxMeshChunks;
            info.stride = sizeof(GPUMeshChunkInfo);
            m_managedChunkInfo.create(info);
        }

        {
            m_chunkInfos.resize(65536);
            memset(m_chunkInfos.data(), 0, m_chunkInfos.dataSize());

            m_freeIds.reserve(65535);
            for (uint32_t i = 65545; i >= 1; --i)
                m_freeIds.pushBack(i);
        }

        return base::app::ServiceInitializationResult::Finished;
    }

    void MeshService::onShutdownService()
    {
        MeshChunkRenderID id = 0;
        for (auto& entry : m_chunkInfos)
        {
            if (entry.allocated)
                unregisterChunkData(id);
            id += 1;
        }

        m_managedIndices->advanceFrame();
        m_managedIndices.reset();

        m_managedVertices->advanceFrame();
        m_managedVertices.reset();

        m_managedChunkInfo.reset();

        m_closed = true;
    }

    void MeshService::onSyncUpdate()
    {
        m_managedVertices->advanceFrame();
        m_managedIndices->advanceFrame();
    }

    //--

    struct GPUMeshChunksData
    {
        BufferView indexBuffer;
        BufferView vertexBuffer;
        BufferView chunkInfoBuffer;
    };

    void MeshService::prepareForFrame(command::CommandWriter& cmd)
    {
        // push data
        m_managedIndices->update(cmd);
        m_managedVertices->update(cmd);
        m_managedChunkInfo->update(cmd);

        // bind parameters
        {
            GPUMeshChunksData params;
            params.indexBuffer = m_managedIndices->bufferView();
            params.vertexBuffer = m_managedVertices->bufferView();
            params.chunkInfoBuffer = m_managedChunkInfo->bufferView();

            cmd.opBindParametersInline("MeshBuffers"_id, params);
        }
    }

    void MeshService::prepareForDraw(command::CommandWriter& cmd) const
    {
        cmd.opBindIndexBuffer(m_managedIndices->bufferView(), ImageFormat::R32_UINT);
    }

    ///---

    MeshChunkRenderID MeshService::registerChunkData(const MeshChunk& data)
    {
        // already closed
        if (m_closed)
            return 0;

        // noting to allocated
        DEBUG_CHECK_EX(data.unpackedIndexSize && data.unpackedVertexSize, "Nothing to allocate");
        if (!data.unpackedIndexSize || !data.unpackedVertexSize)
            return 0;

        // unpack vertex data
        auto vertexData = data.packedVertexData;
        if (data.unpackedVertexSize != data.packedVertexData.size())
        {
            vertexData = UncompressVertexBuffer(data.packedVertexData.data(), data.packedVertexData.size(), data.vertexFormat, data.vertexCount);
            if (!vertexData)
                return 0;
        }

        // index vertex data
        auto indexData = data.packedIndexData;
        if (data.unpackedIndexSize != data.packedIndexData.size())
        {
            indexData = UncompressIndexBuffer(data.packedIndexData.data(), data.packedIndexData.size(), data.indexCount);
            if (!indexData)
                return 0;
        }

        // find space for vertices
        ManagedBufferBlock vertexBlock;
        if (!m_managedVertices->allocateBlock(vertexData, vertexBlock))
        {
            TRACE_ERROR("RENDERING ERROR: Out of memory for vertex data of size {}", data.packedVertexData);
            return 0;
        }

        // find space for indices
        ManagedBufferBlock indexBlock;
        if (!m_managedIndices->allocateBlock(indexData, indexBlock))
        {
            TRACE_ERROR("RENDERING ERROR: Out of memory for index data of size {}", data.packedIndexData);
            m_managedVertices->freeBlock(vertexBlock);
            return 0;
        }
        

        // allocate ID
        MeshChunkRenderID id = 0;
        {
            auto lock = CreateLock(m_freeIdLock);

            // no more IDs
            if (m_freeIds.empty())
            {
                TRACE_ERROR("RENDERING ERROR: Out of free mesh chunk IDs. There are to many loaded/used meshes.");
                m_managedVertices->freeBlock(vertexBlock);
                m_managedIndices->freeBlock(indexBlock);
                return 0;
            }

            id = m_freeIds.back();
            m_freeIds.popBack();
        }

        // update data
        auto lock = CreateLock(m_chunkInfosLock);

        // setup entry
        auto& entry = m_chunkInfos[id];
        DEBUG_CHECK_EX(0 == entry.allocated.exchange(1), "Entry should not be allocated");
        entry.format = data.vertexFormat;
        entry.indexBlock = indexBlock;
        entry.vertexBlock = vertexBlock;
        entry.indexCount = data.indexCount;
        entry.vertexCount = data.vertexCount;

        // update GPU chunk entry
        GPUMeshChunkInfo gpuChunk;
        gpuChunk.numIndices = data.indexCount;
        gpuChunk.numVertices = data.vertexCount;
        gpuChunk.numWordsPerVertex = GetMeshVertexFormatInfo(data.vertexFormat).stride / 4;
        gpuChunk.firstWordInIndexBuffer = indexBlock.bufferOffset / 4;
        gpuChunk.firstWordInVertexBuffer = vertexBlock.bufferOffset / 4;
        gpuChunk.quantizationOffset = data.quantizationOffset;
        gpuChunk.quantizationScale = data.quantizationScale;
        DEBUG_CHECK(indexBlock.bufferOffset % 4 == 0);
        DEBUG_CHECK(vertexBlock.bufferOffset % 4 == 0);
        m_managedChunkInfo->writeAtIndex(id, gpuChunk);
        TRACE_INFO("MeshChunk++: {}, vertex {} @ {}, index {} @ {}", id, gpuChunk.numVertices, gpuChunk.firstWordInVertexBuffer, gpuChunk.numIndices, gpuChunk.firstWordInIndexBuffer);
        return id;
    }

    void MeshService::unregisterChunkData(MeshChunkRenderID data)
    {
        if (data && !m_closed)
        {
            auto& entry = m_chunkInfos[data];
            DEBUG_CHECK_EX(1 == entry.allocated.exchange(0), "Entry should be allocated");

            m_managedVertices->freeBlock(entry.vertexBlock);
            m_managedIndices->freeBlock(entry.indexBlock);
            TRACE_INFO("MeshChunk--:  {}", data);

            {
                auto lock = CreateLock(m_freeIdLock);
                m_freeIds.pushBack(data);
            }
        }
    }

    ///---

} // rendering