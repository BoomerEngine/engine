/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "editor/assets/include/resourceEditor.h"
#include "engine/material/include/materialInstance.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

class MaterialGraphEditorPanel;
class MaterialParametersPanel;
class MaterialPreviewPanel;

/// editor for meshes
class EDITOR_MATERIAL_EDITOR_API MaterialGraphEditor : public ResourceEditor
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphEditor, ResourceEditor);

public:
    MaterialGraphEditor(const ResourceInfo& info);
    virtual ~MaterialGraphEditor();

    //--

    INLINE MaterialPreviewPanel* previewPanel() const { return m_previewPanel; }
    INLINE MaterialGraphEditorPanel* graphEditor() const { return m_graphEditor; }
    INLINE MaterialGraph* graph() const { return m_graph; }

    //--

private:
    GlobalEventTable m_events;

    RefPtr<MaterialGraph> m_graph;
    RefPtr<MaterialInstance> m_previewMaterial;

    //---

    RefPtr<MaterialPreviewPanel> m_previewPanel;
    RefPtr<MaterialGraphEditorPanel> m_graphEditor;
    RefPtr<MaterialParametersPanel> m_parametersPanel;
    ui::GraphBlockPalettePtr m_graphPalette;

    ui::DataInspectorPtr m_properties;

    void updatePreviewMaterial();
    void createInterface();
    void handleChangedSelection();

    virtual void handleGeneralCopy() override;
    virtual void handleGeneralCut() override;
    virtual void handleGeneralPaste() override;
    virtual void handleGeneralDelete() override;

    virtual bool checkGeneralCopy() const override;
    virtual bool checkGeneralCut() const override;
    virtual bool checkGeneralPaste() const override;
    virtual bool checkGeneralDelete() const override;
};

END_BOOMER_NAMESPACE_EX(ed)
