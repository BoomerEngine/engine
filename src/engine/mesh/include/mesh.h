/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "format.h"
#include "core/object/include/compressedBuffer.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// renderable mesh chunk
struct ENGINE_MESH_API MeshChunk
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MeshChunk);

	MeshChunkProxyPtr proxy; // runtime

    MeshVertexFormat vertexFormat;

    uint16_t materialIndex = 0;
    uint32_t renderMask = 0;
    uint32_t detailMask = 0;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;

    Vector3 quantizationOffset;
    Vector3 quantizationScale;

    CompressedBufer packedVertexData; // packed (compressed) vertex data
    CompressedBufer packedIndexData; // packed (compressed) index data

    uint32_t unpackedVertexSize = 0;
    uint32_t unpackedIndexSize = 0;
};

//---

/// rendering detail range (LOD range)
struct ENGINE_MESH_API MeshDetailLevel
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MeshDetailLevel);

    float rangeMin = 0.0f;
    float rangeMax = FLT_MAX;
};

//---

    /// mesh visibility group, defines what kind of stuff we are dealing with
enum class MeshVisibilityGroup : uint8_t
{
    Generic,
    SmallProps,

    // TODO: more

    MAX,
};

/// visibility group info
struct MeshVisibilityGroupInfo
{
    StringView name;
    float referenceVisibilityDistance = 100.0f; // distance at which this kind of mesh should be visible
    float referenceStreamingDistance = 0.0f; // distance at which this kind of mesh should stream
};

/// get info about mesh visibility group
extern ENGINE_MESH_API const MeshVisibilityGroupInfo* GetMeshVisibilityGroupInfo(MeshVisibilityGroup group);

//---

/// material information
struct ENGINE_MESH_API MeshMaterial
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MeshMaterial);

    StringID name;
    MaterialInstancePtr material; // local override built base on config
};

//---

/// mesh initialization data
struct ENGINE_MESH_API MeshInitData
{
    MeshVisibilityGroup visibilityGroup = MeshVisibilityGroup::Generic;
    float visibilityDistanceMultiplier = 1.0f;
    float visibilityDistanceOverride = 0.0f;

    Box bounds;

    Array<MeshChunk> chunks;
    Array<MeshDetailLevel> detailLevels;
    Array<MeshMaterial> materials;

    //--
};

//---

/// a rendering mesh
class ENGINE_MESH_API Mesh : public IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(Mesh, IResource);

public:
    Mesh();
    Mesh(MeshInitData && data);
    virtual ~Mesh();

    //---

    /// get mesh bounds that cover all the mesh vertices
    INLINE const Box& bounds() const { return m_bounds; }

    // get the content group this mesh belongs to
    INLINE MeshVisibilityGroup visibilityGroup() const { return m_visibilityGroup; }

    /// get the visibility radius multiplier for this mesh
    INLINE float visibilityDistanceMultiplier() const { return m_visibilityDistanceMultiplier; }

    /// get the custom forced visibility for this mesh
    INLINE float visibilityDistanceOverride() const { return m_visibilityDistanceOverride; }

    //---

    // get used materials
    INLINE const Array<MeshMaterial>& materials() const { return m_materials; }

    // get used bones
    //INLINE const Array<StringID>& boneRefs() const { return m_materialRefs; }

    // raw data chunk
    INLINE const Array<MeshChunk>& chunks() const { return m_chunks; }

    // detail levels
    INLINE const Array<MeshDetailLevel>& detailLevels() const { return m_details; }

    //---

    // helper, select LOD index based on given distance, will return -1 if we are outside the visibility distance
    int calculateHighestDetailLevel(float distance) const;

    // helper, calculate the detail mask (all active LODs) for given distance
    uint32_t calculateActiveDetailLevels(float distance) const;

    //--

protected:
    Box m_bounds;

    Array<MeshMaterial> m_materials;
    Array<MeshChunk> m_chunks;
    Array<MeshDetailLevel> m_details;

    MeshVisibilityGroup m_visibilityGroup = MeshVisibilityGroup::Generic;
    float m_visibilityDistanceMultiplier = 1.0f;
    float m_visibilityDistanceOverride = 0.0f;

    void registerChunks();
    void unregisterChunks();

    // IObject
    virtual void onPostLoad() override;
};

///---

END_BOOMER_NAMESPACE()
