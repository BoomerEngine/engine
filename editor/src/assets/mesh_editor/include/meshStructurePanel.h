/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once
#include "base/ui/include/uiSimpleTreeModel.h"

namespace ed
{
    //--

    // mesh structure node
    class ASSETS_MESH_EDITOR_API MeshStructureNode : public base::IReferencable
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(MeshStructureNode);

    public:
        MeshStructureNode(base::StringView<char> txt, base::StringID type);
        virtual ~MeshStructureNode();

        INLINE const base::StringBuf& caption() const { return m_caption; }
        INLINE base::StringID type() const { return m_type; }

    private:
        base::StringBuf m_caption;
        base::StringID m_type;
    };

    typedef base::RefPtr<MeshStructureNode> MeshStructureNodePtr;

    //--

    // model for a mesh structure tree
    class ASSETS_MESH_EDITOR_API MeshStructureTreeModel : public ui::SimpleTreeModel<MeshStructureNodePtr>
    {
    public:
        MeshStructureTreeModel();
        virtual ~MeshStructureTreeModel();

        virtual bool compare(const MeshStructureNodePtr& a, const MeshStructureNodePtr& b, int colIndex) const override final;
        virtual bool filter(const MeshStructureNodePtr& data, const ui::SearchPattern& filter, int colIndex = 0) const override final;
        virtual base::StringBuf displayContent(const MeshStructureNodePtr& data, int colIndex = 0) const override final;
    };

    //--

    // a preview panel for an image
    class ASSETS_MESH_EDITOR_API MeshStructurePanel : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshStructurePanel, ui::IElement);

    public:
        MeshStructurePanel();
        virtual ~MeshStructurePanel();

        void setMesh(const base::mesh::MeshPtr& mesh);

    private:
        base::mesh::MeshPtr m_mesh;

        ui::TreeViewPtr m_tree;
        base::RefPtr<MeshStructureTreeModel> m_treeModel;
    };

    //--

} // ed