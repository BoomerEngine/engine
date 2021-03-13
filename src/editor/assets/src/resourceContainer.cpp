/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"

#include "resourceEditor.h"
#include "resourceContainer.h"
#include "browserDialogs.h"

#include "engine/canvas/include/canvas.h"
#include "core/app/include/launcherPlatform.h"
#include "engine/ui/include/uiRenderer.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDockNotebook.h"
#include "engine/ui/include/uiDockContainer.h"
#include "engine/ui/include/uiElementConfig.h"
#include "engine/ui/include/uiButton.h"
#include "core/resource/include/metadata.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceContainerWindow);
RTTI_END_TYPE();

IResourceContainerWindow::IResourceContainerWindow(StringView tag)
    : IEditorWindow(tag, "Boomer Engine")
{}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(TabbedResourceContainerWindow);
RTTI_END_TYPE();

TabbedResourceContainerWindow::TabbedResourceContainerWindow(StringView tag)
    : IResourceContainerWindow(tag)
{
    customMinSize(1200, 900);

    auto showFileListFunc = [this]() {
        auto activeEditor = m_dockArea->layout().activePanel<ResourceEditor>();
        ShowOpenedEditorsList(this, activeEditor);
    };

    auto listButton = RefNew<ui::Button>("[img:page_zoom] Files");
    listButton->styleType("BackgroundButton"_id);
    listButton->customVerticalAligment(ui::ElementVerticalLayout::Middle);
    listButton->customMargins(ui::Offsets(5, 0, 5, 0));
    m_dockArea->layout().notebook()->attachHeaderElement(listButton);

    listButton->bind(ui::EVENT_CLICKED) = showFileListFunc;
    bindShortcut("Alt+Shift+O") = showFileListFunc;
}

TabbedResourceContainerWindow::~TabbedResourceContainerWindow()
{
}

bool TabbedResourceContainerWindow::unnecessary() const
{
    return !m_dockArea->layout().findPanel<ResourceEditor>();
}

void TabbedResourceContainerWindow::update()
{
    TBaseClass::update();

    m_dockArea->layout().iterateAllPanels<ResourceEditor>([](ResourceEditor* panel)
        {
            panel->update();
        });
}

void TabbedResourceContainerWindow::iterateEditors(const std::function<void(ResourceEditor*)>& func, ClassType cls) const
{
    m_dockArea->layout().iteratePanels([&func, cls](ui::DockPanel* panel)
        {
            if (panel->is<ResourceEditor>())
                if (!cls || panel->is(cls))
                    func(static_cast<ResourceEditor*>(panel));
        });
}

void TabbedResourceContainerWindow::collectEditors(Array<ResourceEditorPtr>& outList, const std::function<bool(ResourceEditor*)>& func, ClassType cls) const
{
    m_dockArea->layout().iteratePanels([&func, &outList, cls](ui::DockPanel* panel)
        {
            if (panel->is<ResourceEditor>())
                if (!cls || panel->is(cls))
                    if (!func || func(static_cast<ResourceEditor*>(panel)))
                        outList.pushBack(AddRef(static_cast<ResourceEditor*>(panel)));
        });
}

bool TabbedResourceContainerWindow::findEditor(ResourceEditorPtr& outRet, const std::function<bool(ResourceEditor*)>& func, ClassType cls) const
{
    return m_dockArea->layout().iteratePanelsEx([&func, &outRet, cls](ui::DockPanel* panel) -> bool
        {
            if (panel->is<ResourceEditor>())
                if (!cls || panel->is(cls))
                    if (!func || func(static_cast<ResourceEditor*>(panel)))
                        outRet = AddRef(static_cast<ResourceEditor*>(panel));

            return outRet;
        });
}

bool TabbedResourceContainerWindow::selectEditor(ResourceEditor* editor) const
{
    return m_dockArea->layout().activatePanel(editor);
}

void TabbedResourceContainerWindow::attachEditor(ResourceEditor* editor, bool focus)
{
    DEBUG_CHECK_RETURN_EX(editor, "Invalid editor");
    m_dockArea->layout().attachPanel(editor, focus);
}

void TabbedResourceContainerWindow::detachEditor(ResourceEditor* editor)
{
    DEBUG_CHECK_RETURN_EX(editor, "Invalid editor");
    m_dockArea->layout().detachPanel(editor);
}

//--

void TabbedResourceContainerWindow::handleExternalCloseRequest()
{
    // collect files that are modified
    auto modifiedEditors = m_dockArea->layout().collectPanels<ResourceEditor>(
        [](ResourceEditor* ed) { return ed->modified(); });

    // if we have modified files make sure we have a chance to save them, this is also last change to cancel the close
    if (!modifiedEditors.empty())
        if (!SaveEditors(this, modifiedEditors))
            return; // closing canceled

    // close all editors, leave other panels
    {
        auto allEditors = m_dockArea->layout().collectPanels<ResourceEditor>();
        for (const auto& editor : allEditors)
        {
            editor->cleanup();
            detachEditor(editor);
        }
    }

    // we can close now
    requestClose();
}

//--

void TabbedResourceContainerWindow::queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const
{
    TBaseClass::queryInitialPlacementSetup(outSetup);

    outSetup.flagForceActive = true;
    outSetup.flagAllowResize = true;
    outSetup.flagShowOnTaskBar = true;
    outSetup.title = "Boomer Editor";
    outSetup.mode = ui::WindowInitialPlacementMode::ScreenCenter;
}

//--

END_BOOMER_NAMESPACE_EX(ed)

