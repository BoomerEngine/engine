/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#include "build.h"
#include "fbxFileData.h"
#include "fbxFileLoaderService.h"

#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/containers/include/inplaceArray.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"

namespace fbx
{
    //---

    MaterialEntry::MaterialEntry()
    {}

    MaterialMapper::MaterialMapper()
    {
        materials.reserve(256);
        materialsMapping.reserve(256);
    }

    uint32_t MaterialMapper::addMaterial(const char* materialName, const fbxsdk::FbxSurfaceMaterial* material)
    {
        if (!materialName || !*materialName)
            materialName = "DefaultMaterial";

        uint32_t ret = 0;
        if (materialsMapping.find(base::StringView(materialName), ret))
            return ret;

        auto& entry = materials.emplaceBack();
        entry.name = base::StringBuf(materialName);
        entry.sourceData = material; // may be NULL

        materialsMapping[entry.name] = materials.lastValidIndex();
        return materials.lastValidIndex();
    }

    //---

    SkeletonBuilder::SkeletonBuilder()
    {
        m_skeletonBones.reserve(256);
        m_skeletonBonesMapping.reserve(256);
        m_skeletonRawBonesMapping.reserve(256);
    }

    uint32_t SkeletonBuilder::addBone(const LoadedFile& owner, const DataNode* node)
    {
        // invalid node, map to root
        if (!node)
            return 0;

        // find mapping
        uint32_t index = 0;
        if (m_skeletonBonesMapping.find(node, index))
            return index;

        // map parent
        SkeletonBone info;
        if (node->m_parent != nullptr)
            info.m_parentIndex = addBone(owner, node->m_parent);
        else
            info.m_parentIndex = -1;
        info.m_data = node;

        // store
        index = m_skeletonBones.size();
        m_skeletonBones.pushBack(info);

        // map
        m_skeletonBonesMapping[node] = index;
        m_skeletonRawBonesMapping[node->m_node] = index;
        return index;
    }

    uint32_t SkeletonBuilder::addBone(const LoadedFile& owner, const fbxsdk::FbxNode* node)
    {
        // invalid node, map to root
        if (!node)
            return 0;

        // find mapping
        uint32_t index = 0;
        if (m_skeletonRawBonesMapping.find(node, index))
            return index;

        // find the actual node
        auto actualNode  = owner.findDataNode(node);
        return addBone(owner, actualNode);
    }

    //---

    struct MeshVertexInfluence
    {
        uint32_t m_indices[4];
        float m_weights[4];
        uint8_t m_numBones;

        void add(uint32_t index, float weight)
        {
            if (m_numBones < ARRAY_COUNT(m_indices))
            {
                m_indices[m_numBones] = index;
                m_weights[m_numBones] = weight;
                m_numBones += 1;
            }
            else
            {
                uint32_t smallest = 0;
                for (uint32_t i=1; i<ARRAY_COUNT(m_indices); ++i)
                {
                    if (m_weights[i] < m_weights[smallest])
                    {
                        smallest = i;
                    }
                }

                if (weight > m_weights[smallest])
                {
                    m_indices[smallest] = index;
                    m_weights[smallest] = weight;
                }
            }
        }

        void normalize()
        {
            float weightSum = 0.0f;
            for (uint32_t i=0; i<ARRAY_COUNT(m_indices); ++i)
            {
                weightSum += m_weights[i];
            }

            if (weightSum > 0)
            {
                for (uint32_t i=0; i<ARRAY_COUNT(m_indices); ++i)
                {
                    m_weights[i] /= weightSum;
                }
            }
        }
    };

    struct NormalVector
    {
        base::Vector3 vector;
    };

    struct SkinIndices
    {
        uint8_t indices[4];
    };

    struct SkinWeights
    {
        float weights[4];
    };

    struct Position
    {
        base::Vector3 vector;
    };

    static void ExtractStreamElement(const FbxVector4& source, Position& writeTo, const base::Matrix& localToWorld)
    {
        writeTo.vector.x = source[0];
        writeTo.vector.y = source[1];
        writeTo.vector.z = source[2];
        writeTo.vector = localToWorld.transformPoint(writeTo.vector);
    }

    static void ExtractStreamElement(const FbxVector4& source, NormalVector& writeTo, const base::Matrix& localToWorld)
    {
        writeTo.vector.x = source[0];
        writeTo.vector.y = source[1];
        writeTo.vector.z = source[2];
        writeTo.vector = localToWorld.transformVector(writeTo.vector);
        writeTo.vector.normalize();
    }

    static void ExtractStreamElement(const FbxVector4& source, base::Vector3& writeTo, const base::Matrix& localToWorld)
    {
        writeTo.x = source[0];
        writeTo.y = source[1];
        writeTo.z = source[2];
    }

    static void ExtractStreamElement(const FbxVector2& source, base::Vector2& writeTo, const base::Matrix& localToWorld)
    {
        writeTo.x = source[0];
        writeTo.y = source[1];
    }

