/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streams #]
***/

#include "build.h"
#include "streamData.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ENUM(MeshStreamType);
RTTI_OLD_NAME("MeshStreamType");
RTTI_ENUM_OPTION(Position_3F);
RTTI_ENUM_OPTION(Normal_3F);
RTTI_ENUM_OPTION(Tangent_3F);
RTTI_ENUM_OPTION(Binormal_3F);
RTTI_ENUM_OPTION(TexCoord0_2F);
RTTI_ENUM_OPTION(TexCoord1_2F);
RTTI_ENUM_OPTION(TexCoord2_2F);
RTTI_ENUM_OPTION(TexCoord3_2F);
RTTI_ENUM_OPTION(Color0_4U8);
RTTI_ENUM_OPTION(Color1_4U8);
RTTI_ENUM_OPTION(Color2_4U8);
RTTI_ENUM_OPTION(Color3_4U8);
RTTI_ENUM_OPTION(SkinningIndices_4U8);
RTTI_ENUM_OPTION(SkinningWeights_4F);
RTTI_ENUM_OPTION(SkinningIndicesEx_4U8);
RTTI_ENUM_OPTION(SkinningWeightsEx_4F);
RTTI_ENUM_OPTION(TreeLodPosition_3F);
RTTI_ENUM_OPTION(TreeWindBranchData_4F);
RTTI_ENUM_OPTION(TreeBranchData_7F);
RTTI_ENUM_OPTION(TreeFrondData_4F);
RTTI_ENUM_OPTION(TreeLeafAnchors_3F);
RTTI_ENUM_OPTION(TreeLeafWindData_4F);
RTTI_ENUM_OPTION(TreeLeafCardCorner_3F);
RTTI_ENUM_OPTION(TreeLeafLod_1F);
RTTI_ENUM_OPTION(General0_F4);
RTTI_ENUM_OPTION(General1_F4);
RTTI_ENUM_OPTION(General2_F4);
RTTI_ENUM_OPTION(General3_F4);
RTTI_ENUM_OPTION(General4_F4);
RTTI_ENUM_OPTION(General5_F4);
RTTI_ENUM_OPTION(General6_F4);
RTTI_ENUM_OPTION(General7_F4);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(MeshChunkRenderingMaskBit);
RTTI_ENUM_OPTION(Scene);
RTTI_ENUM_OPTION(ObjectShadows);
RTTI_ENUM_OPTION(LocalShadows);
RTTI_ENUM_OPTION(Cascade0);
RTTI_ENUM_OPTION(Cascade1);
RTTI_ENUM_OPTION(Cascade2);
RTTI_ENUM_OPTION(Cascade3);
RTTI_ENUM_OPTION(LodMerge);
RTTI_ENUM_OPTION(ShadowMesh);
RTTI_ENUM_OPTION(LocalReflection);
RTTI_ENUM_OPTION(GlobalReflection);
RTTI_ENUM_OPTION(Lighting);
RTTI_ENUM_OPTION(StaticOccluder);
RTTI_ENUM_OPTION(DynamicOccluder);
RTTI_ENUM_OPTION(ConvexCollision);
RTTI_ENUM_OPTION(ExactCollision);
RTTI_ENUM_OPTION(Cloth);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_BITFIELD(MeshChunkRenderingMask);
RTTI_BITFIELD_OPTION(Scene);
RTTI_BITFIELD_OPTION(ObjectShadows);
RTTI_BITFIELD_OPTION(LocalShadows);
RTTI_BITFIELD_OPTION(Cascade0);
RTTI_BITFIELD_OPTION(Cascade1);
RTTI_BITFIELD_OPTION(Cascade2);
RTTI_BITFIELD_OPTION(Cascade3);
RTTI_BITFIELD_OPTION(LodMerge);
RTTI_BITFIELD_OPTION(ShadowMesh);
RTTI_BITFIELD_OPTION(LocalReflection);
RTTI_BITFIELD_OPTION(GlobalReflection);
RTTI_BITFIELD_OPTION(Lighting);
RTTI_BITFIELD_OPTION(StaticOccluder);
RTTI_BITFIELD_OPTION(DynamicOccluder);
RTTI_BITFIELD_OPTION(ConvexCollision);
RTTI_BITFIELD_OPTION(ExactCollision);
RTTI_BITFIELD_OPTION(Cloth);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(MeshTopologyType);
RTTI_ENUM_OPTION(Triangles);
RTTI_ENUM_OPTION(Quads);
RTTI_END_TYPE();

//--

uint32_t GetMeshStreamStride(MeshStreamType type)
{
    switch (type)
    {
    case MeshStreamType::TexCoord0_2F:
    case MeshStreamType::TexCoord1_2F:
    case MeshStreamType::TexCoord2_2F:
    case MeshStreamType::TexCoord3_2F:
        return 8;

    case MeshStreamType::TreeLeafLod_1F:
    case MeshStreamType::Color0_4U8:
    case MeshStreamType::Color1_4U8:
    case MeshStreamType::Color2_4U8:
    case MeshStreamType::Color3_4U8:
    case MeshStreamType::SkinningIndices_4U8:
    case MeshStreamType::SkinningIndicesEx_4U8:
        return 4;

    case MeshStreamType::Position_3F:
    case MeshStreamType::Normal_3F:
    case MeshStreamType::Tangent_3F:
    case MeshStreamType::Binormal_3F:
    case MeshStreamType::TreeLodPosition_3F:
    case MeshStreamType::TreeLeafAnchors_3F:
    case MeshStreamType::TreeLeafCardCorner_3F:
        return 12;

    case MeshStreamType::SkinningWeights_4F:
    case MeshStreamType::SkinningWeightsEx_4F:
    case MeshStreamType::TreeWindBranchData_4F:
    case MeshStreamType::TreeFrondData_4F:
    case MeshStreamType::TreeLeafWindData_4F:
        return 16;

    case MeshStreamType::TreeBranchData_7F:
        return 28;
    }

    FATAL_ERROR("Invalid mesh stream type");
    return 0;
}

Buffer CreateMeshStreamBuffer(MeshStreamType type, uint32_t vertexCount)
{
    Buffer ret;

    if (vertexCount)
    {
        const auto dataSize = vertexCount * GetMeshStreamStride(type);
        ret.initWithZeros(POOL_TEMP, dataSize, 16);
    }

    return ret;
}

//--

RTTI_BEGIN_TYPE_STRUCT(MeshRawChunkData);
    RTTI_PROPERTY(type);
    RTTI_PROPERTY(data);
RTTI_END_TYPE();

MeshRawChunkData::MeshRawChunkData()
    : type(MeshStreamType::Position_3F)
{}

//---

RTTI_BEGIN_TYPE_STRUCT(MeshRawChunk);
    RTTI_PROPERTY_FORCE_TYPE(streamMask, uint64_t);
    RTTI_PROPERTY_FORCE_TYPE(renderMask, uint32_t);
    RTTI_PROPERTY(detailMask);
    RTTI_PROPERTY(materialIndex);
    RTTI_PROPERTY(topology);
    RTTI_PROPERTY(numVertices);
    RTTI_PROPERTY(numFaces);
    RTTI_PROPERTY(streams);
    //RTTI_PROPERTY(faces);
RTTI_END_TYPE();

MeshRawChunk::MeshRawChunk()
    : streamMask(0)
    , renderMask(MeshChunkRenderingMaskBit::DEFAULT)
    , detailMask(1)
    , materialIndex(0)
    , topology(MeshTopologyType::Triangles)
    , numFaces(0)
    , numVertices(0)
{}

//--

END_BOOMER_NAMESPACE()
