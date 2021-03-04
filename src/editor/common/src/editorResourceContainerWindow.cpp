/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "editorService.h"
#include "editorResourceContainerWindow.h"

#include "assetBrowser.h"
#include "assetBrowserTabFiles.h"
#include "assetBrowserDialogs.h"
#include "assetFileImportPrepareDialog.h"

#include "resourceEditor.h"

#include "managedFileFormat.h"
#include "managedFile.h"
#include "managedDirectory.h"
#include "managedItemCollection.h"
#include "managedFileNativeResource.h"

#include "engine/canvas/include/canvas.h"
#include "core/app/include/launcherPlatform.h"
#include "engine/ui/include/uiRenderer.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDockNotebook.h"
#include "engine/ui/include/uiDockContainer.h"
#include "engine/ui/include/uiElementConfig.h"
#include "engine/ui/include/uiButton.h"
#include "core/resource/include/metadata.h"
#include "core/resource_compiler/include/importFileList.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IBaseResourceContainerWindow);
RTTI_END_TYPE();

IBaseResourceContainerWindow::IBaseResourceContainerWindow(StringView tag, StringView title)
    : Window(ui::WindowFeatureFlagBit::DEFAULT_FRAME, title)
    , m_tag(tag)
{
    customMinSize(1200, 900);

    auto showFileListFunc = [this]() {
        ManagedFile* activeFile = nullptr;
        m_dockArea->iterateSpecificPanels<ResourceEditor>([&activeFile](ResourceEditor* editor)
            {
                if (editor->file())
                {
                    activeFile = editor->file();
                    return true;
                }

                return false;
            }, ui::DockPanelIterationMode::ActiveOnly);

        ShowOpenedFilesList(this, activeFile);
    };

    m_dockArea = createChild<ui::DockContainer>();
    m_dockArea->layout().notebook()->styleType("FileNotebook"_id);

    auto listButton = RefNew<ui::Button>("[img:page_zoom] Files");
    listButton->styleType("BackgroundButton"_id);
    listButton->customVerticalAligment(ui::ElementVerticalLayout::Middle);
    listButton->customMargins(ui::Offsets(5, 0, 5, 0));
    m_dockArea->layout().notebook()->attachHeaderElement(listButton);

    listButton->bind(ui::EVENT_CLICKED) = showFileListFunc;
    actions().bindCommand("ShowFileList"_id) = showFileListFunc;
    actions().bindShortcut("ShowFileList"_id, "Alt+Shift+O");
}

IBaseResourceContainerWindow::~IBaseResourceContainerWindow()
{
}

bool IBaseResourceContainerWindow::singularEditorOnly() const
{
    return false;
}

ui::DockLayoutNode& IBaseResourceContainerWindow::layout()
{
    return m_dockArea->layout();
}

void IBaseResourceContainerWindow::configLoad(const ui::ConfigBlock& block)
{
    TBaseClass::configLoad(block);
    m_dockArea->configLoad(block.tag("Docking"));
}

void IBaseResourceContainerWindow::configSave(const ui::ConfigBlock& block) const
{
    TBaseClass::configSave(block);
    m_dockArea->configSave(block.tag("Docking"));
}

bool IBaseResourceContainerWindow::hasEditors() const
{
    return m_dockArea->layout().iteratePanels([](ui::DockPanel* panel)
        {
            return panel->is<ResourceEditor>();
        }, ui::DockPanelIterationMode::All);
}

void IBaseResourceContainerWindow::iterateAllEditors(const std::function<void(ResourceEditor*)>& enumFunc) const
{
    m_dockArea->layout().iteratePanels([&enumFunc](ui::DockPanel* panel)
        {
            if (auto* editor = rtti_cast<ResourceEditor>(panel))
                enumFunc(editor);
            return false;
        }, ui::DockPanelIterationMode::All);
}

ResourceEditorPtr IBaseResourceContainerWindow::activeEditor() const
{
    ResourceEditorPtr ret;

    m_dockArea->layout().iteratePanels([&ret](ui::DockPanel* panel)
        {
            if (auto* editor = rtti_cast<ResourceEditor>(panel))
            {
                ret = AddRef(editor);
                return true;
            }                
            return false;
        }, ui::DockPanelIterationMode::ActiveOnly);

    return ret;
}

bool IBaseResourceContainerWindow::iterateEditors(const std::function<bool(ResourceEditor*)>& enumFunc, ui::DockPanelIterationMode mode) const
{
    return m_dockArea->layout().iteratePanels([&enumFunc](ui::DockPanel* panel)
        {
            if (auto* editor = rtti_cast<ResourceEditor>(panel))
                return enumFunc(editor);
            return false;
        }, mode);
}

void IBaseResourceContainerWindow::attachEditor(ResourceEditor* editor, bool focus)
{
    m_dockArea->layout().attachPanel(editor, focus);
}

void IBaseResourceContainerWindow::detachEditor(ResourceEditor* editor)
{
    m_dockArea->layout().detachPanel(editor);
}

bool IBaseResourceContainerWindow::selectEditor(ResourceEditor* editor)
{
    return m_dockArea->layout().activatePanel(editor);
}

//--

bool IBaseResourceContainerWindow::closeContainedFiles()
{
    // collect modified files in this container
    InplaceArray<ManagedFile*, 10> modifiedFiles;
    InplaceArray<ResourceEditorPtr, 10> editors;
    iterateAllEditors([&modifiedFiles, &editors](ResourceEditor* editor) {
        editors.pushBack(AddRef(editor));
        if (editor->modified())
            modifiedFiles.pushBack(editor->file());
        return false;
        });

    // if we have modified files make sure we have a chance to save them, this is also last change to cancel the close
    if (!modifiedFiles.empty())
        if (!SaveDepotFiles(this, modifiedFiles))
            return false;

    // close all editors
    for (const auto& editor : editors)
    {
        editor->cleanup();
        detachEditor(editor);
    }

    return true;
}

void IBaseResourceContainerWindow::handleExternalCloseRequest()
{
    if (closeContainedFiles())
        requestClose();
}   
    
//--

void IBaseResourceContainerWindow::queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const
{
    TBaseClass::queryInitialPlacementSetup(outSetup);

    outSetup.flagForceActive = true;
    outSetup.flagAllowResize = true;
    outSetup.flagShowOnTaskBar = true;
    outSetup.title = "Boomer Editor";
    outSetup.mode = ui::WindowInitialPlacementMode::ScreenCenter;
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(FloatingResourceContainerWindow);
RTTI_END_TYPE();

FloatingResourceContainerWindow::FloatingResourceContainerWindow(StringView tag)
    : IBaseResourceContainerWindow(tag, "Boomer Engine - Resource Editor")
{}

FloatingResourceContainerWindow::~FloatingResourceContainerWindow()
{}

//--

END_BOOMER_NAMESPACE_EX(ed)

