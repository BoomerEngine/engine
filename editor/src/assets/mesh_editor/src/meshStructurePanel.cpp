/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "meshStructurePanel.h"
#include "base/ui/include/uiTreeView.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshStructureNode);
    RTTI_END_TYPE();

    MeshStructureNode::MeshStructureNode(base::StringView<char> txt, base::StringID type, Data data)
        : m_caption(txt)
        , m_type(type)
        , m_data(data)
    {}

    MeshStructureNode::~MeshStructureNode()
    {}

    //--

    MeshStructureTreeModel::MeshStructureTreeModel()
    {}

    MeshStructureTreeModel::~MeshStructureTreeModel()
    {}

    bool MeshStructureTreeModel::compare(const MeshStructureNodePtr& a, const MeshStructureNodePtr& b, int colIndex) const
    {
        return a->caption() < b->caption();
    }

    bool MeshStructureTreeModel::filter(const MeshStructureNodePtr& data, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
    {
        return filter.testString(data->caption());
    }

    base::StringBuf MeshStructureTreeModel::displayContent(const MeshStructureNodePtr& data, int colIndex /*= 0*/) const
    {
        return data->caption();
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(MeshStructurePanel);
    RTTI_END_TYPE();

    MeshStructurePanel::MeshStructurePanel()
    {
        layoutVertical();

        m_tree = createChild<ui::TreeView>();
        m_tree->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_tree->customVerticalAligment(ui::ElementVerticalLayout::Expand);
    }

    MeshStructurePanel::~MeshStructurePanel()
    {}

    void MeshStructurePanel::setMesh(const rendering::MeshPtr& meshPtr)
    {
        m_mesh = meshPtr;
        m_treeModel = base::CreateSharedPtr<MeshStructureTreeModel>();

        {
            auto boundsNode = base::CreateSharedPtr<MeshStructureNode>("[img:wireframe] Bounds", "Bounds"_id);
            m_treeModel->addRootNode(boundsNode);
        }

        {
            auto materials = base::CreateSharedPtr<MeshStructureNode>("[img:color_swatch] Materials", "Materials"_id);
            auto id = m_treeModel->addRootNode(materials);

            for (uint32_t i=0; i<meshPtr->materials().size(); i++)
            {
                const auto& mat = meshPtr->materials().typedData()[i];

                auto matNode = base::CreateSharedPtr<MeshStructureNode>(base::TempString("[img:material] {}", mat.name), "Material"_id, i);
                m_treeModel->addChildNode(id, matNode);

            }
        }

        for (uint32_t detailIndex = 0; detailIndex<meshPtr->detailLevels().size(); ++detailIndex)
        {
            uint32_t numChunks = 0;
            for (const auto& chunk : meshPtr->chunks())
                if (chunk.detailMask & (1U << detailIndex))
                    numChunks += 1;

            const auto& detail = meshPtr->detailLevels()[detailIndex];
            auto caption = base::StringBuf(base::TempString("[img:house] LOD {}-{} ({} chunks)", (int)detail.rangeMin, (int)detail.rangeMax, numChunks));
            auto geometry = base::CreateSharedPtr<MeshStructureNode>(caption, "LOD"_id, detailIndex);
            auto id = m_treeModel->addRootNode(geometry);

            for (const auto& chunk : meshPtr->chunks())
            {
                // only chunks belonging to that detail level
                if (0 == (chunk.detailMask & (1U << detailIndex)))
                    continue;

                // material name
                base::StringView<char> materialName = "Unknown Material";
                if (chunk.materialIndex < meshPtr->materials().size())
                    materialName = meshPtr->materials()[chunk.materialIndex].name.view();

                // chunk node
                auto caption = base::StringBuf(base::TempString("[img:house] Chunk '{}' ({}f, {}v, {})", materialName, chunk.indexCount / 3, chunk.vertexCount, chunk.vertexFormat));
                auto chunkNode = base::CreateSharedPtr<MeshStructureNode>(base::TempString("[img:teapot] {}", caption), "Chunk"_id);
                auto id2 = m_treeModel->addChildNode(id, chunkNode);
                
                /*for (const auto& chunk : model.chunks)
                {
                    {
                        auto matName = meshPtr->materials()[chunk.materialIndex].name;
                        auto chunkMatNode = base::CreateSharedPtr<MeshStructureNode>(base::TempString("[img:material] {}", matName), "ChunkMaterial"_id);
                        m_treeModel->addChildNode(id3, chunkMatNode);
                    }

                    for (const auto& stream : chunk.streams)
                    {
                        auto chunkNode = base::CreateSharedPtr<MeshStructureNode>(base::TempString("[img:table] Stream {}", stream.type), "ChunkStream"_id);
                        auto id4 = m_treeModel->addChildNode(id3, chunkNode);
                    }
                }*/
            }
        }

        m_tree->model(m_treeModel);
    }

    //--
    
} // ed
