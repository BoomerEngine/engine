/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiTreeViewEx.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

struct MeshPreviewPanelSettings;

// mesh structure node
class EDITOR_MESH_EDITOR_API IMeshStructureNode : public ui::ITreeItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(IMeshStructureNode, ui::ITreeItem);

public:
    IMeshStructureNode(Mesh* mesh, StringView text);
    virtual ~IMeshStructureNode();

    virtual void setup(MeshPreviewPanelSettings& settings, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const;
    virtual void render(rendering::FrameParams& frame, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const;

    virtual void handleItemExpand() override;
    virtual void handleItemCollapse() override;

protected:
    MeshPtr m_mesh;
};

typedef RefPtr<IMeshStructureNode> MeshStructureNodePtr;

//--

extern void CreateStructure(ui::TreeViewEx* view, Mesh* mesh);

//--

END_BOOMER_NAMESPACE_EX(ed)
