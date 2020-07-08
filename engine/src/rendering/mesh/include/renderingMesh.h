/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "rendering/driver/include/renderingParametersView.h"

#include "renderingMeshFormat.h"

namespace rendering
{
    //---

    /// mesh bounds
    struct RENDERING_MESH_API MeshBounds
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MeshBounds);

        base::Box box;
    };

    //---

    /// material data
    struct RENDERING_MESH_API MeshMaterial
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MeshMaterial);

        base::StringID name;
        MaterialInstancePtr baseMaterial; // owned by mesh
        MaterialInstancePtr material; // owned by mesh
    };

    //---

    /// renderable mesh chunk
    struct RENDERING_MESH_API MeshChunk
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MeshChunk);

        MeshChunkRenderID renderId = 0; // assigned on load

        MeshVertexFormat vertexFormat;

        base::Box bounds;
        
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

    /// a rendering mesh data
    class RENDERING_MESH_API Mesh : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Mesh, base::res::IResource);

    public:
        Mesh();
        Mesh(MeshBounds&& bounds, base::Array<MeshMaterial>&& materials, base::Array<MeshChunk>&& chunks);
        virtual ~Mesh();

        //---

        // bounding box
        INLINE const MeshBounds& bounds() const { return m_bounds; }

        // materials
        INLINE const base::Array<MeshMaterial>& materials() const { return m_materials; }

        // chunks
        INLINE const base::Array<MeshChunk>& chunks() const { return m_chunks; }

        //---

    protected:
        MeshBounds m_bounds;
        base::Array<MeshMaterial> m_materials;
        base::Array<MeshChunk> m_chunks;

        void registerChunks();
        void unregisterChunks();

        // IObject
        virtual void onPostLoad() override;
    };

    ///---

} // rendering