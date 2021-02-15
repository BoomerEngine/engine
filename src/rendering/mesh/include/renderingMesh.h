/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "renderingMeshFormat.h"

namespace rendering
{
    //---

    /// renderable mesh chunk
    struct RENDERING_MESH_API MeshChunk
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MeshChunk);

		MeshChunkProxyPtr proxy; // runtime

        MeshVertexFormat vertexFormat;

        uint16_t materialIndex = 0;
        uint32_t renderMask = 0;
        uint32_t detailMask = 0;
        uint32_t indexCount = 0;
        uint32_t vertexCount = 0;

        uint32_t unpackedVertexSize = 0;
        uint32_t unpackedIndexSize = 0;

        base::Vector3 quantizationOffset;
        base::Vector3 quantizationScale;

        base::Buffer packedVertexData; // packed (compressed) vertex data
        base::Buffer packedIndexData; // packed (compressed) index data
    };

    //---

    /// rendering detail range (LOD range)
    struct RENDERING_MESH_API MeshDetailLevel
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
        base::StringView name;
        float referenceVisibilityDistance = 100.0f; // distance at which this kind of mesh should be visible
        float referenceStreamingDistance = 0.0f; // distance at which this kind of mesh should stream
    };

    /// get info about mesh visibility group
    extern RENDERING_MESH_API const MeshVisibilityGroupInfo* GetMeshVisibilityGroupInfo(MeshVisibilityGroup group);

    //---

    /// material information
    struct RENDERING_MESH_API MeshMaterial
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MeshMaterial);

        base::StringID name;
        MaterialInstancePtr material; // local override built base on config
    };

    //---

    /// mesh initialization data
    struct RENDERING_MESH_API MeshInitData
    {
        MeshVisibilityGroup visibilityGroup = MeshVisibilityGroup::Generic;
        float visibilityDistanceMultiplier = 1.0f;
        float visibilityDistanceOverride = 0.0f;

        base::Box bounds;

        base::Array<MeshChunk> chunks;
        base::Array<MeshDetailLevel> detailLevels;
        base::Array<MeshMaterial> materials;

        //--
    };

    //---

    /// a rendering mesh
    class RENDERING_MESH_API Mesh : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Mesh, base::res::IResource);

    public:
        Mesh();
        Mesh(MeshInitData && data);
        virtual ~Mesh();

        //---

        /// get mesh bounds that cover all the mesh vertices
        INLINE const base::Box& bounds() const { return m_bounds; }

        // get the content group this mesh belongs to
        INLINE MeshVisibilityGroup visibilityGroup() const { return m_visibilityGroup; }

        /// get the visibility radius multiplier for this mesh
        INLINE float visibilityDistanceMultiplier() const { return m_visibilityDistanceMultiplier; }

        /// get the custom forced visibility for this mesh
        INLINE float visibilityDistanceOverride() const { return m_visibilityDistanceOverride; }

        //---

        // get used materials
        INLINE const base::Array<MeshMaterial>& materials() const { return m_materials; }

        // get used bones
        //INLINE const base::Array<base::StringID>& boneRefs() const { return m_materialRefs; }

        // raw data chunk
        INLINE const base::Array<MeshChunk>& chunks() const { return m_chunks; }

        // detail levels
        INLINE const base::Array<MeshDetailLevel>& detailLevels() const { return m_details; }

        //---

    protected:
        base::Box m_bounds;

        base::Array<MeshMaterial> m_materials;
        base::Array<MeshChunk> m_chunks;
        base::Array<MeshDetailLevel> m_details;

        MeshVisibilityGroup m_visibilityGroup = MeshVisibilityGroup::Generic;
        float m_visibilityDistanceMultiplier = 1.0f;
        float m_visibilityDistanceOverride = 0.0f;

        void registerChunks();
        void unregisterChunks();

        // IObject
        virtual void onPostLoad() override;
    };

    ///---

} // rendering