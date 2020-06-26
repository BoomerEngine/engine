/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#pragma once

namespace fbx
{
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
    struct ASSETS_FBX_LOADER_API SkeletonBuilder
    {
        base::Array<SkeletonBone> m_skeletonBones;

        base::HashMap<const DataNode*, uint32_t> m_skeletonBonesMapping;
        base::HashMap<const fbxsdk::FbxNode*, uint32_t> m_skeletonRawBonesMapping;

        //--

        SkeletonBuilder();

        uint32_t addBone(const LoadedFile& owner, const DataNode* node);
        uint32_t addBone(const LoadedFile& owner, const fbxsdk::FbxNode* node);
    };

    //  material mapper
    struct ASSETS_FBX_LOADER_API MaterialMapper
    {
        base::Array<const fbxsdk::FbxSurfaceMaterial*> m_materials;
        base::HashMap<const fbxsdk::FbxSurfaceMaterial*, uint32_t> m_materialsMapping;

        MaterialMapper();

        uint32_t addMaterial(const fbxsdk::FbxSurfaceMaterial* material);
    };

    //  extractable node
    struct ASSETS_FBX_LOADER_API DataNode
    {
        DataNodeType m_type = DataNodeType::Helper;
        base::StringID m_name;
        uint8_t m_lodIndex = 0; // for Visual Mesh

        base::Matrix m_localToParent;
        base::Matrix m_localToWorld;

        DataNode* m_parent = nullptr;
        base::Array<DataNode*> m_children;

        const fbxsdk::FbxMesh* m_mesh = nullptr;
        const fbxsdk::FbxNode* m_node = nullptr;

        //--

        // write content to mesh builder
        //void exportToMeshBuilder(const LoadedFile& owner, const base::Matrix& fileToWorld, rendering::content::MeshGeometryBuilder& outGeonetry, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials, bool forceSkinToNode) const;
    };

    //  data blob (loaded scene)
    class ASSETS_FBX_LOADER_API LoadedFile : public base::IReferencable
    {
    public:
        LoadedFile(fbxsdk::FbxScene* fbxScene = nullptr);
        virtual ~LoadedFile();

        ///--

        // get the scene object
        INLINE fbxsdk::FbxScene* sceneData() const { return m_fbxScene; }

        // get the root node
        INLINE const DataNode* rootNode() const { return m_nodes[0]; }

        // get all nodes, parents are always before children
        INLINE const base::Array<const DataNode*>& nodes() const { return (const base::Array<const DataNode*>&)m_nodes; }

        ///--

        // extract internal node structure (capture our own representation of the scene graph)
        bool captureNodes(const base::Matrix& spaceConversionMatrix);

        ///--

        // find data node for given  node
        const DataNode* findDataNode(const fbxsdk::FbxNode* fbxNode) const;

    private:
        fbxsdk::FbxScene* m_fbxScene;
        base::Array<DataNode*> m_nodes;
        base::HashMap<const fbxsdk::FbxNode*, const DataNode*> m_nodeMap;

        bool walkStructure(const base::Matrix& worldToParent, const base::Matrix& spaceConversionMatrix, const fbxsdk::FbxNode* node, DataNode* parentDataNode);
    };

    //---

} // fbx

