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

#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/editor/include/assetBrowser.h"
#include "base/editor/include/managedFileFormat.h"
#include "base/editor/include/managedFileNativeResource.h"

namespace ed
{

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialInstanceEditor);
    RTTI_END_TYPE();

    MaterialInstanceEditor::MaterialInstanceEditor(ManagedFileNativeResource* file)
        : ResourceEditorNativeFile(file, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::UndoRedo, ResourceEditorFeatureBit::Imported })
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

            m_previewPanel = tab->createChild<MaterialPreviewPanel>();
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

    bool MaterialInstanceEditor::initialize()
    {
        if (!TBaseClass::initialize())
            return false;

        m_previewPanel->bindMaterial(materialInstance());
        m_properties->bindData(materialInstance()->createDataView());
        return true;
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

        virtual base::RefPtr<ResourceEditor> createEditor(ManagedFile* file) const override
        {
            if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            {
                if (auto loadedGraph = base::rtti_cast<rendering::MaterialInstance>(nativeFile->loadContent()))
                {
                    auto ret = base::CreateSharedPtr<MaterialInstanceEditor>(nativeFile);
                    ret->bindResource(loadedGraph);
                    return ret;
                }
            }

            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialInstaceResourceEditorOpener);
    RTTI_END_TYPE();

    //---

} // ed