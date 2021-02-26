/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiGraphEditor.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

DECLARE_UI_EVENT(EVENT_MATERIAL_BLOCK_SELECTION_CHANGED);

//--

// inner material graph panel
class EDITOR_MATERIAL_EDITOR_API MaterialGraphInnerEditorPanel : public ui::GraphEditor
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphInnerEditorPanel, ui::GraphEditor);

public:
    MaterialGraphInnerEditorPanel();

};

//--

// material specific graph editor
class EDITOR_MATERIAL_EDITOR_API MaterialGraphEditorPanel : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphEditorPanel, ui::IElement);

public:
    MaterialGraphEditorPanel(const ActionHistoryPtr& actions);
    virtual ~MaterialGraphEditorPanel();

    // get current selection
    INLINE const Array<RefPtr<MaterialGraphBlock>>& selectedBlocks() const { return m_selectedBlocks; }

    // set graph
    void bindGraph(const MaterialGraphPtr& graph);

    // actions
    void actionCopySelection();
    void actionCutSelection();
    void actionPasteSelection();
    void actionDeleteSelection();
    bool hasDataToPaste() const;
    bool hasSelection() const;

private:
    RefPtr<MaterialGraphInnerEditorPanel> m_graphEditor;
    Array<RefPtr<MaterialGraphBlock>> m_selectedBlocks;

    MaterialGraphPtr m_graph;
    bool m_hasValidSelection = false;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
