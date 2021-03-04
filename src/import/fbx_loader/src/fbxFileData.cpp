/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#include "build.h"
#include "fbxFileData.h"

#include "core/io/include/fileHandle.h"
#include "core/app/include/localServiceContainer.h"
#include "core/containers/include/inplaceArray.h"
#include "core/resource/include/resource.h"
#include "engine/mesh/include/streamData.h"
#include "engine/mesh/include/streamBuilder.h"
#include "engine/mesh/include/streamIterator.h"
#include "core/resource/include/tags.h"
#include "../../mesh_loader/include/renderingMeshImportConfig.h"

#pragma optimize("", off)

#undef TRACE_INFO
#define TRACE_INFO TRACE_WARNING

BEGIN_BOOMER_NAMESPACE_EX(assets)

//---

MaterialMapper::MaterialMapper()
{
    materials.reserve(256);
    materialsMapping.reserve(256);
}

uint32_t MaterialMapper::addMaterial(const char* materialName)
{
    if (!materialName || !*materialName)
        materialName = "default";

    uint32_t ret = 0;
    if (materialsMapping.find(StringView(materialName), ret))
        return ret;

    const auto materialNameString = StringBuf(materialName);
    materials.emplaceBack(materialNameString);        
    materialsMapping[materialNameString] = materials.lastValidIndex();

    return materials.lastValidIndex();
}

//---

SkeletonBuilder::SkeletonBuilder()
{
    m_skeletonBones.reserve(256);
    m_skeletonBonesMapping.reserve(256);
    m_skeletonRawBonesMapping.reserve(256);
}

uint32_t SkeletonBuilder::addBone(const FBXFile& owner, const DataNode* node)
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

uint32_t SkeletonBuilder::addBone(const FBXFile& owner, const ofbx::Object* node)
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
    Vector3 vector;
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
    Vector3 vector;
};

static void ExtractStreamElement(const ofbx::Vec3& source, Position& writeTo, const Matrix& localToWorld)
{
    writeTo.vector.x = source.x;
    writeTo.vector.y = source.y;
    writeTo.vector.z = source.z;
    writeTo.vector = localToWorld.transformPoint(writeTo.vector);
}

static void ExtractStreamElement(const ofbx::Vec3& source, NormalVector& writeTo, const Matrix& localToWorld)
{
    writeTo.vector.x = source.x;
    writeTo.vector.y = source.y;
    writeTo.vector.z = source.z;
    writeTo.vector = localToWorld.transformVector(writeTo.vector);
    writeTo.vector.normalize();
}

static void ExtractStreamElement(const ofbx::Vec4& source, Position& writeTo, const Matrix& localToWorld)
{
    writeTo.vector.x = source.x;
    writeTo.vector.y = source.y;
    writeTo.vector.z = source.z;
    writeTo.vector = localToWorld.transformPoint(writeTo.vector);
}

static void ExtractStreamElement(const ofbx::Vec4& source, NormalVector& writeTo, const Matrix& localToWorld)
{
    writeTo.vector.x = source.x;
    writeTo.vector.y = source.y;
    writeTo.vector.z = source.z;
    writeTo.vector = localToWorld.transformVector(writeTo.vector);
    writeTo.vector.normalize();
}

static void ExtractStreamElement(const ofbx::Vec4& source, Vector3& writeTo, const Matrix& localToWorld)
{
    writeTo.x = source.x;
    writeTo.y = source.y;
    writeTo.z = source.z;
}

static void ExtractStreamElement(const ofbx::Vec3& source, Vector3& writeTo, const Matrix& localToWorld)
{
    writeTo.x = source.x;
    writeTo.y = source.y;
    writeTo.z = source.z;
}

static void ExtractStreamElement(const ofbx::Vec2& source, Vector2& writeTo, const Matrix& localToWorld)
{
    writeTo.x = source.x;
    writeTo.y = source.y;
}

