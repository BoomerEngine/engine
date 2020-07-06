/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "materialInstanceEditor.h"
#include "materialPreviewPanel.h"

#include "rendering/material/include/renderingMaterialInstance.h"
#include "base/editor/include/singleResourceEditor.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/editor/include/assetBrowser.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/editor/include/managedFileFormat.h"

namespace ed
{

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialInstanceEditor);
    RTTI_END_TYPE();

    MaterialInstanceEditor::MaterialInstanceEditor(ConfigGroup config, ManagedFile* file)
        : SingleResourceEditor(config, file)
    {
        createInterface();
    }

    MaterialInstanceEditor::~MaterialInstanceEditor()
    {}

    void MaterialInstanceEditor::createInterface()
    {
        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:shader] Preview", "PreviewPanel");
            tab->layoutVertical();

            m_previewPanel = tab->createChild<MaterialPreviewPanelWithToolbar>();
            m_previewPanel->expand();

            dockLayout().attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:properties] Properties", "PropertiesPanel");
            tab->layoutVertical();

            m_properties = tab->createChild<ui::DataInspector>();
            m_properties->bindActionHistory(actionHistory());
            m_properties->expand();

            dockLayout().right(0.2f).attachPanel(tab);
        }        
    }

    void MaterialInstanceEditor::fillEditMenu(ui::MenuButtonContainer* menu)
    {
        TBaseClass::fillEditMenu(menu);
    }

    bool MaterialInstanceEditor::initialize()
    {
        if (!TBaseClass::initialize())
            return false;

        m_instance = file()->loadContent<rendering::MaterialInstance>();
        if (!m_instance)
            return false;

        m_instance->resetModifiedFlag();
        m_previewPanel->bindMaterial(m_instance);
        m_properties->bindData(m_instance->createDataView());
        return true;
    }

    bool MaterialInstanceEditor::saveFile(ManagedFile* fileToSave)
    {
        if (fileToSave == file())
        {
            if (file()->storeContent(m_instance))
            {
                m_instance->resetModifiedFlag();
                return true;
            }
        }

        return false;
    }

    void MaterialInstanceEditor::collectModifiedFiles(AssetItemList& outList) const
    {
        if (m_instance->modified())
            outList.collectFile(file());
    }

    //---

    class MaterialInstaceResourceEditorOpener : public IResourceEditorOpener
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstaceResourceEditorOpener, IResourceEditorOpener);

    public:
        virtual bool canOpen(const ManagedFileFormat& format) const override
        {
            const auto graphClass = rendering::MaterialInstance::GetStaticClass();
            return (format.nativeResourceClass() == graphClass);
        }

        virtual base::RefPtr<ResourceEditor> createEditor(ConfigGroup config, ManagedFile* file) const override
        {
            return base::CreateSharedPtr<MaterialInstanceEditor>(config, file);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialInstaceResourceEditorOpener);
    RTTI_END_TYPE();

    //---

} // ed