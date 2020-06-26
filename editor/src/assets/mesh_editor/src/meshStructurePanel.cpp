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
#include "base/geometry/include/mesh.h"

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshStructureNode);
    RTTI_END_TYPE();

    MeshStructureNode::MeshStructureNode(base::StringView<char> txt, base::StringID type)
        : m_caption(txt)
        , m_type(type)
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

    void MeshStructurePanel::setMesh(const base::mesh::MeshPtr& meshPtr)
    {
        m_mesh = meshPtr;
        m_treeModel = base::CreateSharedPtr<MeshStructureTreeModel>();

        {
            auto materials = base::CreateSharedPtr<MeshStructureNode>("[img:color_swatch] Materials", "Materials"_id);
            auto id = m_treeModel->addRootNode(materials);

            for (const auto& mat : meshPtr->materials())
            {
                auto matNode = base::CreateSharedPtr<MeshStructureNode>(base::TempString("[img:material] {}", mat.name), "Material"_id);
                m_treeModel->addChildNode(id, matNode);
            }
        }

        {
            auto geometry = base::CreateSharedPtr<MeshStructureNode>("[img:house] Models", "Models"_id);
            auto id = m_treeModel->addRootNode(geometry);

            for (const auto& model : meshPtr->models())
            {
                auto modelNode = base::CreateSharedPtr<MeshStructureNode>(base::TempString("[img:teapot] {}", model.name), "Model"_id);
                auto id2 = m_treeModel->addChildNode(id, modelNode);

                {
                    auto boundsNode = base::CreateSharedPtr<MeshStructureNode>("[img:wireframe] Bounds", "Bounds"_id);
                    m_treeModel->addChildNode(id2, boundsNode);
                }

                for (const auto& chunk : model.chunks)
                {
                    auto chunkNode = base::CreateSharedPtr<MeshStructureNode>("[img:wireframe] Chunk", "Chunk"_id);
                    auto id3 = m_treeModel->addChildNode(id2, chunkNode);

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
                }
            }
        }

        {
            auto geometry = base::CreateSharedPtr<MeshStructureNode>("[img:node] Instances", "Instances"_id);
            auto id = m_treeModel->addRootNode(geometry);

            for (const auto& model : meshPtr->models())
            {
            }
        }

        m_tree->model(m_treeModel);
    }

    //--
    
} // ed