    static void ExtractStreamElement(const FbxColor& source, base::Color& writeTo, const base::Matrix& localToWorld)
    {
        writeTo.r = base::FloatTo255(source.mRed);
        writeTo.g = base::FloatTo255(source.mGreen);
        writeTo.b = base::FloatTo255(source.mBlue);
        writeTo.a = base::FloatTo255(source.mAlpha);
    }

    /*static void ExtractStreamElement(const FbxVector4& source, base::Vector3& writeTo)
    {
    }*/

    template< typename T, typename DataType >
    static void ExtractStreamData(const T* stream, const base::Matrix& localToWorld, uint32_t vertexIndex, uint32_t controlPointIndex, DataType& writeTo)
    {
        if (stream)
        {
            if (stream->GetMappingMode() == FbxGeometryElement::eByControlPoint)
            {
                switch (stream->GetReferenceMode())
                {
                    case FbxGeometryElement::eDirect:
                    {
                        ExtractStreamElement(stream->GetDirectArray().GetAt(controlPointIndex), writeTo, localToWorld);
                        break;
                    }

                    case FbxGeometryElement::eIndexToDirect:
                    {
                        uint32_t id = stream->GetIndexArray().GetAt(controlPointIndex);
                        ExtractStreamElement(stream->GetDirectArray().GetAt(id), writeTo, localToWorld);
                        break;
                    }
                }
            }
            else if (stream->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
            {
                switch (stream->GetReferenceMode())
                {
                    case FbxGeometryElement::eDirect:
                    {
                        ExtractStreamElement(stream->GetDirectArray().GetAt(vertexIndex), writeTo, localToWorld);
                        break;
                    }

                    case FbxGeometryElement::eIndexToDirect:
                    {
                        uint32_t id = stream->GetIndexArray().GetAt(vertexIndex);
                        ExtractStreamElement(stream->GetDirectArray().GetAt(id), writeTo, localToWorld);
                        break;
                    }
                }
            }
        }
    }

    template< typename T, typename DataType >
    static void ExtractStreamDataUV(const T* stream, const base::Matrix& localToWorld, uint32_t vertexIndex, uint32_t controlPointIndex, DataType& writeTo, bool flipUV)
    {
        if (stream)
        {
            if (stream->GetMappingMode() == FbxGeometryElement::eByControlPoint)
            {
                switch (stream->GetReferenceMode())
                {
                case FbxGeometryElement::eDirect:
                {
                    ExtractStreamElement(stream->GetDirectArray().GetAt(controlPointIndex), writeTo, localToWorld);
                    break;
                }

                case FbxGeometryElement::eIndexToDirect:
                {
                    uint32_t id = stream->GetIndexArray().GetAt(controlPointIndex);
                    ExtractStreamElement(stream->GetDirectArray().GetAt(id), writeTo, localToWorld);
                    break;
                }
                }
            }
            else if (stream->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
            {
                switch (stream->GetReferenceMode())
                {
                    case FbxGeometryElement::eIndexToDirect:
                    case FbxGeometryElement::eDirect:
                    {
                        ExtractStreamElement(stream->GetDirectArray().GetAt(vertexIndex), writeTo, localToWorld);
                        break;
                    }
                }
            }

            if (flipUV)
                writeTo.y = 1.0f - writeTo.y;
        }
    }

    struct FBXMeshStreams : public base::NoCopy
    {
        const FbxVector4* positions = nullptr;
        const FbxGeometryElementNormal* normals = nullptr;
        const FbxGeometryElementBinormal* bitangents = nullptr;
        const FbxGeometryElementTangent* tangents = nullptr;
        const FbxGeometryElementVertexColor* color0 = nullptr;
        const FbxGeometryElementVertexColor* color1 = nullptr;
        const FbxGeometryElementUV* uv0 = nullptr;
        const FbxGeometryElementUV* uv1 = nullptr;

        const MeshVertexInfluence* skinInfluences = nullptr;

        base::mesh::MeshStreamMask streamMask = 0;

        bool flipUV = true;
        bool flipFaces = false;

        const fbxsdk::FbxMesh* mesh = nullptr;

        FBXMeshStreams(const fbxsdk::FbxMesh* mesh_, const base::Array<MeshVertexInfluence>& skinningData)
            : mesh(mesh_)
        {
            // get mesh flags
            auto hasNormal = (mesh->GetElementNormalCount() >= 1);
            auto hasTangents = (mesh->GetElementTangentCount() >= 1);
            auto hasBitangents = (mesh->GetElementBinormalCount() >= 1);
            auto hasUV0 = (mesh->GetElementUVCount() >= 1);
            auto hasUV1 = (mesh->GetElementUVCount() >= 2);
            auto hasColor0 = (mesh->GetElementVertexColorCount() >= 1);
            auto hasColor1 = (mesh->GetElementVertexColorCount() >= 2);

            // get input data streams
            positions = mesh->GetControlPoints();
            normals = hasNormal ? mesh->GetElementNormal(0) : NULL;
            tangents = hasTangents ? mesh->GetElementTangent(0) : NULL;
            bitangents = hasBitangents ? mesh->GetElementBinormal(0) : NULL;
            color0 = hasColor0 ? mesh->GetElementVertexColor(0) : NULL;
            color1 = hasColor1 ? mesh->GetElementVertexColor(1) : NULL;
            uv0 = hasUV0 ? mesh->GetElementUV(0) : NULL;
            uv1 = hasUV1 ? mesh->GetElementUV(1) : NULL;

            // determine stream mask for all chunks in this model
            streamMask = base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Position_3F);
            if (normals)
                streamMask |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Normal_3F);
            if (bitangents)
                streamMask |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Binormal_3F);
            if (tangents)
                streamMask |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Tangent_3F);
            if (color0)
                streamMask |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Color0_4U8);
            if (color1)
                streamMask |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Color1_4U8);
            if (uv0)
                streamMask |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::TexCoord0_2F);
            if (uv1)
                streamMask |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::TexCoord1_2F);

            // skinning data
            if (!skinningData.empty())
            {
                skinInfluences = skinningData.typedData();
                streamMask |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::SkinningIndices_4U8);
                streamMask |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::SkinningWeights_4F);

                // TODO: 8 bone skinning
            }
        }
    };

    struct MeshWriteStreams
    {
        Position* writePosition = nullptr;
        NormalVector* writeNormal = nullptr;
        NormalVector* writeTangent = nullptr;
        NormalVector* writeBitangent = nullptr;
        base::Vector2* writeUV0 = nullptr;
        base::Vector2* writeUV1 = nullptr;
        base::Color* writeColor0 = nullptr;
        base::Color* writeColor1 = nullptr;
        SkinIndices* writeSkinIndices = nullptr;
        SkinWeights* writeSkinWeights = nullptr;

        base::mesh::MeshStreamBuilder builder;

        MeshWriteStreams(base::mesh::MeshTopologyType topology, uint32_t numFaces, base::mesh::MeshStreamMask streams)
            : builder(topology)
        {
            if (topology == base::mesh::MeshTopologyType::Triangles)
                builder.reserveVertices(numFaces * 3, streams);
            else if (topology == base::mesh::MeshTopologyType::Quads)
                builder.reserveVertices(numFaces * 4, streams);

            writePosition = (Position*)builder.vertexData<base::mesh::MeshStreamType::Position_3F>();
            writeNormal = (NormalVector*)builder.vertexData<base::mesh::MeshStreamType::Normal_3F>();
            writeBitangent = (NormalVector*)builder.vertexData<base::mesh::MeshStreamType::Binormal_3F>();
            writeTangent = (NormalVector*)builder.vertexData<base::mesh::MeshStreamType::Tangent_3F>();
            writeColor0 = builder.vertexData<base::mesh::MeshStreamType::Color0_4U8>();
            writeColor1 = builder.vertexData<base::mesh::MeshStreamType::Color1_4U8>();
            writeUV0 = builder.vertexData<base::mesh::MeshStreamType::TexCoord0_2F>();
            writeUV1 = builder.vertexData<base::mesh::MeshStreamType::TexCoord1_2F>();
            writeSkinIndices = (SkinIndices*)builder.vertexData<base::mesh::MeshStreamType::SkinningIndices_4U8>();
            writeSkinWeights = (SkinWeights*)builder.vertexData<base::mesh::MeshStreamType::SkinningWeights_4F>();
        }

        template< typename T >
        static void AdvancePointer(T*& ptr, uint32_t count)
        {
            if (ptr)
                ptr += count;
        }

        void advance(uint32_t numVertices)
        {
            AdvancePointer(writePosition, numVertices);
            AdvancePointer(writeNormal, numVertices);
            AdvancePointer(writeTangent, numVertices);
            AdvancePointer(writeBitangent, numVertices);
            AdvancePointer(writeColor0, numVertices);
            AdvancePointer(writeColor1, numVertices);
            AdvancePointer(writeUV0, numVertices);
            AdvancePointer(writeUV1, numVertices);
        }

        void extract(base::mesh::MeshModelChunk& outChunk)
        {
            const auto* writePositionStart = (Position*)builder.vertexData<base::mesh::MeshStreamType::Position_3F>();
            const auto numVeritces = writePosition - writePositionStart;

            builder.numVeritces = numVeritces;
            builder.extract(outChunk);
        }
    };

    static void ExtractVertex(const FBXMeshStreams& streams, const base::Matrix& localToWorld, uint32_t controlPoint, uint32_t vertexIndex, uint32_t uvVertexIndex, uint32_t j, MeshWriteStreams& outWriter)
    {
        ExtractStreamElement(streams.positions[controlPoint], outWriter.writePosition[j], localToWorld);
        ExtractStreamData(streams.normals, localToWorld, vertexIndex, controlPoint, outWriter.writeNormal[j]);
        ExtractStreamData(streams.tangents, localToWorld, vertexIndex, controlPoint, outWriter.writeTangent[j]);
        ExtractStreamData(streams.bitangents, localToWorld, vertexIndex, controlPoint, outWriter.writeBitangent[j]);
        ExtractStreamData(streams.color0, localToWorld, vertexIndex, controlPoint, outWriter.writeColor0[j]);
        ExtractStreamData(streams.color1, localToWorld, vertexIndex, controlPoint, outWriter.writeColor1[j]);
        ExtractStreamDataUV(streams.uv0, localToWorld, uvVertexIndex, controlPoint, outWriter.writeUV0[j], streams.flipUV);
        ExtractStreamDataUV(streams.uv1, localToWorld, uvVertexIndex, controlPoint, outWriter.writeUV1[j], streams.flipUV);

        if (streams.skinInfluences)
        {
            const auto& skin = streams.skinInfluences[vertexIndex];

            // TODO: remap bones or just switch to 16bit indices
            outWriter.writeSkinIndices[j].indices[0] = skin.m_indices[0];
            outWriter.writeSkinIndices[j].indices[1] = skin.m_indices[1];
            outWriter.writeSkinIndices[j].indices[2] = skin.m_indices[2];
            outWriter.writeSkinIndices[j].indices[3] = skin.m_indices[3];
            outWriter.writeSkinWeights[j].weights[3] = skin.m_weights[0];
            outWriter.writeSkinWeights[j].weights[1] = skin.m_weights[1];
            outWriter.writeSkinWeights[j].weights[2] = skin.m_weights[2];
            outWriter.writeSkinWeights[j].weights[3] = skin.m_weights[3];
        }
    }

    static void ExtractTriangles(const FBXMeshStreams& streams, const base::Matrix& localToWorld, uint32_t polygonIndex, uint32_t vertexIndex, MeshWriteStreams& outWriter)
    {
        const auto polygonSize = streams.mesh->GetPolygonSize(polygonIndex);

        uint32_t controlPoints[3];
        controlPoints[0] = streams.mesh->GetPolygonVertex(polygonIndex, 0);
        controlPoints[1] = streams.mesh->GetPolygonVertex(polygonIndex, 1);

        uint32_t uvControlPoints[3];
        uvControlPoints[0] = ((FbxMesh*)streams.mesh)->GetTextureUVIndex(polygonIndex, 0);
        uvControlPoints[1] = ((FbxMesh*)streams.mesh)->GetTextureUVIndex(polygonIndex, 1);
        
        uint32_t vertices[3];
        vertices[0] = vertexIndex + 0;
        vertices[1] = vertexIndex + 1;

        for (uint32_t i = 2; i < polygonSize; ++i)
        {
            // extract "lead vertex"
            controlPoints[2] = streams.mesh->GetPolygonVertex(polygonIndex, i);
            uvControlPoints[2] = ((FbxMesh*)streams.mesh)->GetTextureUVIndex(polygonIndex, i);
            vertices[2] = vertexIndex + i;

            // process triangle
            if (streams.flipFaces)
            {
                for (uint32_t j = 0; j < 3; ++j)
                    ExtractVertex(streams, localToWorld, controlPoints[2-j], vertices[2 - j], uvControlPoints[2 - j], j, outWriter);
            }
            else
            {
                for (uint32_t j = 0; j < 3; ++j)
                    ExtractVertex(streams, localToWorld, controlPoints[j], vertices[j], uvControlPoints[j], j, outWriter);
            }

            // save for next pass
            vertices[1] = vertices[2];
            controlPoints[1] = controlPoints[2];
            uvControlPoints[1] = uvControlPoints[2];

            // advance writing stream
            outWriter.advance(3);
        }
    }

    static void ExtractQuad(const FBXMeshStreams& streams, const base::Matrix& localToWorld, uint32_t polygonIndex, uint32_t vertexIndex, MeshWriteStreams& outWriter)
    {
        DEBUG_CHECK(streams.mesh->GetPolygonSize(polygonIndex) == 4);

        uint32_t controlPoints[4];
        controlPoints[0] = streams.mesh->GetPolygonVertex(polygonIndex, 0);
        controlPoints[1] = streams.mesh->GetPolygonVertex(polygonIndex, 1);
        controlPoints[2] = streams.mesh->GetPolygonVertex(polygonIndex, 2);
        controlPoints[3] = streams.mesh->GetPolygonVertex(polygonIndex, 3);

        uint32_t uvControlPoints[4];
        uvControlPoints[0] = ((FbxMesh*)streams.mesh)->GetTextureUVIndex(polygonIndex, 0);
        uvControlPoints[1] = ((FbxMesh*)streams.mesh)->GetTextureUVIndex(polygonIndex, 1);
        uvControlPoints[2] = ((FbxMesh*)streams.mesh)->GetTextureUVIndex(polygonIndex, 2);
        uvControlPoints[3] = ((FbxMesh*)streams.mesh)->GetTextureUVIndex(polygonIndex, 3);

        uint32_t vertices[4];
        vertices[0] = vertexIndex + 0;
        vertices[1] = vertexIndex + 1;
        vertices[2] = vertexIndex + 2;
        vertices[3] = vertexIndex + 3;

        if (streams.flipFaces)
        {
            for (uint32_t j = 0; j < 4; ++j)
                ExtractVertex(streams, localToWorld, controlPoints[3-j], vertices[3 - j], uvControlPoints[3 - j], j, outWriter);
        }
        else
        {
            for (uint32_t j = 0; j < 4; ++j)
                ExtractVertex(streams, localToWorld, controlPoints[j], vertices[j], uvControlPoints[j], j, outWriter);
        }

        outWriter.advance(4);
    }

    void DataNode::extractSkinInfluences(const LoadedFile& owner, base::Array<MeshVertexInfluence>& outInfluences, SkeletonBuilder& outSkeleton, bool forceSkinToNode) const
    {
        // extract skinning
        const auto numVertices = m_mesh->GetControlPointsCount();
        if (forceSkinToNode)
        {
            TRACE_INFO("FBX: Binding all vertices to node");

            // prepare influence table
            outInfluences.resize(numVertices);
            memzero(outInfluences.data(), outInfluences.dataSize());

            // map the single bone (node)
            auto boneIndex = outSkeleton.addBone(owner, this);
            for (uint32_t i = 0; i < numVertices; ++i)
                outInfluences[i].add(boneIndex, 1.0f);
        }
        else if (m_mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
        {
            const FbxSkin* pSkin = pSkin = (const FbxSkin*)m_mesh->GetDeformer(0, FbxDeformer::eSkin);
            TRACE_INFO("FBX: Found skinning ({} clusters)", pSkin->GetClusterCount());

            // prepare influence table
            outInfluences.resize(numVertices);
            memzero(outInfluences.data(), outInfluences.dataSize());

            // load influences
            uint32_t numTotalInfluences = 0;
            uint32_t numClusters = pSkin->GetClusterCount();
            for (uint32_t i = 0; i < numClusters; ++i)
            {
                const FbxCluster* pCluster = pSkin->GetCluster(i);

                const FbxNode* pLinkNode = pCluster->GetLink();
                if (!pLinkNode)
                    continue;

                FbxAMatrix linkMatrix;
                pCluster->GetTransformLinkMatrix(linkMatrix);

                uint32_t globalBoneIndex = outSkeleton.addBone(owner, pLinkNode);//, linkMatrix);
                //LOG(TXT("Cluster %d link to '%s' mapped as %d"), i, ANSI_TO_UNICODE(pLinkNode->GetName()), globalBoneIndex);

                uint32_t numInfluences = pCluster->GetControlPointIndicesCount();
                const int* pIndices = pCluster->GetControlPointIndices();
                const double* pWeights = pCluster->GetControlPointWeights();
                for (uint32_t j = 0; j < numInfluences; ++j)
                {
                    uint32_t pointIndex = pIndices[j];
                    float weight = (float)pWeights[j];
                    outInfluences[pointIndex].add(globalBoneIndex, weight);
                    numTotalInfluences += 1;
                }
            }

            TRACE_INFO("FBX: Loaded {} total skin influences", numTotalInfluences);
        }
    }

    void DataNode::extractBuildChunks(const LoadedFile& owner, base::Array<ChunkInfo>& outBuildChunks, MaterialMapper& outMaterials) const
    {
        // get number of vertices
        uint32_t numVertices = m_mesh->GetControlPointsCount();
        const FbxVector4* pControlPoints = m_mesh->GetControlPoints();
        TRACE_INFO("FBX: Found {} control points (vertices) in {}", numVertices, m_name);

        // get number of polygons
        uint32_t numPolygons = m_mesh->GetPolygonCount();
        TRACE_INFO("FBX: Found {} polygons in {}", numPolygons, m_name);

        // check if all materials on mesh are the same (faster loading)
        bool bIsAllSame = true;
        if (m_mesh->GetElementMaterialCount() > 0)
        {
            const FbxGeometryElementMaterial* pMaterialElement = m_mesh->GetElementMaterial(0);
            if (pMaterialElement->GetMappingMode() == FbxGeometryElement::eByPolygon)
            {
                TRACE_INFO("FBX: Mesh {} has per-polygon material mapping", m_name);
                bIsAllSame = false;
            }
        }

        // check if the same material is used for the whole mesh
        uint32_t meshMaterialID = 0;
        if (bIsAllSame)
        {
            const FbxGeometryElementMaterial* lMaterialElement = m_mesh->GetElementMaterial(0);
            if (lMaterialElement && lMaterialElement->GetMappingMode() == FbxGeometryElement::eAllSame)
            {
                FbxSurfaceMaterial* lMaterial = m_mesh->GetNode()->GetMaterial(lMaterialElement->GetIndexArray().GetAt(0));
                meshMaterialID = lMaterialElement->GetIndexArray().GetAt(0);
                TRACE_INFO("FBX: Mesh uses material {} for all it's content", meshMaterialID);
            }
        }


        // gather information about polygons in each chunk
        base::InplaceArray<int, 64> localMaterialMapping;
        base::InplaceArray<ChunkInfo, 64> materialChunksInfo;
        const auto numMaterials = m_mesh->GetNode()->GetMaterialCount() + 1;
        {
            materialChunksInfo.resize(numMaterials);
            localMaterialMapping.resizeWith(numMaterials, -1);

            // reserve some space for polygon indices
            for (auto& chunk : materialChunksInfo)
            {
                chunk.polygonIndices.reserve(2 * (numPolygons / numMaterials));
                chunk.firstVertexIndices.reserve(2 * (numPolygons / numMaterials));
            }

            // gather polygons into buckets
            uint32_t vertexIndex = 0;
            for (uint32_t i = 0; i < numPolygons; i++)
            {
                // face vertex count
                const auto polygonSize = m_mesh->GetPolygonSize(i);

                // get material used on the face
                uint32_t polygonMaterialID = meshMaterialID;
                if (!bIsAllSame)
                {
                    const FbxGeometryElementMaterial* pMaterialElement = m_mesh->GetElementMaterial(0);
                    polygonMaterialID = pMaterialElement->GetIndexArray().GetAt(i);
                    if (polygonMaterialID < 0 || polygonMaterialID >= materialChunksInfo.lastValidIndex())
                        polygonMaterialID = 0;
                }

                // map polygon material
                auto realPolygonMaterialId = localMaterialMapping[polygonMaterialID];
                if (realPolygonMaterialId == -1)
                {
                    auto lMaterial = m_mesh->GetNode()->GetMaterial(polygonMaterialID);
                    auto materialName = lMaterial ? lMaterial->GetName() : m_name.c_str();
                    realPolygonMaterialId = outMaterials.addMaterial(materialName, lMaterial);
                    localMaterialMapping[polygonMaterialID] = realPolygonMaterialId;
                }

                // count number of needed triangles
                // TODO: add quads...
                if (polygonSize == 4)
                {
                    materialChunksInfo[polygonMaterialID].numQuads += 1;
                }
                else
                {
                    const auto numTriangles = polygonSize - 2;
                    materialChunksInfo[polygonMaterialID].numTriangles += numTriangles;
                }

                // add to the chunk
                materialChunksInfo[polygonMaterialID].polygonIndices.pushBack(i);
                materialChunksInfo[polygonMaterialID].firstVertexIndices.pushBack(vertexIndex);
                vertexIndex += polygonSize;
            }
        }

        // finalize material mapping
        for (uint32_t i = 0; i < materialChunksInfo.size(); ++i)
        {
            auto& info = materialChunksInfo[i];

            // if we don't have ALL QUADS than fall back to triangles
            if (info.numTriangles != 0)
            {
                info.numTriangles += info.numQuads * 2;
                info.numQuads = 0;
            }

            if (info.numTriangles || info.numQuads)
            {
                auto& outChunk = outBuildChunks.emplaceBack();
                outChunk.numTriangles = info.numTriangles;
                outChunk.numQuads = info.numQuads;
                outChunk.materialIndex = localMaterialMapping[i]; // get global material index for local material index
                outChunk.polygonIndices = std::move(info.polygonIndices);
                outChunk.firstVertexIndices = std::move(info.firstVertexIndices);
            }
        }
    }

    static void ExtractChunkFaces(const FBXMeshStreams& source, const DataNode::ChunkInfo& buildChunk, const base::Matrix& localToWorld, MeshWriteStreams& outWriter)
    {
        DEBUG_CHECK(buildChunk.polygonIndices.size() == buildChunk.firstVertexIndices.size());

        const auto numFaces = buildChunk.polygonIndices.size();

        if (buildChunk.topology() == base::mesh::MeshTopologyType::Triangles)
        {
            for (uint32_t i = 0; i < numFaces; ++i)
            {
                const auto polygonIndex = buildChunk.polygonIndices.typedData()[i];
                const auto vertexIndex = buildChunk.firstVertexIndices.typedData()[i];
                ExtractTriangles(source, localToWorld, polygonIndex, vertexIndex, outWriter);
            }
        }
        else
        {
            for (uint32_t i = 0; i < numFaces; ++i)
            {
                const auto polygonIndex = buildChunk.polygonIndices.typedData()[i];
                const auto vertexIndex = buildChunk.firstVertexIndices.typedData()[i];
                ExtractQuad(source, localToWorld, polygonIndex, vertexIndex, outWriter);
            }
        }
    }

    bool DataNode::exportToMeshModel(base::IProgressTracker& progress, const LoadedFile& owner, const base::Matrix& worldToEngine, DataNodeMesh& outGeonetry, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials, bool forceSkinToNode, bool flipUV, bool flipFace) const
    {
        // no mesh data
        if (!m_mesh)
            return false;

        // flip faces ?
        auto meshToEngine = m_meshToWorld * worldToEngine;

        // prepare skin tables
        base::Array<MeshVertexInfluence> skinInfluences;
        extractSkinInfluences(owner, skinInfluences, outSkeleton, forceSkinToNode);

        // extract build chunk
        base::Array<ChunkInfo> buildChunks;
        extractBuildChunks(owner, buildChunks, outMaterials);

        // source stream data
        FBXMeshStreams sourceStreams(m_mesh, skinInfluences);
        sourceStreams.flipUV = flipUV;
        sourceStreams.flipFaces = (meshToEngine.det3() < 0.0f) ^ flipFace;

        // process chunk data
        // TODO: run on fibers
        for (const auto& buildChunk : buildChunks)
        {
            MeshWriteStreams writer(buildChunk.topology(), buildChunk.faceCount(), sourceStreams.streamMask);

            // break
            if (progress.checkCancelation())
                return false;

            // export data
            ExtractChunkFaces(sourceStreams, buildChunk, meshToEngine, writer);

            // finalize chunk
            {
                auto& exportChunk = outGeonetry.chunks.emplaceBack();
                exportChunk.materialIndex = buildChunk.materialIndex;
                exportChunk.detailMask = 1U << m_lodIndex;
                writer.extract(exportChunk);
                TRACE_INFO("FBX: Extracted {} faces from mesh {}, material {}", exportChunk.numFaces, m_mesh->GetName(), exportChunk.materialIndex);
            }
        }

        // done
        return true;
    }

    //---

    RTTI_BEGIN_TYPE_CLASS(LoadedFile);
    RTTI_END_TYPE();

    LoadedFile::LoadedFile()
    {
        m_nodes.reserve(256);
        m_nodeMap.reserve(256);
    }

	bool GFBXSceneClosed = false;

    LoadedFile::~LoadedFile()
    {
        if (m_fbxScene)
        {
			if (!GFBXSceneClosed)
				m_fbxScene->Destroy();
            m_fbxScene = nullptr;
        }
    }

    const DataNode* LoadedFile::findDataNode(const fbxsdk::FbxNode* fbxNode) const
    {
        const DataNode* ret = nullptr;
        m_nodeMap.find(fbxNode, ret);
        return ret;
    }

    template<typename T>
    INLINE static T InitialValue(const FbxPropertyT<T>& prop)
    {
        FbxTime t = 0;
        if (prop.IsAnimated())
        {
            auto evaluator = prop.GetAnimationEvaluator();
            if (evaluator)
                t = evaluator->ValidateTime(t);
        }

        return const_cast<FbxPropertyT<T>&>(prop).EvaluateValue(t); // TODO: fix once FBX SDK comes to it's senses
    }

    void LoadedFile::walkStructure(const fbxsdk::FbxAMatrix& fbxParentToWorld, const base::Matrix& spaceConversionMatrix, const fbxsdk::FbxNode* node, DataNode* parentDataNode)
    {
        // get the local pose of the node
        FbxAMatrix fbxLocalToParent;
        {
            const auto rotationOrder = InitialValue(node->RotationOrder);

            const auto lT = InitialValue(node->LclTranslation);
            const auto lR = InitialValue(node->LclRotation);
            const auto lS = InitialValue(node->LclScaling);
            TRACE_INFO("LocalTranslation for '{}': [{},{},{}]", node->GetName(), lT[0], lT[1], lT[2]);
            TRACE_INFO("LocalRotation for '{}': [{},{},{}]", node->GetName(), lR[0], lR[1], lR[2]);
            TRACE_INFO("LocalScale for '{}': [{},{},{}]", node->GetName(), lS[0], lS[1], lS[2]);
            fbxLocalToParent.SetT(lT);
            fbxLocalToParent.SetR(lR);
            fbxLocalToParent.SetS(lS);
        }

        // get the local to world matrix of the node
        const auto fbxLocalToWorld = fbxParentToWorld * fbxLocalToParent; // NOTE: inverted order

        // get the additional transform, just for the mesh
        FbxAMatrix fbxMeshToLocal;
        fbxMeshToLocal.SetIdentity();
        if (node->GetNodeAttribute())
        {
            const auto lT = node->GetGeometricTranslation(FbxNode::eSourcePivot);
            const auto lR = node->GetGeometricRotation(FbxNode::eSourcePivot);
            const auto lS = node->GetGeometricScaling(FbxNode::eSourcePivot);
            TRACE_INFO("MeshTranslation for '{}': [{},{},{}]", node->GetName(), lT[0], lT[1], lT[2]);
            TRACE_INFO("MeshRotation for '{}': [{},{},{}]", node->GetName(), lR[0], lR[1], lR[2]);
            TRACE_INFO("MeshScale for '{}': [{},{},{}]", node->GetName(), lS[0], lS[1], lS[2]);
            fbxMeshToLocal.SetT(lT);
            fbxMeshToLocal.SetR(lR);
            fbxMeshToLocal.SetS(lS);
        }

        // get our final matrices
        const auto localToWorld = ToMatrix(fbxLocalToWorld) * spaceConversionMatrix;
        const auto meshToWorld = ToMatrix(fbxLocalToWorld * fbxMeshToLocal) * spaceConversionMatrix;

        // get the local to parent matrix from FBX node
        // NOTE: this code is still shit...
        TRACE_INFO("LocalToWorld for '{}': [{},{},{},{}] [{},{},{},{}] [{},{},{},{}]", node->GetName(),
            localToWorld.m[0][0], localToWorld.m[0][1], localToWorld.m[0][2], localToWorld.m[0][3],
            localToWorld.m[1][0], localToWorld.m[1][1], localToWorld.m[1][2], localToWorld.m[1][3],
            localToWorld.m[2][0], localToWorld.m[2][1], localToWorld.m[2][2], localToWorld.m[2][3]);

        // create the local node
        auto localNode  = MemNew(DataNode);
        localNode->m_localToWorld = localToWorld;
        localNode->m_meshToWorld = meshToWorld;
        //localNode->m_localToParent = localNode->m_localToWorld * worldToParent;
        localNode->m_name = base::StringView(node->GetName());
        localNode->m_parent = parentDataNode;
        if (parentDataNode)
            parentDataNode->m_children.pushBack(localNode);
        m_nodes.pushBack(localNode);

        // map
        localNode->m_node = node;
        m_nodeMap[node] = localNode;

        // extract mesh at this node
        if (node->GetNodeAttribute() != NULL)
        {
            if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
            {
                const FbxMesh *mesh = (const FbxMesh *) node->GetNodeAttribute();
                localNode->m_mesh = mesh;

                FbxAMatrix pivotMatrix;
                const auto& pivot = mesh->GetPivot(pivotMatrix);
                TRACE_INFO("Pivot for '{}': [{},{},{}] [{},{},{},{}] [{},{},{}]", node->GetName(),
                    pivotMatrix.GetT()[0], pivotMatrix.GetT()[1], pivotMatrix.GetT()[2],
                    pivotMatrix.GetR()[0], pivotMatrix.GetR()[1], pivotMatrix.GetR()[2], pivotMatrix.GetR()[3],
                    pivotMatrix.GetS()[0], pivotMatrix.GetS()[1], pivotMatrix.GetS()[2]);

                // remove pivot bullshit
                //((FbxMesh *) mesh)->ApplyPivot();

                // skip
                if (localNode->m_name.view().endsWithNoCase("_tri"))
                {
                    TRACE_INFO("Found triangle collision mesh data on node '{}'", localNode->m_name);
                    localNode->m_type = DataNodeType::TriangleCollision;
                }
                else if (localNode->m_name.view().endsWithNoCase("_convex") 
                    || localNode->m_name.view().endsWithNoCase("_col") 
                    || localNode->m_name.view().beginsWithNoCase("ucx_"))
                {
                    TRACE_INFO("Found convex collision mesh data on node '{}'", localNode->m_name);
                    localNode->m_type = DataNodeType::ConvexCollision;
                }
                else
                {
                    localNode->m_type = DataNodeType::VisualMesh;

                    if (localNode->m_name.view().endsWithNoCase("_lod0"))
                        localNode->m_lodIndex = 0;
                    else if (localNode->m_name.view().endsWithNoCase("_lod1"))
                        localNode->m_lodIndex = 1;
                    else if (localNode->m_name.view().endsWithNoCase("_lod2"))
                        localNode->m_lodIndex = 2;
                    else if (localNode->m_name.view().endsWithNoCase("_lod3"))
                        localNode->m_lodIndex = 3;
                    else if (localNode->m_name.view().endsWithNoCase("_lod4"))
                        localNode->m_lodIndex = 4;
                    else if (localNode->m_name.view().endsWithNoCase("_lod5"))
                        localNode->m_lodIndex = 5;
                    else if (localNode->m_name.view().endsWithNoCase("_lod6"))
                        localNode->m_lodIndex = 6;
                    else if (localNode->m_name.view().endsWithNoCase("_lod7"))
                        localNode->m_lodIndex = 7;


                    TRACE_INFO("Found visual mesh (LOD{}) on node '{}'", localNode->m_lodIndex, localNode->m_name);
                }
            }
        }

        // visit children
        uint32_t numChildren = node->GetChildCount();
        if (numChildren)
        {
            auto worldToParent = localNode->m_localToWorld.inverted();
            for (uint32_t i = 0; i < numChildren; ++i)
            {
                if (const FbxNode* childNode = node->GetChild(i))
                    walkStructure(fbxLocalToWorld, spaceConversionMatrix, childNode, localNode);
            }
        }
    }    

    //---

    uint64_t LoadedFile::calcMemoryUsage() const
    {
        return m_fbxDataSize;
    }

    bool LoadedFile::loadFromMemory(Buffer data)
    {
        if (!data)
            return false;

        base::Matrix spaceConversionMatrix;
        auto scene = GetService<FileLoadingService>()->loadScene(data, spaceConversionMatrix);
        if (!scene)
            return false;

        m_fbxDataSize = data.size();
        m_fbxScene = scene;

        FbxAMatrix rootMatrix;
        rootMatrix.SetIdentity();

        auto rootNode = m_fbxScene->GetRootNode();
        walkStructure(rootMatrix, spaceConversionMatrix, rootNode, nullptr);

        return true;
    }

    //---

} // fbx
