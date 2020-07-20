/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "meshEditor.h"
#include "meshPreviewPanel.h"
#include "meshStructurePanel.h"
#include "meshMaterialsPanel.h"

#include "base/editor/include/managedFile.h"
#include "base/editor/include/managedFileFormat.h"
#include "base/editor/include/managedFileNativeResource.h"
#include "base/ui/include/uiDockPanel.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiRuler.h"
#include "base/ui/include/uiSplitter.h"

#include "rendering/mesh/include/renderingMesh.h"
#include "base/ui/include/uiMenuBar.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshEditor);
    RTTI_END_TYPE();

    MeshEditor::MeshEditor(ConfigGroup config, ManagedFileNativeResource* file)
        : ResourceEditorNativeFile(config, file, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::UndoRedo, ResourceEditorFeatureBit::Imported })
    {
        createInterface();
    }

    MeshEditor::~MeshEditor()
    {}

    void MeshEditor::createInterface()
    {
        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:world] Preview");
            tab->layoutVertical();

            m_previewPanel = tab->createChild<MeshPreviewPanel>();
            m_previewPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_previewPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            dockLayout().attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:tree] Structure");
            tab->layoutVertical();

            m_structurePanel = tab->createChild<MeshStructurePanel>();
            m_structurePanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_structurePanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            dockLayout().right().attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:color] Materials");
            tab->layoutVertical();

            m_materialsPanel = tab->createChild<MeshMaterialsPanel>(m_previewPanel, actionHistory());
            m_materialsPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_materialsPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            dockLayout().right().attachPanel(tab, false);
        }

        {
            actions().bindCommand("MeshPreview.ShowBounds"_id) = [this]() {
                auto settings = m_previewPanel->previewSettings(); 
                settings.showBounds = !settings.showBounds; 
                m_previewPanel->previewSettings(settings);
            };
            actions().bindToggle("MeshPreview.ShowBounds"_id) = [this]() {
                return m_previewPanel->previewSettings().showBounds;
            };
        }
    }

    void MeshEditor::fillViewMenu(ui::MenuButtonContainer* menu)
    {
        TBaseClass::fillViewMenu(menu);
        menu->createAction("MeshPreview.ShowBounds"_id, "Show bounds", "cube");
    }

    void MeshEditor::bindResource(const res::ResourcePtr& resource)
    {
        TBaseClass::bindResource(resource);

        if (m_structurePanel)
            m_structurePanel->bindResource(mesh());

        if (m_previewPanel)
            m_previewPanel->previewMesh(mesh());
    }

    //---

    class MeshResourceEditorOpener : public IResourceEditorOpener
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshResourceEditorOpener, IResourceEditorOpener);

    public:
        virtual bool canOpen(const ManagedFileFormat& format) const override
        {
            return format.nativeResourceClass() == rendering::Mesh::GetStaticClass();
        }

        virtual base::RefPtr<ResourceEditor> createEditor(ConfigGroup config, ManagedFile* file) const override
        {
            if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            {
                if (auto mesh = base::rtti_cast<rendering::Mesh>(nativeFile->loadContent()))
                {
                    auto ret = base::CreateSharedPtr<MeshEditor>(config, nativeFile);
                    ret->bindResource(mesh);
                    return ret;
                }
            }

            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MeshResourceEditorOpener);
    RTTI_END_TYPE();

    //---

} // ed