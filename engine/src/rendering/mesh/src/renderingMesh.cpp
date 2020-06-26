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
#include "rendering/material/include/renderingMaterialInstance.h"
#include "rendering/material/include/renderingMaterialTemplate.h"

namespace rendering
{

    ///---

    RTTI_BEGIN_TYPE_CLASS(MeshBounds);
        RTTI_PROPERTY(box);
    RTTI_END_TYPE();

    ///---

    RTTI_BEGIN_TYPE_CLASS(MeshMaterial);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(baseMaterial); // owned by mesh
        RTTI_PROPERTY(material);
    RTTI_END_TYPE();

    ///---

    RTTI_BEGIN_TYPE_CLASS(MeshChunk);
        RTTI_PROPERTY(vertexFormat);
        RTTI_PROPERTY(bounds);
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

    RTTI_BEGIN_TYPE_CLASS(Mesh);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4mesh");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Mesh");
        RTTI_METADATA(base::res::ResourceBakedOnlyMetadata);
        RTTI_METADATA(base::res::ResourceTagColorMetadata).color(0xed, 0x6b, 0x86);
        RTTI_PROPERTY(m_bounds);
        RTTI_PROPERTY(m_materials);
        RTTI_PROPERTY(m_chunks);
    RTTI_END_TYPE();

    //$pink - lavender: #cfb3cdff;

    ///----

    Mesh::Mesh()
    {}

    Mesh::Mesh(MeshBounds&& bounds, base::Array<MeshMaterial>&& materials, base::Array<MeshChunk>&& chunks)
        : m_materials(std::move(materials))
        , m_chunks(std::move(chunks))
        , m_bounds(std::move(bounds))
    {
        for (auto& material : m_materials)
        {
            DEBUG_CHECK_EX(material.baseMaterial->parent() == nullptr, "Material bound to another parent");
            material.baseMaterial->parent(this);
            DEBUG_CHECK_EX(material.material->parent() == nullptr, "Material bound to another parent");
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
        TRACE_INFO("Registering {} chunks for '{}'", m_chunks.size(), path());
        if (auto* meshChunkService = base::GetService<MeshService>())
        {
            for (auto& chunk : m_chunks)
                chunk.renderId = meshChunkService->registerChunkData(chunk);
        }
    }

    void Mesh::unregisterChunks()
    {
        TRACE_INFO("Unregistering {} chunks for '{}'", m_chunks.size(), path());

        if (auto* meshChunkService = base::GetService<MeshService>())
        {
            for (auto& chunk : m_chunks)
            {
                meshChunkService->unregisterChunkData(chunk.renderId);
                chunk.renderId = 0;
            }
        }
    }

    void Mesh::onPostLoad()
    {
        TBaseClass::onPostLoad();
        registerChunks();

        for (auto& mat : m_materials)
        {
            if (mat.baseMaterial)
            {
                auto templateMaterial = mat.baseMaterial->resolveTemplate();
                TRACE_INFO("Template: '{}'", templateMaterial ? templateMaterial->path().view() : "");
            }
        }
    }

    ///---

} // rendering