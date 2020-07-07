/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooker #]
***/

#include "build.h"
#include "renderingMeshCooker.h"
#include "renderingMeshCookerChunks.h"
#include "meshopt/meshoptimizer.h"
#include "mikktspace/mikktspace.h"

#include "base/geometry/include/mesh.h"

namespace rendering
{
    //---

    template< typename T >
    static base::Buffer UnpackQuadsToTriangles(const base::Buffer& buffer)
    {
        base::Buffer ret;

        const auto numQuads = (buffer.size() / sizeof(T)) / 4;
        if (numQuads > 0)
        {
            auto triDataSize = (numQuads * 6) * sizeof(T);
            ret.init(POOL_TEMP, triDataSize, 4);

            const auto* readPtr = (const T*)buffer.data();
            auto* writePtr = (T*)ret.data();
            for (uint32_t i = 0; i < numQuads; ++i, readPtr += 4, writePtr += 6)
            {
                writePtr[0] = readPtr[0];
                writePtr[1] = readPtr[1];
                writePtr[2] = readPtr[2];
                writePtr[3] = readPtr[0];
                writePtr[4] = readPtr[2];
                writePtr[5] = readPtr[3];
            }
        }

        return ret;
    }

    template< typename T >
    static void PackIndexData(const T* srcData, T* destData, uint32_t count, T vertexOffset)
    {
        if (vertexOffset == 0)
        {
            memcpy(destData, srcData, sizeof(uint32_t) * count);
        }
        else
        {
            const auto* srcDataEnd = srcData + count;
            while (srcData < srcDataEnd)
                *destData++ = *srcData++ + vertexOffset;
        }
    }

    template< typename T >
    static base::Buffer GenerateQuadTriListIndexBuffer(uint32_t numVertices)
    {
        base::Buffer ret;

        DEBUG_CHECK_EX(numVertices % 4 == 0, "Strange number of vertices for a quad mesh");
        const auto numQuads = numVertices / 4;
        if (numQuads > 0)
        {
            auto triDataSize = (numQuads * 6) * sizeof(T);
            ret.init(POOL_TEMP, triDataSize, 4);

            T vertexIndex = 0;
            auto* writePtr = (T*)ret.data();
            for (uint32_t i = 0; i < numQuads; ++i, vertexIndex += 4, writePtr += 6)
            {
                writePtr[0] = vertexIndex + 0;
                writePtr[1] = vertexIndex + 1;
                writePtr[2] = vertexIndex + 2;
                writePtr[3] = vertexIndex + 0;
                writePtr[4] = vertexIndex + 2;
                writePtr[5] = vertexIndex + 3;
            }
        }

        return ret;
    }

    template< typename T >
    static base::Buffer GenerateTriListIndexBuffer(uint32_t numVertices)
    {
        base::Buffer ret;

        DEBUG_CHECK_EX(numVertices % 3 == 0, "Strange number of vertices for a triangle mesh");
        const auto numTriangles = numVertices / 3;
        if (numTriangles > 0)
        {
            auto triDataSize = (numTriangles * 3) * sizeof(T);
            ret.init(POOL_TEMP, triDataSize, 4);

            T vertexIndex = 0;
            auto* writePtr = (T*)ret.data();
            for (uint32_t i = 0; i < numTriangles; ++i, vertexIndex += 3, writePtr += 3)
            {
                writePtr[0] = vertexIndex + 0;
                writePtr[1] = vertexIndex + 1;
                writePtr[2] = vertexIndex + 2;
            }
        }

        return ret;
    }

    //---


    struct MeshChunkMikktInterface
    {
        uint32_t numFaces = 0;
        uint32_t verticesPerFace = 3;
        base::Vector3* vertexData = nullptr;
        base::Vector3* normalData = nullptr;
        base::Vector3* tangentData = nullptr;
        base::Vector3* bitangentData = nullptr;
        base::Vector2* uvData = nullptr;

        static const MeshChunkMikktInterface& self(const SMikkTSpaceContext* pContext)
        {
            return *(const MeshChunkMikktInterface*)pContext->m_pUserData;
        }