static void ExtractStreamElement(const ofbx::Vec4& source, Color& writeTo, const Matrix& localToWorld)
{
    writeTo.r = FloatTo255(source.x);
    writeTo.g = FloatTo255(source.y);
    writeTo.b = FloatTo255(source.z);
    writeTo.a = FloatTo255(source.w);
}

/*static void ExtractStreamElement(const FbxVector4& source, Vector3& writeTo)
{
}*/

template< typename T, typename DataType >
static void ExtractStreamData(const T* stream, const Matrix& localToWorld, uint32_t vertexIndex, DataType& writeTo)
{
    if (stream)
        ExtractStreamElement(stream[vertexIndex], writeTo, localToWorld);
}

template< typename T, typename DataType >
static void ExtractStreamDataUV(const T* stream, const Matrix& localToWorld, uint32_t vertexIndex, DataType& writeTo, bool flipUV)
{
    if (stream)
    {
        ExtractStreamElement(stream[vertexIndex], writeTo, localToWorld);

        if (flipUV)
            writeTo.y = 1.0f - writeTo.y;
    }
}

struct FBXMeshStreams : public NoCopy
{
    const ofbx::Vec3* positions = nullptr;
    const ofbx::Vec3* normals = nullptr;
    const ofbx::Vec3* bitangents = nullptr;
    const ofbx::Vec3* tangents = nullptr;
    const ofbx::Vec4* color0 = nullptr;
    const ofbx::Vec4* color1 = nullptr;
    const ofbx::Vec2* uv0 = nullptr;
    const ofbx::Vec2* uv1 = nullptr;

    const MeshVertexInfluence* skinInfluences = nullptr;

    MeshStreamMask streamMask = 0;

    bool flipUV = true;

    const ofbx::Geometry* geometry = nullptr;

    FBXMeshStreams(const ofbx::Geometry* geom_, const Array<MeshVertexInfluence>& skinningData)
        : geometry(geom_)
    {
        // get input data streams
        positions = geometry->getVertices();
        normals = geometry->getNormals();
        //tangents = nullptr;
        //bitangents = nullptr;
        color0 = geometry->getColors();
        uv0 = geometry->getUVs(0);
        uv1 = geometry->getUVs(1);

        // determine stream mask for all chunks in this model
        streamMask = MeshStreamMaskFromType(MeshStreamType::Position_3F);
        if (positions != nullptr)
            streamMask |= MeshStreamMaskFromType(MeshStreamType::Normal_3F);
        if (bitangents != nullptr)
            streamMask |= MeshStreamMaskFromType(MeshStreamType::Binormal_3F);
        if (tangents != nullptr)
            streamMask |= MeshStreamMaskFromType(MeshStreamType::Tangent_3F);
        if (color0 != nullptr)
            streamMask |= MeshStreamMaskFromType(MeshStreamType::Color0_4U8);
        if (color1 != nullptr)
            streamMask |= MeshStreamMaskFromType(MeshStreamType::Color1_4U8);
        if (uv0 != nullptr)
            streamMask |= MeshStreamMaskFromType(MeshStreamType::TexCoord0_2F);
        if (uv1 != nullptr)
            streamMask |= MeshStreamMaskFromType(MeshStreamType::TexCoord1_2F);

        // skinning data
        if (!skinningData.empty())
        {
            skinInfluences = skinningData.typedData();
            streamMask |= MeshStreamMaskFromType(MeshStreamType::SkinningIndices_4U8);
            streamMask |= MeshStreamMaskFromType(MeshStreamType::SkinningWeights_4F);

            // TODO: 8 bone skinning
        }
    }
};

struct MeshWriteStreams
{
    uint32_t numVerticesPerFace = 0;;

    Position* writePosition = nullptr;
    NormalVector* writeNormal = nullptr;
    NormalVector* writeTangent = nullptr;
    NormalVector* writeBitangent = nullptr;
    Vector2* writeUV0 = nullptr;
    Vector2* writeUV1 = nullptr;
    Color* writeColor0 = nullptr;
    Color* writeColor1 = nullptr;
    SkinIndices* writeSkinIndices = nullptr;
    SkinWeights* writeSkinWeights = nullptr;

