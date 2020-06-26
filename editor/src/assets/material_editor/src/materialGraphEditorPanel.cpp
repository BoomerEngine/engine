/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "materialGraphEditorPanel.h"
#include "base/ui/include/uiScintillaTextEditor.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiGraphEditor.h"
#include "base/ui/include/uiGraphEditorNode.h"

#include "rendering/material_graph/include/renderingMaterialGraph.h"
#include "rendering/material_graph/include/renderingMaterialGraphBlock.h"

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialGraphInnerEditorPanel);
    RTTI_END_TYPE();

    MaterialGraphInnerEditorPanel::MaterialGraphInnerEditorPanel()
    {}

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialGraphEditorPanel);
    RTTI_END_TYPE();

    MaterialGraphEditorPanel::MaterialGraphEditorPanel(const base::ActionHistoryPtr& actions)
        : m_hasValidSelection(false)
    {
        layoutVertical();

        m_graphEditor = createChild<ui::GraphEditor>();
        m_graphEditor->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_graphEditor->customVerticalAligment(ui::ElementVerticalLayout::Expand);
        m_graphEditor->bindActionHistory(actions);

        m_graphEditor->bind("OnSelectionChanged"_id) = [this]()
        {
            m_selectedBlocks.reset();

            m_graphEditor->enumSelectedElements([this](ui::VirtualAreaElement* elem)
                {
                    if (auto* node = base::rtti_cast<ui::GraphEditorBlockNode>(elem))
                        if (auto block = base::rtti_cast<rendering::MaterialGraphBlock>(node->block()))
                            m_selectedBlocks.pushBack(block);
                    return false;
                });

            m_hasValidSelection = !m_selectedBlocks.empty();

            call("OnSelectionChanged"_id);
        };
    }

    MaterialGraphEditorPanel::~MaterialGraphEditorPanel()
    {}

    void MaterialGraphEditorPanel::bindGraph(const rendering::MaterialGraphPtr& graph)
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

    //--
    
} // ed
