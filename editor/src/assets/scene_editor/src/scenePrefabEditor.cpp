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
#include "base/world/include/world.h"

#include "base/ui/include/uiMenuBar.h"
#include "sceneEditMode_Default.h"
#include "base/world/include/worldPrefab.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePrefabEditor);
    RTTI_END_TYPE();

    ScenePrefabEditor::ScenePrefabEditor(ManagedFileNativeResource* file)
        : ResourceEditorNativeFile(file, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::UndoRedo, ResourceEditorFeatureBit::CopyPaste })
    {
        m_content = CreateSharedPtr<SceneContentStructure>(true);
        m_defaultEditMode = CreateSharedPtr<SceneEditMode_Default>(actionHistory());

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
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:world] Preview", "PreviewPanel");
            tab->layoutVertical();

            m_previewContainer = tab->createChild<ScenePreviewContainer>(m_content, m_defaultEditMode);
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

                m_previewContainer->requestRecreatePanelGizmos();
            };

            dockLayout().attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:tree] Structure", "StructurePanel");
            tab->layoutVertical();

            m_structurePanel = tab->createChild<SceneStructurePanel>(m_content, m_previewContainer);
            m_structurePanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_structurePanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            dockLayout().left().attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:color] Inspector", "InspectorPanel");
            tab->expand();
            tab->layoutVertical();

            m_inspectorPanel = tab->createChild<ui::ScrollArea>(ui::ScrollMode::Auto);
            m_inspectorPanel->expand();
            m_inspectorPanel->layoutVertical();

            dockLayout().right().attachPanel(tab, false);
        }
    }

    void ScenePrefabEditor::fillEditMenu(ui::MenuButtonContainer* menu)
    {
        TBaseClass::fillEditMenu(menu);

        if (auto mode = m_previewContainer->mode())
            mode->configureEditMenu(menu);
    }

    void ScenePrefabEditor::fillViewMenu(ui::MenuButtonContainer* menu)
    {
        TBaseClass::fillViewMenu(menu);

        m_previewContainer->fillViewConfigMenu(menu);
        
        if (auto mode = m_previewContainer->mode())
            mode->configureViewMenu(menu);
    }

    bool ScenePrefabEditor::initialize()
    {
        if (!TBaseClass::initialize())
            return false;

        recreateContent();
        refreshEditMode();
        return true;
    }

    void ScenePrefabEditor::recreateContent()
    {
        if (m_content)
        {
            if (auto rootNode = rtti_cast<SceneContentPrefabRoot>(m_content->root()))
            {
                rootNode->detachAllChildren();

                if (const auto prefabData = base::rtti_cast<base::world::Prefab>(resource()))
                    for (const auto& prefabRootNode : prefabData->nodes())
                        if (const auto editableNode = UnpackNode(nullptr, prefabRootNode))
                            rootNode->attachChildNode(editableNode);

                rootNode->resetModifiedStatus();
                m_defaultEditMode->activeNode(rootNode);
            }

            actionHistory()->clear();

            m_defaultEditMode->reset();
            m_previewContainer->actionSwitchMode(m_defaultEditMode);
        }
    }

    void ScenePrefabEditor::bindResource(const res::ResourcePtr& resource)
    {
        TBaseClass::bindResource(resource);
        recreateContent();
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

    bool ScenePrefabEditor::checkGeneralSave() const
    {
        if (const auto& root = m_content->root())
            return root->modified();

        return false;
    }

    void ScenePrefabEditor::update()
    {
        m_previewContainer->update();
    }

    bool ScenePrefabEditor::save()
    {
        if (const auto& root = m_content->root())
        {
            Array<world::NodeTemplatePtr> nodes;
            for (const auto& rootChild : root->entities())
            {
                bool unused = false;
                if (const auto compiledNode = rootChild->compileDifferentialData(unused))
                    nodes.pushBack(compiledNode);
            }

            if (const auto prefabData = base::rtti_cast<base::world::Prefab>(resource()))
                prefabData->setup(nodes);

            if (TBaseClass::save())
            {
                root->resetModifiedStatus();
                return true;
            }
        }

        return false;
    }

    //---

    void ScenePrefabEditor::handleGeneralCopy()
    {
        if (auto mode = m_previewContainer->mode())
            mode->handleGeneralCopy();
    }

    void ScenePrefabEditor::handleGeneralCut()
    {
        if (auto mode = m_previewContainer->mode())
            mode->handleGeneralCut();
    }

    void ScenePrefabEditor::handleGeneralPaste()
    {
        if (auto mode = m_previewContainer->mode())
            mode->handleGeneralPaste();
    }

    void ScenePrefabEditor::handleGeneralDelete()
    {
        if (auto mode = m_previewContainer->mode())
            mode->handleGeneralDelete();
    }

    bool ScenePrefabEditor::checkGeneralCopy() const
    {
        if (auto mode = m_previewContainer->mode())
            return mode->checkGeneralCopy();
        return false;
    }

    bool ScenePrefabEditor::checkGeneralCut() const
    {
        if (auto mode = m_previewContainer->mode())
            return mode->checkGeneralCut();
        return false;
    }

    bool ScenePrefabEditor::checkGeneralPaste() const
    {
        if (auto mode = m_previewContainer->mode())
            return mode->checkGeneralPaste();
        return false;
    }

    bool ScenePrefabEditor::checkGeneralDelete() const
    {
        if (auto mode = m_previewContainer->mode())
            return mode->checkGeneralDelete();
        return false;
    }

    //---

    class ScenePrefabResourceEditorOpener : public IResourceEditorOpener
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScenePrefabResourceEditorOpener, IResourceEditorOpener);

    public:
        virtual bool canOpen(const ManagedFileFormat& format) const override
        {
            return format.nativeResourceClass() == base::world::Prefab::GetStaticClass();
        }

        virtual base::RefPtr<ResourceEditor> createEditor(ManagedFile* file) const override
        {
            if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            {
                if (auto mesh = base::rtti_cast<base::world::Prefab>(nativeFile->loadContent()))
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