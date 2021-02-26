/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "materialGraphEditorPanel.h"
#include "engine/ui/include/uiScintillaTextEditor.h"
#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiGraphEditor.h"
#include "engine/ui/include/uiGraphEditorNode.h"

#include "engine/material_graph/include/renderingMaterialGraph.h"
#include "engine/material_graph/include/renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialGraphInnerEditorPanel);
RTTI_END_TYPE();

MaterialGraphInnerEditorPanel::MaterialGraphInnerEditorPanel()
{}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialGraphEditorPanel);
RTTI_END_TYPE();

MaterialGraphEditorPanel::MaterialGraphEditorPanel(const ActionHistoryPtr& actions)
    : m_hasValidSelection(false)
{
    layoutVertical();

    m_graphEditor = createChild<ui::GraphEditor>();
    m_graphEditor->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_graphEditor->customVerticalAligment(ui::ElementVerticalLayout::Expand);
    m_graphEditor->bindActionHistory(actions);

    m_graphEditor->bind(ui::EVENT_VIRTUAL_AREA_SELECTION_CHANGED) = [this]()
    {
        m_selectedBlocks.reset();

        m_graphEditor->enumSelectedElements([this](ui::VirtualAreaElement* elem)
            {
                if (auto* node = rtti_cast<ui::GraphEditorBlockNode>(elem))
                    if (auto block = rtti_cast<MaterialGraphBlock>(node->block()))
                        m_selectedBlocks.pushBack(block);
                return false;
            });

        m_hasValidSelection = !m_selectedBlocks.empty();

        call(EVENT_MATERIAL_BLOCK_SELECTION_CHANGED);
    };
}

MaterialGraphEditorPanel::~MaterialGraphEditorPanel()
{}

void MaterialGraphEditorPanel::bindGraph(const MaterialGraphPtr& graph)
{
    m_graph = graph;

    auto innerContainer = graph ? graph->graph() : nullptr;
    m_graphEditor->bindGraph(innerContainer);

    if (graph)
        m_graphEditor->zoomToFit();
}

void MaterialGraphEditorPanel::actionCopySelection()
{
    m_graphEditor->actionCopySelection();
}

void MaterialGraphEditorPanel::actionCutSelection()
{
    m_graphEditor->actionCutSelection();
}

void MaterialGraphEditorPanel::actionPasteSelection()
{
    m_graphEditor->actionPasteSelection(false, ui::VirtualPosition(0,0));
}

void MaterialGraphEditorPanel::actionDeleteSelection()
{
    m_graphEditor->actionDeleteSelection();
}

bool MaterialGraphEditorPanel::hasDataToPaste() const
{
    return true;
}

bool MaterialGraphEditorPanel::hasSelection() const
{
    return m_hasValidSelection;
}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
