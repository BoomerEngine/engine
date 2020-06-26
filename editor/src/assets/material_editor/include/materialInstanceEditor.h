/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/editor/include/singleResourceEditor.h"

namespace ed
{

    class MaterialPreviewPanel;
    class MaterialPreviewPanelWithToolbar;

    /// editor for meshes
    class ASSETS_MATERIAL_EDITOR_API MaterialInstanceEditor : public SingleResourceEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstanceEditor, SingleResourceEditor);

    public:
        MaterialInstanceEditor(ConfigGroup config, ManagedFile* file);
        virtual ~MaterialInstanceEditor();

        //--

        INLINE MaterialPreviewPanelWithToolbar* previewPanel() const { return m_previewPanel; }

        //--

    private:
        base::RefPtr<rendering::MaterialInstance> m_instance;

        //---

        base::RefPtr<MaterialPreviewPanelWithToolbar> m_previewPanel;
        ui::DataInspectorPtr m_properties;

        void createInterface();

        virtual void fillEditMenu(ui::MenuButtonContainer* menu) override;
        virtual bool initialize() override;
        virtual bool saveFile(ManagedFile* file) override;
        virtual void collectModifiedFiles(AssetItemList& outList) const override;
    };

} // ed
