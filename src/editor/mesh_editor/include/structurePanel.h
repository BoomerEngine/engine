/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiTreeViewEx.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

class IMeshStructureNode;
typedef RefPtr<IMeshStructureNode> MeshStructureNodePtr;

struct MaterialPreviewPanelSettings;

//--

// a preview panel for an image
class EDITOR_MESH_EDITOR_API MeshStructurePanel : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshStructurePanel, ui::IElement);

public:
    MeshStructurePanel(ActionHistory* ah);
    virtual ~MeshStructurePanel();

    void bindResource(const MeshPtr& mesh);

    void collect(Array<const IMeshStructureNode*>& outStack) const;

private:
    MeshPtr m_mesh;

    ActionHistory* m_actionHistory;

    ui::TreeViewExPtr m_tree;
    ui::TextLabelPtr m_details;

    MeshStructureNodePtr m_root;

    Array<MeshStructureNodePtr> m_nodeStack;
    MeshStructureNodePtr m_top;

    void updateSelection();
};

//--

END_BOOMER_NAMESPACE_EX(ed)
