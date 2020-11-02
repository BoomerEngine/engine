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
#include "sceneContentNodes.h"
#include "sceneContentStructure.h"
#include "scenePreviewPanel.h"
#include "scenePreviewContainer.h"

#include "base/object/include/actionHistory.h"
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
#include "game/world/include/world.h"
#include "sceneEditMode_Default.h"

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

    void ScenePrefabEditor::createContentStructure()
    {    
    }

    void ScenePrefabEditor::createInterface()
    {
        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:world] Preview");
            tab->layoutVertical();

            m_previewContainer = tab->createChild<ScenePreviewContainer>();
            m_previewContainer->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_previewContainer->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            m_previewContainer->bind(EVENT_EDIT_MODE_CHANGED) = [this]()
            {
                refreshEditMode();
            };

            m_previewContainer->bind(EVENT_EDIT_MODE_SELECTION_CHANGED) = [this]()
            {
                if (auto mode = m_previewContainer->mode())
                {
                    const auto selection = mode->querySelection();
                    m_structurePanel->syncExternalSelection(selection);
                }
            };

            dockLayout().attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:tree] Structure");
            tab->layoutVertical();

            m_structurePanel = tab->createChild<SceneStructurePanel>();
            m_structurePanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_structurePanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            dockLayout().left().attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:color] Inspector");
            tab->expand();
            tab->layoutVertical();

            m_inspectorPanel = tab->createChild<ui::ScrollArea>(ui::ScrollMode::Auto);
            m_inspectorPanel->expand();
            m_inspectorPanel->layoutVertical();

            dockLayout().right().attachPanel(tab, false);
        }
    }

    void ScenePrefabEditor::fillViewMenu(ui::MenuButtonContainer* menu)
    {
        TBaseClass::fillViewMenu(menu);
        //menu->createAction("ScenePreview.ShowBounds"_id, "Show bounds", "cube");
    }

    bool ScenePrefabEditor::initialize()
    {
        if (!TBaseClass::initialize())
            return false;

        m_previewWorld = CreateSharedPtr<game::World>(game::WorldType::Editor);

        auto rootNode = CreateSharedPtr<SceneContentPrefabRootNode>(nativeFile());
        rootNode->reloadContent();

        m_content = CreateSharedPtr<SceneContentStructure>(m_previewWorld, rootNode);

        m_defaultEditMode = CreateSharedPtr<SceneEditMode_Default>(actionHistory());

        m_structurePanel->bindScene(m_content, m_previewContainer);
        m_previewContainer->bindContent(m_content, m_defaultEditMode);

        refreshEditMode();
        return true;
    }

    void ScenePrefabEditor::bindResource(const res::ResourcePtr& resource)
    {
        TBaseClass::bindResource(resource);

        if (m_content)
        {
            if (auto rootNode = rtti_cast<SceneContentPrefabRootNode>(m_content->root()))
                rootNode->reloadContent();

            actionHistory()->clear();

            m_defaultEditMode->reset();
            m_previewContainer->actionSwitchMode(m_defaultEditMode);
        }
    }

    void ScenePrefabEditor::refreshEditMode()
    {
        m_inspectorPanel->removeAllChildren();

        if (auto mode = m_previewContainer->mode())
        {
            if (auto ui = mode->queryUserInterface())
            {
                m_inspectorPanel->attachChild(ui);
                ui->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                ui->customVerticalAligment(ui::ElementVerticalLayout::Top);
            }
        }
    }

    bool ScenePrefabEditor::save()
    {
        return TBaseClass::save();
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