        static int GetNumFaces(const SMikkTSpaceContext* pContext)
        {
            return self(pContext).numFaces;
        }

        static int GetNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
        {
            return self(pContext).verticesPerFace;
        }

        static void GetPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
        {
            const auto& s = self(pContext);

            const auto vertexIndex = (iFace * s.verticesPerFace) + iVert;
            fvPosOut[0] = s.vertexData[vertexIndex].x;
            fvPosOut[1] = s.vertexData[vertexIndex].y;
            fvPosOut[2] = s.vertexData[vertexIndex].z;
        }

        static void GetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
        {
            const auto& s = self(pContext);

            const auto vertexIndex = (iFace * s.verticesPerFace) + iVert;
            fvNormOut[0] = s.normalData[vertexIndex].x;
            fvNormOut[1] = s.normalData[vertexIndex].y;
            fvNormOut[2] = s.normalData[vertexIndex].z;
        }

        static void GetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
        {
            const auto& s = self(pContext);

            const auto vertexIndex = (iFace * s.verticesPerFace) + iVert;
            fvTexcOut[0] = s.uvData[vertexIndex].x;
            fvTexcOut[1] = s.uvData[vertexIndex].y;
        }

        static void SetTSpace(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fvBiTangent[], const float fMagS, const float fMagT, const tbool bIsOrientationPreserving, const int iFace, const int iVert)
        {
            const auto& s = self(pContext);

            const auto vertexIndex = (iFace * s.verticesPerFace) + iVert;

            s.tangentData[vertexIndex].x = fvTangent[0];
            s.tangentData[vertexIndex].y = fvTangent[1];
            s.tangentData[vertexIndex].z = fvTangent[2];

            s.bitangentData[vertexIndex].x = fvBiTangent[0];
            s.bitangentData[vertexIndex].y = fvBiTangent[1];
            s.bitangentData[vertexIndex].z = fvBiTangent[2];
        }

        MeshChunkMikktInterface(ImportChunk& importChunk)
        {
            numFaces = importChunk.numFaces;
            verticesPerFace = importChunk.faceQuads ? 4 : 3;
            vertexData = (base::Vector3*) importChunk.vertexStreamData(base::mesh::MeshStreamType::Position_3F);
            normalData = (base::Vector3*) importChunk.vertexStreamData(base::mesh::MeshStreamType::Normal_3F);
            tangentData = (base::Vector3*) importChunk.vertexStreamData(base::mesh::MeshStreamType::Tangent_3F);
            bitangentData = (base::Vector3*) importChunk.vertexStreamData(base::mesh::MeshStreamType::Binormal_3F);
            uvData = (base::Vector2*) importChunk.vertexStreamData(base::mesh::MeshStreamType::TexCoord0_2F);
        }

        static SMikkTSpaceInterface& GetInterface()
        {
            static SMikkTSpaceInterface ret;
            ret.m_getNormal = &GetNormal;
            ret.m_getNumFaces = &GetNumFaces;
            ret.m_getNumVerticesOfFace = &GetNumVerticesOfFace;
            ret.m_getPosition = &GetPosition;
            ret.m_getTexCoord = &GetTexCoord;
            ret.m_setTSpace = &SetTSpace;
            ret.m_setTSpaceBasic = nullptr;
            return ret;
        }
    };

    //---

    ImportChunk::ImportChunk(const base::mesh::MeshModelChunk& sourceChunk)
    {
        materialIndex = sourceChunk.materialIndex;
        renderMask = sourceChunk.renderMask.rawValue();
        detailMask = sourceChunk.detailMask;

        vertexDataStreams = sourceChunk.streams;
        vertexDataStreamMask = sourceChunk.streamMask;
        numVertices = sourceChunk.numVertices;

        faceQuads = (sourceChunk.topology == base::mesh::MeshTopologyType::Quads);
        numFaces = sourceChunk.numFaces;

        bounds = sourceChunk.bounds;
    }

