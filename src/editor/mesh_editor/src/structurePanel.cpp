/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "structureNodes.h"

#include "structurePanel.h"
#include "engine/ui/include/uiTreeView.h"
#include "engine/mesh/include/mesh.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_CLASS(MeshStructurePanel);
RTTI_END_TYPE();

MeshStructurePanel::MeshStructurePanel()
{
    layoutVertical();

    m_tree = createChild<ui::TreeViewEx>();
    m_tree->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_tree->customVerticalAligment(ui::ElementVerticalLayout::Expand);

    m_tree->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
    {
        updateSelection();
        call(ui::EVENT_ITEM_SELECTION_CHANGED);
    };
}

MeshStructurePanel::~MeshStructurePanel()
{}

void MeshStructurePanel::collect(Array<const IMeshStructureNode*>& outStack) const
{
    auto cur = m_tree->current<IMeshStructureNode>();
    while (cur)
    {
        outStack.pushBack(cur);
        cur = rtti_cast<IMeshStructureNode>(cur->parent().lock());
    }

    std::reverse(outStack.begin(), outStack.end());
}

void MeshStructurePanel::bindResource(const MeshPtr& meshPtr)
{
    m_mesh = meshPtr;
    m_tree->clear();

    if (m_mesh)
        CreateStructure(m_tree, m_mesh);

    updateSelection();
}

void MeshStructurePanel::updateSelection()
{

}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
