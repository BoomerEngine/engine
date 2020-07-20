/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/editor/include/resourceEditorNativeFile.h"

namespace ed
{

    class MaterialPreviewPanel;
    class MaterialPreviewPanelWithToolbar;

    /// editor for meshes
    class ASSETS_MATERIAL_EDITOR_API MaterialInstanceEditor : public ResourceEditorNativeFile
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstanceEditor, ResourceEditorNativeFile);

    public:
        MaterialInstanceEditor(ConfigGroup config, ManagedFileNativeResource* file);
        virtual ~MaterialInstanceEditor();

        //--

        INLINE MaterialPreviewPanelWithToolbar* previewPanel() const { return m_previewPanel; }

        INLINE rendering::MaterialInstancePtr materialInstance() const { return base::rtti_cast<rendering::MaterialInstance>(resource()); }

        //--

    private:

        //---

        base::RefPtr<MaterialPreviewPanelWithToolbar> m_previewPanel;
        ui::DataInspectorPtr m_properties;

        void createInterface();

        virtual bool initialize() override;
    };

} // ed
