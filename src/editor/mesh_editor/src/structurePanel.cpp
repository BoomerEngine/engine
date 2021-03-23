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

#include "engine/ui/include/uiTreeViewEx.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiSplitter.h"
#include "engine/mesh/include/mesh.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshStructurePanel);
RTTI_END_TYPE();

MeshStructurePanel::MeshStructurePanel(ActionHistory* ah)
    : m_actionHistory(ah)
{
    layoutVertical();

    {
        auto split = createChild<ui::Splitter>(ui::Direction::Horizontal, 0.5f);

        m_tree = split->createChild<ui::TreeViewEx>();
        m_tree->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_tree->customVerticalAligment(ui::ElementVerticalLayout::Expand);

        auto area = split->createChild<ui::ScrollArea>(ui::ScrollMode::Always);
        area->expand();

        m_details = area->createChild<ui::TextLabel>();
        m_details->customPadding(5, 5, 5, 5);

        /*m_properties = split->createChild<ui::DataInspector>();
        m_properties->bindActionHistory(m_actionHistory);*/
    }

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
    StringBuilder txt;

    if (auto selection = m_tree->current<IMeshStructureNode>())
        selection->printDetails(txt);

    m_details->text(txt.view());
}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
