/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "structurePanel.h"
#include "structureNodes.h"
#include "previewPanel.h"

#include "engine/mesh/include/mesh.h"
#include "engine/material/include/material.h"
#include "engine/material/include/materialInstance.h"
#include "engine/material/include/materialTemplate.h"
#include "engine/texture/include/texture.h"
#include "engine/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(IMeshStructureNode);
RTTI_END_TYPE();

IMeshStructureNode::IMeshStructureNode(Mesh* mesh, StringView text)
    : m_mesh(AddRef(mesh))
{
    createChild<ui::TextLabel>(text);
}

IMeshStructureNode::~IMeshStructureNode()
{}

void IMeshStructureNode::handleItemExpand()
{
}

void IMeshStructureNode::handleItemCollapse()
{
    removeAllChildren();
}

void IMeshStructureNode::setup(MeshPreviewPanelSettings& settings, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const
{

}

void IMeshStructureNode::render(rendering::FrameParams& frame, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const
{

}

//--

class MeshStructore_MaterialShader : public IMeshStructureNode
{
public:
    MeshStructore_MaterialShader(Mesh* mesh, const MaterialTemplate* temp)
        : IMeshStructureNode(mesh, TempString("[img:shader] Template: {}", temp->loadPath().view().fileStem()))
        , m_template(AddRef(temp))
    {}

private:
    MaterialTemplatePtr m_template;
};

class MeshStructore_MaterialBase : public IMeshStructureNode
{
public:
    MeshStructore_MaterialBase(Mesh* mesh, const MaterialRef& base)
        : IMeshStructureNode(mesh, TempString("[img:file] Base: {}", base.resource()->loadPath().view().fileStem()))
        , m_base(base)
    {}

private:
    MaterialRef m_base;
};

class MeshStructore_MaterialTexture : public IMeshStructureNode
{
public:
    MeshStructore_MaterialTexture(Mesh* mesh, StringID binding, const ITexture* tex)
        : IMeshStructureNode(mesh, TempString("[img:image] {}: {}", binding, tex ? tex->loadPath().view().fileStem() : "<none>"))
        , m_texture(AddRef(tex))
    {}

private:
    StringID m_name;
    TexturePtr m_texture;
};

class MeshStructore_MaterialNode : public IMeshStructureNode
{
public:
    MeshStructore_MaterialNode(Mesh* mesh, StringID name, MaterialInstancePtr data)
        : IMeshStructureNode(mesh, TempString("[img:material] {}", name))
        , m_name(name)
        , m_data(data)
    {}

    virtual bool handleItemCanExpand() const override
    {
        return true;
    }

    virtual void handleItemExpand() override
    {
        if (m_data)
        {
            if (auto shader = m_data->resolveTemplate())
            {
                auto node = RefNew<MeshStructore_MaterialShader>(m_mesh, shader);
                addChild(node);
            }

            if (auto base = m_data->baseMaterial())
            {
                if (base.resource())
                {
                    auto node = RefNew<MeshStructore_MaterialBase>(m_mesh, base);
                    addChild(node);
                }
            }


        }
    }

private:
    StringID m_name;
    MaterialInstancePtr m_data;
};

class MeshStructure_MaterialsRoot : public IMeshStructureNode
{
public:
    MeshStructure_MaterialsRoot(Mesh* mesh)
        : IMeshStructureNode(mesh, "[img:color_swatch] Materials")
    {}

    virtual bool handleItemCanExpand() const override
    {
        return !m_mesh->materials().empty();
    }

    virtual void handleItemExpand() override
    {
        auto materials = m_mesh->materials();
        std::sort(materials.begin(), materials.end(), [](const auto& a, const auto& b) { return a.name.view() < b.name.view(); });

        for (const auto& mat : materials)
        {
            auto node = RefNew<MeshStructore_MaterialNode>(m_mesh, mat.name, mat.material);
            addChild(node);
        }
    }
};

//--

class MeshStructure_BoxBounds : public IMeshStructureNode
{
public:
    MeshStructure_BoxBounds(Mesh* mesh)
        : IMeshStructureNode(mesh, "[img:wireframe] Bounds")
    {}

    virtual void render(rendering::FrameParams& frame, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const override
    {

    }
};

//--

class MeshStructure_GeometryChunk : public IMeshStructureNode
{
public:
    MeshStructure_GeometryChunk(Mesh* mesh, uint32_t index)
        : IMeshStructureNode(mesh, TempString("[img:cylinder] Chunk {}", index))
        , m_index(index)
    {}

    virtual void setup(MeshPreviewPanelSettings& settings, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const override
    {
        settings.forceChunk = m_index;
    }

    virtual void render(rendering::FrameParams& frame, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const override
    {

    }

    virtual bool handleItemCanExpand() const override
    {
        return true;
    }

    virtual void handleItemExpand() override
    {
        if (m_index < m_mesh->chunks().size())
        {
            const auto& chunk = m_mesh->chunks()[m_index];

            if (chunk.materialIndex >= 0 && chunk.materialIndex < m_mesh->materials().size())
            {
                const auto& mat = m_mesh->materials()[chunk.materialIndex];

                auto node = RefNew<MeshStructore_MaterialNode>(m_mesh, mat.name, mat.material);
                addChild(node);
            }
        }
    }

private:
    uint32_t m_index;
};

class MeshStructure_GeometryDetail : public IMeshStructureNode
{
public:
    MeshStructure_GeometryDetail(Mesh* mesh, uint32_t index)
        : IMeshStructureNode(mesh, TempString("[img:shapes] LOD {}", index))
        , m_index(index)
    {}

    virtual void setup(MeshPreviewPanelSettings& settings, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const override
    {
        settings.forceLod = m_index;
    }

    virtual void render(rendering::FrameParams& frame, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const override
    {

    }

    virtual bool handleItemCanExpand() const override
    {
        return true;
    }

    virtual void handleItemExpand() override
    {
        const auto mask = 1U << m_index;

        for (auto i : m_mesh->chunks().indexRange())
        {
            const auto& chunk = m_mesh->chunks()[i];
            if (chunk.detailMask & mask)
            {
                auto node = RefNew<MeshStructure_GeometryChunk>(m_mesh, i);
                addChild(node);
            }
        }
    }

private:
    uint32_t m_index;
};

//--

void CreateStructure(ui::TreeViewEx* view, Mesh* mesh)
{
    {
        auto node = RefNew<MeshStructure_BoxBounds>(mesh);
        view->addRoot(node);
    }

    {
        auto node = RefNew<MeshStructure_MaterialsRoot>(mesh);
        view->addRoot(node);
    }

    for (auto i : mesh->detailLevels().indexRange())
    {
        auto node = RefNew<MeshStructure_GeometryDetail>(mesh, i);
        view->addRoot(node);
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)
