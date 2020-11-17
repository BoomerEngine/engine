/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#pragma once

#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/mesh/include/renderingMeshFormat.h"
#include "rendering/mesh/include/renderingMeshStreamData.h"

namespace rendering
{
    //---

    struct ImportChunk : public base::IReferencable
    {
        uint32_t renderMask = 0;
        uint32_t detailMask = 0;
        uint32_t materialIndex = 0;

        Array<MeshRawChunkData> vertexDataStreams;
        MeshStreamMask vertexDataStreamMask;
        uint32_t numVertices = 0;

        uint32_t numFaces = 0;
        bool faceQuads = false;

        base::Box bounds;

        ImportChunk(const MeshRawChunk& sourceChunk);

        //--

        Buffer buildTriangleListIndexBuffer() const;

        //--

        bool hasVertexStream(MeshStreamType stream) const;
        bool removeVertexStream(MeshStreamType stream);
        MeshRawChunkData createVertexStream(MeshStreamType stream);
        void* vertexStreamData(MeshStreamType stream);

        //--

        bool hasNormals() const;
        void computeFlatNormals();
        void removeNormals();

        //--

        bool hasTangents() const;
        void removeTangentSpace();
        void computeTangentSpace(float angleThreshold);
    };

    struct ImportChunkRegistry : public base::NoCopy
    {
        base::Array<base::RefPtr<ImportChunk>> importChunks;
        base::Box bounds;

        ImportChunkRegistry();
        ~ImportChunkRegistry();
    };

    //---

    struct BuildChunk : public base::IReferencable
    {
        MeshVertexFormat m_format = MeshVertexFormat::Static;
        uint32_t m_material = 0;
        uint32_t m_renderMask = ~0U;
        uint32_t m_detailMask = 1;

        uint32_t m_totalVertices = 0;
        uint32_t m_totalIndices = 0;

        struct SourceChunkInfo : public mem::GlobalPoolObject<POOL_MESH_BUILDER>
        {
            Array<MeshRawChunkData> vertexDataStreams;
            MeshStreamMask vertexDataStreamMask;
            Buffer indexData;

            uint32_t numVertices = 0;
            uint32_t numIndices = 0;

            uint32_t firstVertexIndex = 0;
            uint32_t firstIndex = 0;

            base::Box bounds;
        };

        base::Array<SourceChunkInfo*> m_sourceChunks;
        base::SpinLock m_sourceChunksLock;

        uint32_t m_unpackedVertexDataSize = 0;
        uint32_t m_unpackedIndexDataSize = 0;
        base::Buffer m_packedVertexData;
        base::Buffer m_packedIndexData;

        uint32_t m_finalIndexCount = 0;
        uint32_t m_finalVertexCount = 0;

        int m_quantizationGroup = -1; // not quantized
        base::Vector3 m_quantizationOffset = base::Vector3(0,0,0);
        base::Vector3 m_quantizationScale = base::Vector3(1,1,1);

        bool m_mergeDuplicatedVertices = true;
        bool m_optimizeVertexCache = true;
        bool m_optimizeVertexFetch = true;

        //--        

        BuildChunk(MeshVertexFormat format, uint32_t material, uint32_t renderMask, uint32_t detailMask);
        ~BuildChunk();

        void addChunk(const ImportChunk& sourceChunk);
        void pack(const MeshVertexQuantizationHelper& quantization, base::IProgressTracker& progress);
    };

    //---

    struct BuildChunkRegistry : public base::NoCopy
    {
        BuildChunkRegistry();
        ~BuildChunkRegistry();

        const uint32_t PACKED_RENDER_MASK = (uint32_t)MeshChunkRenderingMaskBit::RENDERABLE;

        BuildChunk* findBuildChunk(MeshVertexFormat format, uint32_t material, uint32_t renderMask, uint32_t detailMask);

        //

        base::Array<base::RefPtr<BuildChunk>> m_buildChunks;
    };

    //---

} // rendering
