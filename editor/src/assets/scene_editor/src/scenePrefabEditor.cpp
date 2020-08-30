/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "scenePrefabEditor.h"
#include "sceneStructurePanel.h"

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

#include "base/ui/include/uiMenuBar.h"
#include "assets/scene_common/include/editorScenePreviewContainer.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePrefabEditor);
    RTTI_END_TYPE();

    ScenePrefabEditor::ScenePrefabEditor(ManagedFileNativeResource* file)
        : ResourceEditorNativeFile(file, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::UndoRedo })
    {
        createInterface();
    }

    ScenePrefabEditor::~ScenePrefabEditor()
    {}

    void ScenePrefabEditor::createInterface()
    {
        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:world] Preview");
            tab->layoutVertical();

            m_previewContainer = tab->createChild<EditorScenePreviewContainer>(false);
            m_previewContainer->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_previewContainer->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            dockLayout().attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:tree] Structure");
            tab->layoutVertical();

            m_structurePanel = tab->createChild<SceneStructurePanel>();
            m_structurePanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_structurePanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            dockLayout().right().attachPanel(tab);
        }

        /*{
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:color] Materials");
            tab->layoutVertical();

            m_materialsPanel = tab->createChild<SceneMaterialsPanel>(actionHistory());
            m_materialsPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_materialsPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            m_materialsPanel->bind(EVENT_MATERIAL_SELECTION_CHANGED, this) = [this]()
            {
                updateMaterialHighlights();
            };

            dockLayout().right().attachPanel(tab, false);
        }*/

        {
        }
    }

    void ScenePrefabEditor::fillViewMenu(ui::MenuButtonContainer* menu)
    {
        TBaseClass::fillViewMenu(menu);
        menu->createAction("ScenePreview.ShowBounds"_id, "Show bounds", "cube");
    }

    void ScenePrefabEditor::bindResource(const res::ResourcePtr& resource)
    {
        TBaseClass::bindResource(resource);

    }

    //---

    class ScenePrefabResourceEditorOpener : public IResourceEditorOpener
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScenePrefabResourceEditorOpener, IResourceEditorOpener);

    public:
        virtual bool canOpen(const ManagedFileFormat& format) const override
        {
            return format.nativeResourceClass() == game::Prefab::GetStaticClass();
        }

        virtual base::RefPtr<ResourceEditor> createEditor(ManagedFile* file) const override
        {
            if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            {
                if (auto mesh = base::rtti_cast<game::Prefab>(nativeFile->loadContent()))
                {
                    auto ret = base::CreateSharedPtr<ScenePrefabEditor>(nativeFile);
                    ret->bindResource(mesh);
                    return ret;
                }
            }

            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(ScenePrefabResourceEditorOpener);
    RTTI_END_TYPE();

    //---

} // ed