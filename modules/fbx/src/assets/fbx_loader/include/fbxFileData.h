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

    struct MeshVertexInfluence;

    // mateiral info
    struct ASSETS_FBX_LOADER_API MaterialEntry
    {
        base::StringBuf name;
        const fbxsdk::FbxSurfaceMaterial* sourceData = nullptr;

        MaterialEntry();
    };

    // material mapper
    struct ASSETS_FBX_LOADER_API MaterialMapper
    {
        base::Array<MaterialEntry> materials;
        base::HashMap<base::StringBuf, uint32_t> materialsMapping;

        MaterialMapper();

        uint32_t addMaterial(const char* materialName, const fbxsdk::FbxSurfaceMaterial* material);
    };

    //  extractable node
    struct ASSETS_FBX_LOADER_API DataNode
    {
        DataNodeType m_type = DataNodeType::Helper;
        base::StringID m_name;
        uint8_t m_lodIndex = 0; // for Visual Mesh

        base::Matrix m_localToWorld; // placement of the node in world space
        base::Matrix m_meshToWorld; // used for mesh vertices import, may contain additional

        base::Matrix m_localToParent; // cached m_localToWorld * m_parent->m_localToWorld.inverted()

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

            INLINE base::mesh::MeshTopologyType topology() const
            {
                DEBUG_CHECK(numTriangles == 0 || numQuads == 0);
                return numQuads ? base::mesh::MeshTopologyType::Quads : base::mesh::MeshTopologyType::Triangles;
            }

            INLINE uint32_t faceCount() const
            {
                DEBUG_CHECK(numTriangles == 0 || numQuads == 0);
                return numQuads ? numQuads : numTriangles;
            }
        };


        void extractSkinInfluences(const LoadedFile& owner, base::Array<MeshVertexInfluence>& outInfluences, SkeletonBuilder& outSkeleton, bool forceSkinToNode) const;

        void extractBuildChunks(const LoadedFile& owner, base::Array<ChunkInfo>& outBuildChunks, MaterialMapper& outMaterials) const;

        bool exportToMeshModel(base::IProgressTracker& progress, const LoadedFile& owner, const base::Matrix& worldToEngine, base::mesh::MeshModel& outGeonetry, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials, bool forceSkinToNode, bool flipUV, bool flipFace) const;
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

        bool walkStructure(const fbxsdk::FbxAMatrix& fbxWorldToParent, const base::Matrix& spaceConversionMatrix, const fbxsdk::FbxNode* node, DataNode* parentDataNode);
    };

    //---

} // fbx

