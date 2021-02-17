/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "editor/common/include/resourceEditor.h"
#include "editor/common/include/resourceEditorNativeFile.h"
#include "rendering/material/include/renderingMaterialInstance.h"

namespace ed
{

    class MaterialGraphEditorPanel;
    class MaterialPreviewPanel;

    /// editor for meshes
    class EDITOR_MATERIAL_EDITOR_API MaterialGraphEditor : public ResourceEditorNativeFile
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphEditor, ResourceEditorNativeFile);

    public:
        MaterialGraphEditor(ManagedFileNativeResource* file);
        virtual ~MaterialGraphEditor();

        //--

        INLINE MaterialPreviewPanel* previewPanel() const { return m_previewPanel; }
        INLINE MaterialGraphEditorPanel* graphEditor() const { return m_graphEditor; }
        INLINE rendering::MaterialGraph* graph() const { return m_graph; }

        //--

    private:
        base::RefPtr<rendering::MaterialGraph> m_graph;
        base::RefPtr<rendering::MaterialInstance> m_previewMaterial;

        //---

        base::RefPtr<MaterialPreviewPanel> m_previewPanel;
        base::RefPtr<MaterialGraphEditorPanel> m_graphEditor;
        ui::GraphBlockPalettePtr m_graphPalette;

        ui::DataInspectorPtr m_properties;

        void updatePreviewMaterial();
        void createInterface();
        void handleChangedSelection();

        virtual void handleContentModified() override;
        virtual void handleGeneralCopy() override;
        virtual void handleGeneralCut() override;
        virtual void handleGeneralPaste() override;
        virtual void handleGeneralDelete() override;

        virtual bool checkGeneralCopy() const override;
        virtual bool checkGeneralCut() const override;
        virtual bool checkGeneralPaste() const override;
        virtual bool checkGeneralDelete() const override;

        virtual bool initialize() override;
        virtual bool save() override;
    };

} // ed
