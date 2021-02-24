/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#pragma once

#include "base/resource_compiler/include/importSourceAsset.h"
#include "rendering/mesh/include/renderingMeshStreamBuilder.h"
#include "rendering/mesh/include/renderingMeshStreamData.h"

BEGIN_BOOMER_NAMESPACE(asset)

//---

struct DataNode;

// type of node
enum class DataNodeType : uint8_t
{
    VisualMesh,
    TriangleCollision,
    ConvexCollision,
    Helper,
};

// bone in the animated skeleton
struct SkeletonBone
{
    int m_parentIndex = -1;
    const DataNode* m_data = nullptr;
};

//  skeleton collector
struct IMPORT_FBX_LOADER_API SkeletonBuilder
{
    base::Array<SkeletonBone> m_skeletonBones;

    base::HashMap<const DataNode*, uint32_t> m_skeletonBonesMapping;
    base::HashMap<const fbxsdk::FbxNode*, uint32_t> m_skeletonRawBonesMapping;

    //--

    SkeletonBuilder();

    uint32_t addBone(const LoadedFile& owner, const DataNode* node);
    uint32_t addBone(const LoadedFile& owner, const fbxsdk::FbxNode* node);
};

struct MeshVertexInfluence;

// material mapper
struct IMPORT_FBX_LOADER_API MaterialMapper
{
    base::Array<base::StringBuf> materials;
    base::HashMap<base::StringBuf, uint32_t> materialsMapping;

    MaterialMapper();

    uint32_t addMaterial(const char* materialName);
};

// exported mode
struct IMPORT_FBX_LOADER_API DataNodeMesh
{
    base::Array<rendering::MeshRawChunk> chunks;
};

// mesh export settings
struct IMPORT_FBX_LOADER_API DataMeshExportSetup
{
    base::Matrix assetToEngine;
    bool applyNodeTransform = false; // export in "world space" of the model, rare
    bool forceSkinToNode = false;
    bool flipUV = false;
    bool flipFace = false;
};

//  extractable node
struct IMPORT_FBX_LOADER_API DataNode
{
    DataNodeType m_type = DataNodeType::Helper;
    base::StringID m_name;
    uint8_t m_lodIndex = 0; // for Visual Mesh

    //base::Matrix m_worldToAsset; // space conversion matrix for the FBX file
    base::Matrix m_localToWorld; // placement of the node in world space
    base::Matrix m_meshToLocal;

    DataNode* m_parent = nullptr;
    base::Array<DataNode*> m_children;

    const fbxsdk::FbxMesh* m_mesh = nullptr;
    const fbxsdk::FbxNode* m_node = nullptr;

    //--

    struct ChunkInfo
    {
        uint32_t materialIndex = 0; // global
        uint32_t numTriangles = 0; // only if we export triangles
        uint32_t numQuads = 0; // only if we export quads
        base::Array<uint32_t> polygonIndices;
        base::Array<uint32_t> firstVertexIndices;

        INLINE rendering::MeshTopologyType topology() const
        {
            DEBUG_CHECK(numTriangles == 0 || numQuads == 0);
            return numQuads ? rendering::MeshTopologyType::Quads : rendering::MeshTopologyType::Triangles;
        }

        INLINE uint32_t faceCount() const
        {
            DEBUG_CHECK(numTriangles == 0 || numQuads == 0);
            return numQuads ? numQuads : numTriangles;
        }
    };


    void extractSkinInfluences(const LoadedFile& owner, base::Array<MeshVertexInfluence>& outInfluences, SkeletonBuilder& outSkeleton, bool forceSkinToNode) const;

    void extractBuildChunks(const LoadedFile& owner, base::Array<ChunkInfo>& outBuildChunks, MaterialMapper& outMaterials) const;

    bool exportToMeshModel(base::IProgressTracker& progress, const LoadedFile& owner, const DataMeshExportSetup& config, DataNodeMesh& outGeonetry, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials) const;
};

class FBXAssetLoader;

//  data blob (loaded asset scene)
class IMPORT_FBX_LOADER_API LoadedFile : public base::res::ISourceAsset
{
    RTTI_DECLARE_VIRTUAL_CLASS(LoadedFile, base::res::ISourceAsset);

public:
    LoadedFile();
    virtual ~LoadedFile();

    ///--

    // get the scene object
    INLINE fbxsdk::FbxScene* sceneData() const { return m_fbxScene; }

    // get the root node
    INLINE const DataNode* rootNode() const { return m_nodes[0]; }

    // get all nodes, parents are always before children
    INLINE const base::Array<const DataNode*>& nodes() const { return (const base::Array<const DataNode*>&)m_nodes; }

    // get all materials
    INLINE const base::Array<const FbxSurfaceMaterial*>& materials() const { return m_materials; }

    ///--

    // find data node for given  node
    const DataNode* findDataNode(const fbxsdk::FbxNode* fbxNode) const;

    // find material by name
    const FbxSurfaceMaterial* findMaterial(base::StringView name) const;

    //--

private:
    uint64_t m_fbxDataSize = 0;
    fbxsdk::FbxScene* m_fbxScene = nullptr;

    base::Array<DataNode*> m_nodes;
    base::HashMap<const fbxsdk::FbxNode*, const DataNode*> m_nodeMap;

    base::Array<const FbxSurfaceMaterial*> m_materials;
    base::HashMap<base::StringBuf, const FbxSurfaceMaterial*> m_materialMap;

    void walkStructure(const fbxsdk::FbxAMatrix& fbxWorldToParent, const base::Matrix& spaceConversionMatrix, const fbxsdk::FbxNode* node, DataNode* parentDataNode);

    //--

    // ISourceAsset
    virtual uint64_t calcMemoryUsage() const override;
    //virtual bool loadFromMemory(Buffer data) override;

    //--

    friend class FBXAssetLoader;
};

//--
    
END_BOOMER_NAMESPACE(asset)