    Buffer ImportChunk::buildTriangleListIndexBuffer() const
    {
        if (faceQuads)
            return GenerateQuadTriListIndexBuffer<uint32_t>(numVertices);
        else
            return GenerateTriListIndexBuffer<uint32_t>(numVertices);
    }

    //--

    bool ImportChunk::hasVertexStream(base::mesh::MeshStreamType stream) const
    {
        const auto mask = base::mesh::MeshStreamMaskFromType(stream);
        return 0 != (vertexDataStreamMask & mask);
    }

    bool ImportChunk::removeVertexStream(base::mesh::MeshStreamType stream)
    {
        for (uint32_t i = 0; i < vertexDataStreams.size(); ++i)
        {
            if (vertexDataStreams[i].type == stream)
            {
                vertexDataStreams.erase(i);
                return true;
            }
        }

        return false;
    }

    void* ImportChunk::vertexStreamData(base::mesh::MeshStreamType type)
    {
        for (const auto& currentStream : vertexDataStreams)
            if (currentStream.type == type)
                return currentStream.data.data();

        return nullptr;
    }

    base::mesh::MeshModelChunkData ImportChunk::createVertexStream(base::mesh::MeshStreamType type)
    {
        for (const auto& currentStream : vertexDataStreams)
            if (currentStream.type == type)
                return currentStream;

        auto& newStream = vertexDataStreams.emplaceBack();
        newStream.type = type;
        newStream.data = base::mesh::CreateMeshStreamBuffer(type, numVertices);
        return newStream;
    }

    //--

    bool ImportChunk::hasNormals() const
    {
        return hasVertexStream(base::mesh::MeshStreamType::Normal_3F);
    }

    void ImportChunk::computeFlatNormals()
    {
        auto data = createVertexStream(base::mesh::MeshStreamType::Normal_3F).data;

        auto* normalPtr = (base::Vector3*)data.data();
        auto* posPtr = (base::Vector3*)vertexStreamData(base::mesh::MeshStreamType::Position_3F);
            
        if (faceQuads)
        {
            for (uint32_t i = 0; i < numFaces; ++i, normalPtr += 4, posPtr += 4)
            {
                const auto normal = -base::TriangleNormal(posPtr[0], posPtr[1], posPtr[2]);
                normalPtr[0] = normal;
                normalPtr[1] = normal;
                normalPtr[2] = normal;
                normalPtr[3] = normal;
            }
        }
        else
        {
            for (uint32_t i = 0; i < numFaces; ++i, normalPtr += 3, posPtr += 3)
            {
                const auto normal = -base::TriangleNormal(posPtr[0], posPtr[1], posPtr[2]);
                normalPtr[0] = normal;
                normalPtr[1] = normal;
                normalPtr[2] = normal;
            }
        }
    }

    void ImportChunk::removeNormals()
    {
        removeVertexStream(base::mesh::MeshStreamType::Normal_3F);
    }

    //--

    bool ImportChunk::hasTangents() const
    {
        return hasVertexStream(base::mesh::MeshStreamType::Binormal_3F) && hasVertexStream(base::mesh::MeshStreamType::Tangent_3F);
    }

    void ImportChunk::removeTangentSpace()
    {
        removeVertexStream(base::mesh::MeshStreamType::Tangent_3F);
        removeVertexStream(base::mesh::MeshStreamType::Binormal_3F);
    }

    void ImportChunk::computeTangentSpace(float angleThreshold)
    {
        const auto texCoordMax = base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::TexCoord0_2F);
        if (0 == (vertexDataStreamMask & texCoordMax))
            return;

        createVertexStream(base::mesh::MeshStreamType::Tangent_3F);
        createVertexStream(base::mesh::MeshStreamType::Binormal_3F);

        MeshChunkMikktInterface data(*this);

        SMikkTSpaceContext context;
        context.m_pInterface = &MeshChunkMikktInterface::GetInterface();
        context.m_pUserData = &data;

