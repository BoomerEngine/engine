/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "sceneCommonEditor.h"
#include "sceneStructurePanel.h"
#include "sceneObjectPalettePanel.h"
#include "sceneContentNodes.h"
#include "sceneContentStructure.h"
#include "scenePreviewPanel.h"
#include "scenePreviewContainer.h"
#include "sceneEditMode_Default.h"

#include "core/object/include/actionHistory.h"
#include "editor/common/include/managedFile.h"
#include "editor/common/include/managedFileFormat.h"
#include "editor/common/include/managedFileNativeResource.h"
#include "engine/ui/include/uiDockPanel.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiRuler.h"
#include "engine/ui/include/uiSplitter.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiElementConfig.h"
#include "engine/world/include/world.h"
#include "engine/world/include/prefab.h"
#include "core/resource/include/indirectTemplate.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneCommonEditor);
RTTI_END_TYPE();

SceneCommonEditor::SceneCommonEditor(ManagedFileNativeResource* file, SceneContentNodeType rootContentType, StringView defaultEditorTag)
    : ResourceEditorNativeFile(file, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::UndoRedo, ResourceEditorFeatureBit::CopyPaste }, defaultEditorTag)
    , m_rootContentType(rootContentType)
{
    m_content = RefNew<SceneContentStructure>(rootContentType);
    m_defaultEditMode = RefNew<SceneEditMode_Default>(actionHistory());

    createInterface();
}

SceneCommonEditor::~SceneCommonEditor()
{}

void SceneCommonEditor::createContentStructure()
{
}

void SceneCommonEditor::createInterface()
{
    {
        auto tab = RefNew<ui::DockPanel>("[img:world] Preview", "PreviewPanel");
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
        auto tab = RefNew<ui::DockPanel>("[img:tree] Structure", "StructurePanel");
        tab->layoutVertical();

        m_structurePanel = tab->createChild<SceneStructurePanel>(m_content, m_previewContainer);
        m_structurePanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_structurePanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

        dockLayout().left(0.2f).attachPanel(tab);
    }

    {
        auto tab = RefNew<ui::DockPanel>("[img:color] Inspector", "InspectorPanel");
        tab->expand();
        tab->layoutVertical();

        m_inspectorPanel = tab->createChild<ui::ScrollArea>(ui::ScrollMode::Auto);
        m_inspectorPanel->expand();
        m_inspectorPanel->layoutVertical();

        dockLayout().right(0.2f).attachPanel(tab, false);
    }

    {
        auto tab = RefNew<ui::DockPanel>("[img:class] Object Palette", "PalettePanel");
        tab->expand();
        tab->layoutVertical();

        m_palettePanel = tab->createChild<SceneObjectPalettePanel>(m_previewContainer);
        m_palettePanel->expand();
        m_palettePanel->layoutVertical();

        m_defaultEditMode->bindObjectPalette(m_palettePanel);

        dockLayout().right(0.2f).attachPanel(tab, false);
    }
}

void SceneCommonEditor::fillEditMenu(ui::MenuButtonContainer* menu)
{
    TBaseClass::fillEditMenu(menu);

    if (auto mode = m_previewContainer->mode())
        mode->configureEditMenu(menu);
}

void SceneCommonEditor::fillViewMenu(ui::MenuButtonContainer* menu)
{
    TBaseClass::fillViewMenu(menu);

    if (auto mode = m_previewContainer->mode())
        mode->configureViewMenu(menu);

    m_previewContainer->fillViewConfigMenu(menu);
}

bool SceneCommonEditor::initialize()
{
    if (!TBaseClass::initialize())
        return false;

    recreateContent();

    m_defaultEditMode->reset();
    m_previewContainer->actionSwitchMode(m_defaultEditMode);

    refreshEditMode();
    return true;
}

void SceneCommonEditor::refreshEditMode()
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

void SceneCommonEditor::update()
{
    m_previewContainer->update();
}

//---

void SceneCommonEditor::configLoad(const ui::ConfigBlock& block)
{
    TBaseClass::configLoad(block);

    m_structurePanel->configLoad(block.tag("Structure"));
    m_previewContainer->configLoad(block.tag("Viewports"));
    m_defaultEditMode->configLoad(m_previewContainer, block.tag("DefaultEditMode"));
}

void SceneCommonEditor::configSave(const ui::ConfigBlock& block) const
{
    TBaseClass::configSave(block);

    m_structurePanel->configSave(block.tag("Structure"));
    m_previewContainer->configSave(block.tag("Viewports"));
    m_defaultEditMode->configSave(m_previewContainer, block.tag("DefaultEditMode"));
}

//---

void SceneCommonEditor::handleGeneralCopy()
{
    if (auto mode = m_previewContainer->mode())
        mode->handleGeneralCopy();
}

void SceneCommonEditor::handleGeneralDuplicate()
{
    if (auto mode = m_previewContainer->mode())
        mode->handleGeneralDuplicate();
}

void SceneCommonEditor::handleGeneralCut()
{
    if (auto mode = m_previewContainer->mode())
        mode->handleGeneralCut();
}

void SceneCommonEditor::handleGeneralPaste()
{
    if (auto mode = m_previewContainer->mode())
        mode->handleGeneralPaste();
}

void SceneCommonEditor::handleGeneralDelete()
{
    if (auto mode = m_previewContainer->mode())
        mode->handleGeneralDelete();
}

bool SceneCommonEditor::checkGeneralCopy() const
{
    if (auto mode = m_previewContainer->mode())
        return mode->checkGeneralCopy();
    return false;
}

bool SceneCommonEditor::checkGeneralDuplicate() const
{
    if (auto mode = m_previewContainer->mode())
        return mode->checkGeneralDuplicate();
    return false;
}

bool SceneCommonEditor::checkGeneralCut() const
{
    if (auto mode = m_previewContainer->mode())
        return mode->checkGeneralCut();
    return false;
}

bool SceneCommonEditor::checkGeneralPaste() const
{
    if (auto mode = m_previewContainer->mode())
        return mode->checkGeneralPaste();
    return false;
}

bool SceneCommonEditor::checkGeneralDelete() const
{
    if (auto mode = m_previewContainer->mode())
        return mode->checkGeneralDelete();
    return false;
}

//---

END_BOOMER_NAMESPACE_EX(ed)
