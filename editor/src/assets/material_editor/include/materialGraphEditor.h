/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/editor/include/singleResourceEditor.h"
#include "rendering/material/include/renderingMaterialInstance.h"

namespace ed
{

    class MaterialGraphEditorPanel;
    class MaterialPreviewPanel;
    class MaterialPreviewPanelWithToolbar;

    /// editor for meshes
    class ASSETS_MATERIAL_EDITOR_API MaterialGraphEditor : public SingleLoadedResourceEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphEditor, SingleLoadedResourceEditor);

    public:
        MaterialGraphEditor(ConfigGroup config, ManagedFile* file);
        virtual ~MaterialGraphEditor();

        //--

        INLINE MaterialPreviewPanelWithToolbar* previewPanel() const { return m_previewPanel; }
        INLINE MaterialGraphEditorPanel* graphEditor() const { return m_graphEditor; }
        INLINE rendering::MaterialGraph* graph() const { return m_graph; }

        //--

    private:
        base::RefPtr<rendering::MaterialGraph> m_graph;
        base::RefPtr<rendering::MaterialInstance> m_previewInstance;

        //---

        base::RefPtr<MaterialPreviewPanelWithToolbar> m_previewPanel;
        base::RefPtr<MaterialGraphEditorPanel> m_graphEditor;
        ui::GraphBlockPalettePtr m_graphPalette;

        ui::DataInspectorPtr m_properties;

        void createInterface();
        void handleChangedSelection();

        virtual void fillEditMenu(ui::MenuButtonContainer* menu) override;
        virtual bool initialize() override;
    };

} // ed