    MeshRawStreamBuilder builder;

    MeshWriteStreams(MeshTopologyType topology, uint32_t numFaces, MeshStreamMask streams)
        : builder(topology)
    {
        if (topology == MeshTopologyType::Triangles)
            numVerticesPerFace = 3;
        else if (topology == MeshTopologyType::Quads)
            numVerticesPerFace = 4;

        builder.reserveVertices(numFaces * numVerticesPerFace, streams);

        writePosition = (Position*)builder.vertexData<MeshStreamType::Position_3F>();
        writeNormal = (NormalVector*)builder.vertexData<MeshStreamType::Normal_3F>();
        writeBitangent = (NormalVector*)builder.vertexData<MeshStreamType::Binormal_3F>();
        writeTangent = (NormalVector*)builder.vertexData<MeshStreamType::Tangent_3F>();
        writeColor0 = builder.vertexData<MeshStreamType::Color0_4U8>();
        writeColor1 = builder.vertexData<MeshStreamType::Color1_4U8>();
        writeUV0 = builder.vertexData<MeshStreamType::TexCoord0_2F>();
        writeUV1 = builder.vertexData<MeshStreamType::TexCoord1_2F>();
        writeSkinIndices = (SkinIndices*)builder.vertexData<MeshStreamType::SkinningIndices_4U8>();
        writeSkinWeights = (SkinWeights*)builder.vertexData<MeshStreamType::SkinningWeights_4F>();
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

    void extract(MeshRawChunk& outChunk)
    {
        const auto* writePositionStart = (Position*)builder.vertexData<MeshStreamType::Position_3F>();
        const auto numVeritces = writePosition - writePositionStart;

        builder.numVeritces = numVeritces;
        builder.extract(outChunk);
    }
};

struct GeometryFace
{
    //uint32_t material = 0;
    mutable const GeometryFace* next = nullptr;

    uint32_t vertices[4]; // up to 4, can be 3 if triangle was exported
    uint32_t indices[4]; // original indices
};

struct GeometryFaceList
{
    int globalMaterialIndex = INDEX_NONE;