        genTangSpace(&context, angleThreshold);
    }

    //---

    ImportChunkRegistry::ImportChunkRegistry()
    {}

    ImportChunkRegistry::~ImportChunkRegistry()
    {}

    //---

    BuildChunk::BuildChunk(MeshVertexFormat format, uint32_t material, uint32_t renderMask, uint32_t detailMask)
        : m_format(format)
        , m_material(material)
        , m_renderMask(renderMask)
        , m_detailMask(detailMask)
    {
        const auto& formatInfo = GetMeshVertexFormatInfo(format);
        if (formatInfo.quantizedPosition)
            m_quantizationGroup = 0; // use default group
    }

    BuildChunk::~BuildChunk()
    {
        m_sourceChunks.clearPtr();
    }

    void BuildChunk::addChunk(const ImportChunk& sourceChunk)
    {
        auto info = MemNew(SourceChunkInfo);
        info->vertexDataStreams = sourceChunk.vertexDataStreams;
        info->vertexDataStreamMask = sourceChunk.vertexDataStreamMask;
        info->numVertices = sourceChunk.numVertices;
        info->bounds = sourceChunk.bounds;

        info->indexData = sourceChunk.buildTriangleListIndexBuffer();
        info->numIndices = info->indexData.size() / sizeof(uint32_t);
        DEBUG_CHECK_EX(info->numIndices % 3 == 0, "Strange number of indices for a triangle mesh");

        {
            auto lock = CreateLock(m_sourceChunksLock);
            info->firstIndex = m_totalIndices;
            info->firstVertexIndex = m_totalVertices;
            m_totalIndices += info->numIndices;
            m_totalVertices += info->numVertices;
            m_sourceChunks.pushBack(info);
        }
    }

    void BuildChunk::pack(const MeshVertexQuantizationHelper& quantization, base::IProgressTracker& progress)
    {
        uint32_t outputVertexCount = m_totalVertices;
        uint32_t outputIndexCount = m_totalIndices;

        // get vertex size
        const uint32_t vertexSize = rendering::GetMeshVertexFormatInfo(m_format).stride;
        const uint32_t indexSize = sizeof(uint32_t);

        // allocate big output
        base::Buffer tempVertices, tempIndices;
        tempVertices.init(POOL_TEMP, vertexSize * outputVertexCount);
        tempIndices.init(POOL_TEMP, indexSize * outputIndexCount);

        // pack data
        {
            auto jobCount = m_sourceChunks.size() * 2;
            auto jobCounter = Fibers::GetInstance().createCounter("ChunkPackerWait", jobCount);
            for (const auto* sourceInfo : m_sourceChunks)
            {
                auto* vertexWritePtr = base::OffsetPtr(tempVertices.data(), sourceInfo->firstVertexIndex * vertexSize);
                auto* indexWritePtr = base::OffsetPtr(tempIndices.data(), sourceInfo->firstIndex * indexSize);

                base::Array<SourceMeshStream> sourceStreams;
                sourceStreams.reserve(sourceInfo->vertexDataStreams.size());
                for (const auto& sourceVertexStream : sourceInfo->vertexDataStreams)
                {
                    auto& entry = sourceStreams.emplaceBack();
                    entry.stream = sourceVertexStream.type;
                    entry.srcData = sourceVertexStream.data.data();
                    entry.srcDataStride = base::mesh::GetMeshStreamStride(sourceVertexStream.type);
                }

                if (progress.checkCancelation())
                {
                    Fibers::GetInstance().signalCounter(jobCounter, jobCount); // signal unsignalled amount
                    break;
                }

                RunChildFiber("PackVertexData") << [jobCounter, &quantization, sourceStreams, vertexWritePtr, this, sourceInfo](FIBER_FUNC)
                {
                    PackVertexData(quantization, sourceStreams.typedData(), sourceStreams.size(), vertexWritePtr, m_format, sourceInfo->numVertices);
                    Fibers::GetInstance().signalCounter(jobCounter);
                };

                RunChildFiber("PackIndexData") << [jobCounter, sourceInfo, indexWritePtr](FIBER_FUNC)
                {
                    PackIndexData<uint32_t>((const uint32_t*)sourceInfo->indexData.data(), (uint32_t*)indexWritePtr, sourceInfo->numIndices, sourceInfo->firstVertexIndex);
                    Fibers::GetInstance().signalCounter(jobCounter);
                };

                jobCount -= 2;
            }
            Fibers::GetInstance().waitForCounterAndRelease(jobCounter);
        }

        // conditional exit
        if (progress.checkCancelation())
            return;

        // TOOD: optimize more
        if (m_mergeDuplicatedVertices)
        {
            base::ScopeTimer timer;

            base::Array<uint32_t> remapTable;
            const auto newVertexCount = OptimizeVertexBuffer(tempVertices.data(), outputVertexCount, m_format, remapTable);
            tempVertices = RemapVertexBuffer(tempVertices.data(), outputVertexCount, m_format, newVertexCount, remapTable.typedData());
            RemapIndexBuffer((uint32_t*)tempIndices.data(), outputIndexCount, remapTable.typedData());

            TRACE_INFO("Optimized vertices {}->{} in {}", outputVertexCount, newVertexCount, timer);
            outputVertexCount = newVertexCount;

            if (m_optimizeVertexCache)
            {
                base::ScopeTimer timer;
                OptimizeVertexCache((uint32_t*)tempIndices.data(), outputIndexCount, outputVertexCount); // TODO: optional cancelation
                TRACE_INFO("Optimized vertex cache for {} indices in {}", outputIndexCount, timer);
            }

            // conditional exit
            if (progress.checkCancelation())
                return;

            if (m_optimizeVertexFetch)
            {
                base::ScopeTimer timer;
                tempVertices = OptimizeVertexFetch(tempVertices.data(), outputVertexCount, m_format, (uint32_t*)tempIndices.data(), outputIndexCount); // TODO: optional internal cancelation
                TRACE_INFO("Optimized vertex fetch for {} vertices in {}", outputVertexCount, timer);
            }
        }

        // conditional exit
        if (progress.checkCancelation())
            return;

        // export
        m_finalIndexCount = outputIndexCount;
        m_finalVertexCount = outputVertexCount;

        // pack vertices
        {
            base::ScopeTimer timer;
            m_unpackedVertexDataSize = tempVertices.size();
            m_packedVertexData = CompressVertexBuffer(tempVertices.data(), m_format, outputVertexCount);
            if (m_packedVertexData)
            {
                TRACE_INFO("Packed vertex buffer ({} vertices) {} -> {} in {}", outputVertexCount, MemSize(tempVertices.size()), MemSize(m_packedVertexData.size()), timer);
            }
            else
            {
                m_packedVertexData = tempVertices;
            }
        }

        // conditional exit
        if (progress.checkCancelation())
            return;

        // pack indices
        {
            base::ScopeTimer timer;
            m_unpackedIndexDataSize = tempIndices.size();
            m_packedIndexData = CompressIndexBuffer(tempIndices.data(), outputIndexCount, outputVertexCount);
            if (m_packedIndexData)
            {
                TRACE_INFO("Packed index buffer ({} indices) {} -> {} in {}", outputIndexCount, MemSize(tempIndices.size()), MemSize(m_packedIndexData.size()), timer);
            }
            else
            {
                m_packedIndexData = tempIndices;
            }
        }
    }

    //---

    BuildChunkRegistry::BuildChunkRegistry()
    {
        m_buildChunks.reserve(32);
    }

    BuildChunkRegistry::~BuildChunkRegistry()
    {
    }

    BuildChunk* BuildChunkRegistry::findBuildChunk(MeshVertexFormat format, uint32_t material, uint32_t renderMask, uint32_t detailMask)
    {
        renderMask &= ~PACKED_RENDER_MASK;
        if (!renderMask)
            return nullptr; // skip non-rederable chunks

        for (const auto& chunk : m_buildChunks)
            if (chunk->m_format == format && chunk->m_material == material && chunk->m_detailMask == detailMask && chunk->m_renderMask == renderMask)
                return chunk;

        auto chunk = MemNew(BuildChunk, format, material, renderMask, detailMask);
        m_buildChunks.pushBack(chunk);
        return chunk;
    }

    //---
    
} // rendering
