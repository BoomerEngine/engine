/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#include "build.h"
#include "renderingMesh.h"
#include "renderingMeshService.h"
#include "rendering/material/include/renderingMaterial.h"
#include "rendering/material/include/renderingMaterialInstance.h"
#include "base/resource/include/resourceTags.h"

BEGIN_BOOMER_NAMESPACE(rendering)

    ///---

    RTTI_BEGIN_TYPE_CLASS(MeshChunk);
        RTTI_PROPERTY(vertexFormat);
        RTTI_PROPERTY(materialIndex);
        RTTI_PROPERTY(renderMask);
        RTTI_PROPERTY(detailMask);
        RTTI_PROPERTY(indexCount);
        RTTI_PROPERTY(vertexCount);
        RTTI_PROPERTY(unpackedVertexSize);
        RTTI_PROPERTY(unpackedIndexSize);
        RTTI_PROPERTY(quantizationOffset);
        RTTI_PROPERTY(quantizationScale);
        RTTI_PROPERTY(packedVertexData);
        RTTI_PROPERTY(packedIndexData);
    RTTI_END_TYPE();

    ///---

    RTTI_BEGIN_TYPE_CLASS(MeshDetailLevel);
        RTTI_PROPERTY(rangeMin);
        RTTI_PROPERTY(rangeMax);
    RTTI_END_TYPE();

    ///---


    RTTI_BEGIN_TYPE_CLASS(MeshMaterial);
    RTTI_PROPERTY(name);
    //RTTI_PROPERTY(baseMaterial); // imported resource
    RTTI_PROPERTY(material); // owned by mesh
    RTTI_END_TYPE();

    ///---

    RTTI_BEGIN_TYPE_CLASS(Mesh);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4mesh");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Mesh");
        RTTI_METADATA(base::res::ResourceDataVersionMetadata).version(11);
        RTTI_METADATA(base::res::ResourceTagColorMetadata).color(0xed, 0x6b, 0x86);
        RTTI_PROPERTY(m_bounds);
        RTTI_PROPERTY(m_materials);
        RTTI_PROPERTY(m_details);
        RTTI_PROPERTY(m_chunks);
        RTTI_CATEGORY("Visibility");
        //RTTI_PROPERTY(m_visibilityGroup).editable("Predefined visibility group this mesh belongs to");
        RTTI_PROPERTY(m_visibilityDistanceMultiplier).editable("Visibility distance multiplier for this mesh").range(0.1f, 3.0f);
        RTTI_PROPERTY(m_visibilityDistanceOverride).editable("Manual visibility distance override (DO NOT USE)");
    RTTI_END_TYPE();

    ///----

    Mesh::Mesh()
    {}

    Mesh::Mesh(MeshInitData&& data)
        : m_chunks(std::move(data.chunks))
        , m_materials(std::move(data.materials))
        , m_details(std::move(data.detailLevels))
        , m_bounds(data.bounds)
    {
        if (m_bounds.empty())
            m_bounds = base::Box(base::Vector3::ZERO(), 0.1f);

        for (auto& material : m_materials)
        {
            DEBUG_CHECK(material.material);

            if (!material.material)
                material.material = base::RefNew<MaterialInstance>();

            DEBUG_CHECK(material.material->parent() == nullptr);
            material.material->parent(this);
        }

        registerChunks();
    }
    
    Mesh::~Mesh()
    {
        unregisterChunks();
    }

    void Mesh::registerChunks()
    {
        if (auto* meshChunkService = base::GetService<MeshService>())
        {
            for (auto& chunk : m_chunks)
                chunk.proxy = meshChunkService->createChunkProxy(chunk);
        }
    }

    void Mesh::unregisterChunks()
    {
        for (auto& chunk : m_chunks)
			chunk.proxy.reset();
    }

    void Mesh::onPostLoad()
    {
        TBaseClass::onPostLoad();
        registerChunks();
    }

    ///---

} // rendering