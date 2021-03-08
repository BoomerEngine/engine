/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#pragma once

#include "core/resource_compiler/include/sourceAsset.h"
#include "engine/mesh/include/streamBuilder.h"
#include "engine/mesh/include/streamData.h"
#include "import/mesh_loader/include/renderingMeshImportConfig.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

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
    Array<SkeletonBone> m_skeletonBones;

    HashMap<const DataNode*, uint32_t> m_skeletonBonesMapping;
    HashMap<const ofbx::Object*, uint32_t> m_skeletonRawBonesMapping;

    //--

    SkeletonBuilder();

    uint32_t addBone(const FBXFile& owner, const DataNode* node);
    uint32_t addBone(const FBXFile& owner, const ofbx::Object* node);
};

struct MeshVertexInfluence;

// material mapper
struct IMPORT_FBX_LOADER_API MaterialMapper
{
    Array<StringBuf> materials;
    HashMap<StringBuf, uint32_t> materialsMapping;

    MaterialMapper();

    uint32_t addMaterial(const char* materialName);
};

// exported mode
struct IMPORT_FBX_LOADER_API DataNodeMesh
{
    Array<MeshRawChunk> chunks;
};

// mesh export settings
struct IMPORT_FBX_LOADER_API DataMeshExportSetup
{
    Matrix assetToEngine;
    bool applyRootTransform = false;
    bool forceSkinToNode = false;
    bool flipUV = false;
    bool flipFace = false;
};

//  extractable node
struct IMPORT_FBX_LOADER_API DataNode
{
    DataNodeType m_type = DataNodeType::Helper;
    StringID m_name;
    uint8_t m_lodIndex = 0; // for Visual Mesh

    //Matrix m_localToAsset; // placement of the node in asset space
    Matrix m_localToParent; // placement of the node in parent space

    DataNode* m_parent = nullptr;
    Array<DataNode*> m_children;

    const ofbx::Object* m_node = nullptr;

    //--

    void exportToMeshModel(IProgressTracker& progress, const FBXFile& owner, const Matrix& localToAsset, const DataMeshExportSetup& config, DataNodeMesh& outGeometry, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials) const;
};

// export node
struct IMPORT_FBX_LOADER_API ExportNode
{
    const DataNode* data = nullptr;
    Matrix localToAsset;
};

class FBXAssetLoader;

//  data blob (loaded asset scene)
class IMPORT_FBX_LOADER_API FBXFile : public ISourceAsset
{
    RTTI_DECLARE_VIRTUAL_CLASS(FBXFile, ISourceAsset);

public:
    FBXFile();
    virtual ~FBXFile();

    ///--

    // get the scene object
    INLINE ofbx::IScene* sceneData() const { return m_fbxScene; }

    // get the root node
    INLINE const DataNode* rootNode() const { return m_nodes[0]; }

    // get all nodes, parents are always before children
    INLINE const Array<const DataNode*>& nodes() const { return (const Array<const DataNode*>&)m_nodes; }

    // get all materials
    INLINE const Array<const ofbx::Material*>& materials() const { return m_materials; }

    // get the FILE scale factor
    INLINE float scaleFactor() const { return m_scaleFactor; }

    // get the import space of the mesh
    INLINE MeshImportSpace space() const { return m_space; }

    ///--

    // find data node for given  node
    const DataNode* findDataNode(const ofbx::Object* fbxNode) const;

    // find material by name
    const ofbx::Material* findMaterial(StringView name) const;

    //--

    // collect export nodes
    void collectExportNodes(bool applyRootTransform, Array<ExportNode>& outExportNodes) const;

private:
    uint64_t m_fbxDataSize = 0;
    ofbx::IScene* m_fbxScene = nullptr;

    float m_scaleFactor = 1.0f;
    MeshImportSpace m_space = MeshImportSpace::LeftHandYUp;

    Array<DataNode*> m_nodes;
    HashMap<const ofbx::Object*, const DataNode*> m_nodeMap;

    Array<const ofbx::Material*> m_materials;
    HashMap<StringBuf, const ofbx::Material*> m_materialMap;

    void walkStructure(const ofbx::Object* node, DataNode* parentDataNode);
    void collectExportNodes(const DataNode* node, const Matrix& localToAsset, Array<ExportNode>& outExportNodes) const;

    //--

    // ISourceAsset
    virtual uint64_t calcMemoryUsage() const override;
    //virtual bool loadFromMemory(Buffer data) override;

    //--

    friend class FBXAssetLoader;
};

//--
    
END_BOOMER_NAMESPACE_EX(assets)