    const GeometryFace* firstFace = nullptr;
    const GeometryFace* lastFace = nullptr;
    uint32_t count = 0;
};

static void ExtractFace(const FBXMeshStreams& streams, const Matrix& localToWorld, const GeometryFace& face, MeshWriteStreams& outWriter, bool normalOrder)
{
    auto ri = outWriter.numVerticesPerFace - 1;
    for (uint32_t i = 0; i < outWriter.numVerticesPerFace; ++i, --ri)
    {
        const auto vi = face.vertices[normalOrder ? i : ri];
        const auto di = face.indices[normalOrder ? i : ri];

        ExtractStreamData(streams.positions, localToWorld, vi, outWriter.writePosition[i]);
        ExtractStreamData(streams.normals, localToWorld, di, outWriter.writeNormal[i]);
        ExtractStreamData(streams.tangents, localToWorld, di, outWriter.writeTangent[i]);
        ExtractStreamData(streams.bitangents, localToWorld, di, outWriter.writeBitangent[i]);
        ExtractStreamData(streams.color0, localToWorld, di, outWriter.writeColor0[i]);
        ExtractStreamData(streams.color1, localToWorld, di, outWriter.writeColor1[i]);
        ExtractStreamDataUV(streams.uv0, localToWorld, di, outWriter.writeUV0[i], streams.flipUV);
        ExtractStreamDataUV(streams.uv1, localToWorld, di, outWriter.writeUV1[i], streams.flipUV);

        if (streams.skinInfluences)
        {
            const auto& skin = streams.skinInfluences[vi];
            for (auto j = 0; j < 4; ++j)
            {
                outWriter.writeSkinIndices[i].indices[j] = skin.m_indices[j];
                outWriter.writeSkinWeights[i].weights[j] = skin.m_weights[j];
            }
        }
    }

    outWriter.advance(outWriter.numVerticesPerFace);
}

static void ExtractFaces(const FBXMeshStreams& streams, const Matrix& localToWorld, const GeometryFace* facePtr, MeshWriteStreams& outWriter, bool normalOrder)
{
    while (facePtr)
    {
        ExtractFace(streams, localToWorld, *facePtr, outWriter, normalOrder);
        facePtr = facePtr->next;
    }
}

static void ExtractSkinInfluences(const ofbx::Geometry* geom, Array<MeshVertexInfluence>& outInfluences, SkeletonBuilder& outSkeleton, bool forceSkinToNode)
{
#if 0
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
#endif
}

static int DecodeIndex(int idx)
{
    return (idx < 0) ? (-idx - 1) : idx;
}

static void DetermineTopology(const ofbx::Geometry* geometry, MeshTopologyType& outTopology, bool& outHasToTriangulate, uint32_t& outNumFaces, uint32_t& outNumMaterials)
{
    const auto* indexPtr = geometry->getFaceIndices();
    const auto* indexEndPtr = indexPtr + geometry->getIndexCount();
    const auto* materialIndicesPerTriangle = geometry->getMaterials();

    bool hasTriangles = false;
    bool hasQuads = false;
    bool hasNgons = false;

    uint32_t numQuads = 0;
    uint32_t numTriangles = 0;

    uint32_t currentVertexCount = 0;
    while (indexPtr < indexEndPtr)
    {
        currentVertexCount += 1;

        const auto index = *indexPtr++;
        if (index < 0)
        {
            const auto materialIndex = materialIndicesPerTriangle ? materialIndicesPerTriangle[numTriangles] : 0;
            outNumMaterials = std::max<uint32_t>(outNumMaterials, materialIndex + 1);

            if (currentVertexCount == 3)
            {
                numTriangles += 1;
                hasTriangles = true;
            }
            else if (currentVertexCount == 4)
            {
                numQuads += 1;
                numTriangles += 2;
                hasQuads = true;
            }
            else
            {
                numTriangles += (currentVertexCount - 2);
                hasNgons = true;
            }

            currentVertexCount = 0;
        }
    }

    DEBUG_CHECK_EX(currentVertexCount == 0, "Not finished on right index");

    if (hasNgons)
    {
        outHasToTriangulate = true;
        outTopology = MeshTopologyType::Triangles;
        outNumFaces = numTriangles;
    }
    else if (hasQuads && !hasTriangles)
    {
        outHasToTriangulate = false;
        outTopology = MeshTopologyType::Quads;
        outNumFaces = numQuads;
    }
    else
    {
        outHasToTriangulate = hasQuads;
        outTopology = MeshTopologyType::Triangles;
        outNumFaces = numTriangles;
    }
}

static void AddFaceToMaterialList(GeometryFace& face, GeometryFaceList& materialFaceList)
{
    materialFaceList.count += 1;

    if (!materialFaceList.firstFace)
    {
        materialFaceList.firstFace = &face;
        materialFaceList.lastFace = &face;
    }
    else
    {
        materialFaceList.lastFace->next = &face;
        materialFaceList.lastFace = &face;
    }
}

static void ExtractRawFaces(const ofbx::Geometry* geometry, Array<GeometryFace>& outFaces, bool triangulate, GeometryFaceList* materialFaceList)
{
    const auto* vertexIndexStartPtr = geometry->getFaceIndices();
    const auto* vertexIndexPtr = vertexIndexStartPtr;
    const auto* vertexIndexEndPtr = vertexIndexStartPtr + geometry->getIndexCount();
    const auto* materialIndicesPerTriangle = geometry->getMaterials();

    InplaceArray<uint32_t, 64> localIndices;
    InplaceArray<uint32_t, 64> localVertices;

    uint32_t faceIndex = 0;
    uint32_t triangleIndex = 0;
    while (vertexIndexPtr < vertexIndexEndPtr)
    {
        const auto index = vertexIndexPtr - vertexIndexStartPtr;
        const auto vertexIndex = *vertexIndexPtr++;
        const auto decodedVertexIndex = DecodeIndex(vertexIndex);

        localIndices.pushBack(index);
        localVertices.pushBack(decodedVertexIndex);

        if (vertexIndex < 0)
        {
            const auto materialIndex = materialIndicesPerTriangle ? materialIndicesPerTriangle[triangleIndex] : 0;

            if (triangulate)
            {
                for (uint32_t i = 2; i < localVertices.size(); ++i)
                {
                    auto& outFace = outFaces.emplaceBack();
                    outFace.indices[0] = localIndices[0];
                    outFace.vertices[0] = localVertices[0];
                    outFace.indices[1] = localIndices[i - 1];
                    outFace.vertices[1] = localVertices[i - 1];
                    outFace.indices[2] = localIndices[i];
                    outFace.vertices[2] = localVertices[i];

                    AddFaceToMaterialList(outFace, materialFaceList[materialIndex]);
                    triangleIndex += 1;
                }
            }
            else
            {
                ASSERT(localIndices.size() <= 4);

                auto& outFace = outFaces.emplaceBack();

                for (uint32_t i = 0; i < localVertices.size(); ++i)
                {
                    outFace.indices[i] = localIndices[i];
                    outFace.vertices[i] = localVertices[i];
                }

                AddFaceToMaterialList(outFace, materialFaceList[materialIndex]);
                triangleIndex += (localVertices.size() - 2);
            }

            localIndices.reset();
            localVertices.reset();
        }
    }
}

void DataNode::exportToMeshModel(IProgressTracker& progress, const FBXFile& owner, const Matrix& localToAsset, const DataMeshExportSetup& config, DataNodeMesh& outGeometry, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials) const
{
    // get mesh and geometry
    if (m_node->getType() != ofbx::Object::Type::MESH)
        return;

    const auto* mesh = static_cast<const ofbx::Mesh*>(m_node);
    const auto* geom = mesh->getGeometry();
    if (!geom)
        return;

    // assemble the final mesh -> world matrix
    const auto geometryToLocal = ToMatrix(mesh->getGeometricMatrix());
    const auto geometryToEngine = geometryToLocal * localToAsset * config.assetToEngine;
    TRACE_INFO("GeometryToEngine for '{}': {}", m_name, geometryToEngine);

    // determine if mesh is build from triangles, quads or a mix
    uint32_t numFaces = 0;
    uint32_t numMaterials = 0;
    uint32_t numGeometryMaterials = mesh->getMaterialCount();
    auto topology = MeshTopologyType::Triangles;
    bool hasToTriangulate = false;
    DetermineTopology(geom, topology, hasToTriangulate, numFaces, numMaterials);
    TRACE_INFO("Geometry '{}': found {} faces, {} materials ({} at geometry), topology {}, {}", 
        m_name, numFaces, numMaterials, numGeometryMaterials,
        topology, hasToTriangulate ? "has to triangulate" : "no triangulation");
    
    // prepare skin tables
    Array<MeshVertexInfluence> skinInfluences;
    if (const auto* skin = geom->getSkin())
    {
        ExtractSkinInfluences(geom, skinInfluences, outSkeleton, config.forceSkinToNode);
        TRACE_INFO("Geometry '{}': found {} skin influences", m_name, skinInfluences.size());
    }

    // group faces per material
    Array<GeometryFace> faces;
    Array<GeometryFaceList> facesPerMaterial;
    {
        facesPerMaterial.resize(numMaterials);
        faces.reserve(numFaces);
        ExtractRawFaces(geom, faces, hasToTriangulate, facesPerMaterial.typedData());
        for (uint32_t i=0; i<numMaterials; ++i)
        {
            auto& faces = facesPerMaterial[i];
            if (faces.count)
            {
                if (i < numGeometryMaterials)
                {
                    const auto* globalMaterial = mesh->getMaterial(i);
                    if (globalMaterial)
                    {
                        TRACE_INFO("Geometry '{}': found material '{}' at slot {}", m_name, StringView(globalMaterial->name), i);
                        faces.globalMaterialIndex = outMaterials.addMaterial(globalMaterial->name);
                    }
                }

                if (faces.globalMaterialIndex == INDEX_NONE)
                    faces.globalMaterialIndex = outMaterials.addMaterial("default");

                TRACE_INFO("Geometry '{}': found {} faces for local material {} (global index {})", m_name, facesPerMaterial[i].count, i, faces.globalMaterialIndex);
            }
        }
    }

    // determine if we need to flip the faces
    const auto normalVertexOrder = (geometryToEngine.det3() > 0.0f) ^ config.flipFace;

    // source stream data
    FBXMeshStreams sourceStreams(geom, skinInfluences);
    sourceStreams.flipUV = config.flipUV;

    // process chunk data
    // TODO: run on fibers
    for (const auto& buildChunk : facesPerMaterial)
    {
        if (!buildChunk.count)
            continue;

        if (progress.checkCancelation())
            break;

        {
            MeshWriteStreams writer(topology, buildChunk.count, sourceStreams.streamMask);
            ExtractFaces(sourceStreams, geometryToEngine, buildChunk.firstFace, writer, normalVertexOrder);

            // finalize chunk
            auto& exportChunk = outGeometry.chunks.emplaceBack();
            exportChunk.materialIndex = buildChunk.globalMaterialIndex;
            exportChunk.detailMask = 1U << m_lodIndex;
            writer.extract(exportChunk);
            TRACE_INFO("FBX: Extracted {} faces from mesh {}, material {}", exportChunk.numFaces, m_name, exportChunk.materialIndex);
        }
    }
}

//---

RTTI_BEGIN_TYPE_CLASS(FBXFile);
RTTI_END_TYPE();

FBXFile::FBXFile()
{
    m_nodes.reserve(256);
    m_nodeMap.reserve(256);
}

FBXFile::~FBXFile()
{
    if (m_fbxScene)
    {
        m_fbxScene->destroy();
        m_fbxScene = nullptr;
    }
}

const ofbx::Material* FBXFile::findMaterial(StringView name) const
{
    for (const auto* mat : m_materialMap.values())
        if (0 == name.caseCmp(mat->name))
            return mat;

    /*const ofbx::Material* ret = nullptr;
    m_materialMap.find(name, ret);
    return ret;*/

    return nullptr;
}

const DataNode* FBXFile::findDataNode(const ofbx::Object* fbxNode) const
{
    const DataNode* ret = nullptr;
    m_nodeMap.find(fbxNode, ret);
    return ret;
}

static bool ValidNodeForCapture(const ofbx::Object* node)
{
    switch (node->getType())
    {
    case ofbx::Object::Type::ROOT:
    case ofbx::Object::Type::MESH:
    case ofbx::Object::Type::LIMB_NODE:
    case ofbx::Object::Type::NULL_NODE:
        return true;
    }

    return false;
}

void FBXFile::collectExportNodes(const DataNode* node, const Matrix& localToAsset, Array<ExportNode>& outExportNodes) const
{
    auto& entry = outExportNodes.emplaceBack();
    entry.data = node;
    entry.localToAsset = localToAsset;

    for (const auto* child : node->m_children)
    {
        const auto childToAsset = child->m_localToParent * localToAsset;
        collectExportNodes(child, childToAsset, outExportNodes);
    }
}

void FBXFile::collectExportNodes(bool applyRootTransform, Array<ExportNode>& outExportNodes) const
{
    if (applyRootTransform)
    {
        collectExportNodes(rootNode(), Matrix::IDENTITY(), outExportNodes);
    }
    else
    {
        for (const auto* child : rootNode()->m_children)
            collectExportNodes(child, Matrix::IDENTITY(), outExportNodes);
    }
}

void FBXFile::walkStructure(/*const Matrix& parentToAsset, */const ofbx::Object* node, DataNode* parentDataNode)
{
    // capture only some nodes
    if (!ValidNodeForCapture(node))
        return;

    // create the local node
    auto localNode = new DataNode;
    localNode->m_name = StringID(node->name);

    // link in hierarchy
    if (parentDataNode)
        parentDataNode->m_children.pushBack(localNode);
    localNode->m_parent = parentDataNode;
    m_nodes.pushBack(localNode);

    // map
    localNode->m_node = node;
    m_nodeMap[node] = localNode;

    // get transforms
    localNode->m_localToParent = ToMatrix(node->getLocalTransform());
    //localNode->m_localToAsset = localNode->m_localToParent * parentToAsset;
    TRACE_INFO("LocalToParent for '{}': {}", localNode->m_name, localNode->m_localToParent);
    //TRACE_INFO("LocalToAsset for '{}': {}", localNode->m_name, localNode->m_localToAsset);

    // extract mesh at this node
    if (node->getType() == ofbx::Object::Type::MESH)
    {
        const auto* mesh = static_cast<const ofbx::Mesh*>(node);

        // matrix
        const auto geometryToLocal = ToMatrix(mesh->getGeometricMatrix());
        TRACE_INFO("GeomtryToLocal for '{}': {}", localNode->m_name, geometryToLocal);

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

        // extract materials
        {
            const auto numMaterials = mesh->getMaterialCount();
            for (uint32_t i = 0; i < numMaterials; ++i)
            {
                auto lMaterial = mesh->getMaterial(i);
                auto materialName = lMaterial ? lMaterial->name : localNode->m_name.c_str();

                if (!m_materialMap.contains(materialName))
                {
                    TRACE_INFO("Discovered FBX material '{}'", materialName);
                    m_materialMap[StringBuf(materialName)] = lMaterial;
                    m_materials.pushBack(lMaterial);
                }
            }
        }
    }

    // visit children
    for (uint32_t i = 0; ; ++i)
    {
        const auto* childNode = node->resolveObjectLink(i);
        if (!childNode)
            break;

        walkStructure(/*localNode->m_localToAsset, */childNode, localNode);
    }
}    

uint64_t FBXFile::calcMemoryUsage() const
{
    return m_fbxDataSize;
}

//---

static MeshImportSpace FindMeshImportSpace(const ofbx::GlobalSettings& settings)
{
    if (settings.UpAxis == ofbx::UpVector_AxisZ)
    {
        if (settings.CoordAxis == ofbx::CoordSystem_RightHanded)
            return MeshImportSpace::RightHandZUp;
        else
            return MeshImportSpace::LeftHandZUp;
    }
    else
    {
        if (settings.CoordAxis == ofbx::CoordSystem_RightHanded)
            return MeshImportSpace::RightHandYUp;
        else
            return MeshImportSpace::LeftHandYUp;
    }
}

/// source asset loader for OBJ data
class FBXAssetLoader : public ISourceAssetLoader
{
    RTTI_DECLARE_VIRTUAL_CLASS(FBXAssetLoader, ISourceAssetLoader);

public:
    virtual SourceAssetPtr loadFromMemory(StringView importPath, StringView contextPath, Buffer data) const override
    {
        if (!data)
            return nullptr;

        auto scene = ofbx::load(data.data(), data.size(), 0);
        if (!scene)
            return nullptr;

        auto ret = RefNew<FBXFile>();
        ret->m_fbxDataSize = data.size();
        ret->m_fbxScene = scene;
        ret->m_scaleFactor = scene->getGlobalSettings()->OriginalUnitScaleFactor;
        ret->m_space = FindMeshImportSpace(*scene->getGlobalSettings());

        if (const auto* rootNode = ret->m_fbxScene->getRoot())
            ret->walkStructure(rootNode, nullptr);

        return ret;
    }
};

RTTI_BEGIN_TYPE_CLASS(FBXAssetLoader);
    RTTI_METADATA(ResourceSourceFormatMetadata).addSourceExtensions("fbx").addSourceExtensions("FBX");
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(assets)
