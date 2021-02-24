/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/ui/include/uiSimpleTreeModel.h"
#include "base/ui/include/uiElement.h"

BEGIN_BOOMER_NAMESPACE(ed)

//--

// mesh structure node
class EDITOR_MESH_EDITOR_API MeshStructureNode : public base::IReferencable
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(MeshStructureNode);

public:
    struct Data
    {
        uint32_t index = 0;
        uint32_t subIndex = 0;

        INLINE Data() {};
        INLINE Data(const Data& other) = default;
        INLINE Data& operator=(const Data& other) = default;

        INLINE Data(uint32_t index_)
            : index(index_)
        {}
    };

    MeshStructureNode(base::StringView txt, base::StringID type, Data data = Data());
    virtual ~MeshStructureNode();

    INLINE const base::StringBuf& caption() const { return m_caption; }
    INLINE base::StringID type() const { return m_type; }
    INLINE const Data& data() const { return m_data; }

private:
    base::StringBuf m_caption;
    base::StringID m_type;
    Data m_data;
};

typedef base::RefPtr<MeshStructureNode> MeshStructureNodePtr;

//--

// model for a mesh structure tree
class EDITOR_MESH_EDITOR_API MeshStructureTreeModel : public ui::SimpleTreeModel<MeshStructureNodePtr>
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
class EDITOR_MESH_EDITOR_API MeshStructurePanel : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshStructurePanel, ui::IElement);

public:
    MeshStructurePanel();
    virtual ~MeshStructurePanel();

    void bindResource(const rendering::MeshPtr& mesh);

private:
    rendering::MeshPtr m_mesh;

    ui::TreeViewPtr m_tree;
    base::RefPtr<MeshStructureTreeModel> m_treeModel;
};

//--

END_BOOMER_NAMESPACE(ed)