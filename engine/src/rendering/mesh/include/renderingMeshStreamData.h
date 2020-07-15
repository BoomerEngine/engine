/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streams #]
***/

#pragma once

namespace rendering
{
        
    ///-----

    /// type of data in the mesh stream
    enum class MeshStreamType : uint8_t
    {
        // float3 POSITION
        Position_3F = 0,

        // tangent space
        Normal_3F = 1,
        Tangent_3F = 2,
        Binormal_3F = 3,

        // UVs
        TexCoord0_2F = 4,
        TexCoord1_2F = 5,
        TexCoord2_2F = 6,
        TexCoord3_2F = 7,

        // colors
        Color0_4U8 = 8,
        Color1_4U8 = 9,
        Color2_4U8 = 10,
        Color3_4U8 = 11,

        // 4 bone skinning
        SkinningIndices_4U8 = 16,
        SkinningWeights_4F = 17,

        // 8 bone skinning (extra data)
        SkinningIndicesEx_4U8 = 18,
        SkinningWeightsEx_4F = 19,

        // trees/foliage
        TreeLodPosition_3F = 40,
        TreeWindBranchData_4F = 41,
        TreeBranchData_7F = 42,
        TreeFrondData_4F = 43,
        TreeLeafAnchors_3F = 44,
        TreeLeafWindData_4F = 45,
        TreeLeafCardCorner_3F = 46,
        TreeLeafLod_1F = 47,

        // extra/general purpose streams
        General0_F4 = 50,
        General1_F4 = 51,
        General2_F4 = 52,
        General3_F4 = 53,
        General4_F4 = 54,
        General5_F4 = 55,
        General6_F4 = 56,
        General7_F4 = 57,
    };

    /// mask of streams
    typedef uint64_t MeshStreamMask;

    /// get stream mask from stream type
    INLINE constexpr MeshStreamMask MeshStreamMaskFromType(MeshStreamType type) { return 1ULL << (uint64_t)type; }

    /// get stride for mesh data stream
    extern RENDERING_MESH_API uint32_t GetMeshStreamStride(MeshStreamType type);

    /// create empty stream buffer for given vertex stream
    extern RENDERING_MESH_API base::Buffer CreateMeshStreamBuffer(MeshStreamType type, uint32_t vertexCount);

    ///----

    // data stream in a mesh
    struct RENDERING_MESH_API MeshRawChunkData
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MeshRawChunkData);

    public:
        MeshRawChunkData();

        MeshStreamType type; // stream type
        base::Buffer data; // data buffer
    };

    //--

    // raw chunk in mesh - group of per-vertex data streams
    struct RENDERING_MESH_API MeshRawChunk
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MeshRawChunk);

    public:
        MeshRawChunk();

        MeshStreamMask streamMask; // streams in the chunk
        MeshChunkRenderingMask renderMask; // render mask for this chunk
        uint32_t detailMask; // detail mask (LOD levels this chunk is in)
        uint32_t materialIndex; // index of the material in the material table

        MeshTopologyType topology; // rendering topology
        uint32_t numVertices; // number of vertices in the chunk
        uint32_t numFaces; // number of faces in the chunk, this groups N vertices into a face 

        base::Array<MeshRawChunkData> streams; // stream of per-vertex data

        base::Box bounds; // position bounds in model space
    };

    ///----

} // rendering