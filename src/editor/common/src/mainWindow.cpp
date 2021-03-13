/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "service.h"
#include "mainWindow.h"
#include "mainStatusBar.h"
#include "mainPanel.h"

#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiDockPanel.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDockContainer.h"
#include "engine/ui/include/uiElementConfig.h"

#include "core/app/include/launcherPlatform.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MainWindow);
RTTI_END_TYPE();

static StringBuf BuildWindowCaption()
{
    StringBuilder txt;
    txt << "Boomer Editor - ";

#ifdef BUILD_DEBUG
    txt.append(" [tag:#A66][b]DEBUG BUILD[/b][/tag]");
#elif defined(BUILD_CHECKED)
    txt.append(" [tag:#999][b]CHECKED BUILD[/b][/tag]");
#elif defined(BUILD_RELEASE)
    txt.append(" [tag:#999][b]RELEASE BUILD[/b][/tag]");
#endif

    //txt.appendf(" [color:#888][size:--]({} {})[/color][/size]", __DATE__, __TIME__);
    return txt.toString();
}

MainWindow::MainWindow()
    : IEditorWindow("MainWindow", BuildWindowCaption())
{
    layoutVertical();
    customMinSize(1900, 1000);

    createMenu();

    createPanels();

    m_statusBar = createChild<MainStatusBar>();
    m_statusBar->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_statusBar->customVerticalAligment(ui::ElementVerticalLayout::Bottom);
}

MainWindow::~MainWindow()
{
}

void MainWindow::configLoad(const ui::ConfigBlock& block)
{
    TBaseClass::configLoad(block);

    for (auto& panel : m_panels)
        if (panel->id())
            panel->configLoad(block.tag(panel->id()));
}

void MainWindow::configSave(const ui::ConfigBlock& block) const
{
    TBaseClass::configSave(block);

    for (auto& panel : m_panels)
        if (panel->id())
            panel->configSave(block.tag(panel->id()));
}

void MainWindow::createMenu()
{
    auto panel = m_dockArea;
    detachChild(m_dockArea);

    auto menubar = createChild<ui::MenuBar>();
    menubar->createMenu("Tools", [this]()
        {
            auto menu = RefNew<ui::MenuButtonContainer>();
            m_dockArea->layout().fillViewMenu(menu);
            return menu->convertToPopup();
        });

    attachChild(m_dockArea);
}

void MainWindow::createPanels()
{
    InplaceArray<SpecificClassType<IEditorPanel>, 10> panelClasses;
    RTTI::GetInstance().enumClasses(panelClasses);

    for (const auto cls : panelClasses)
    {
        if (auto panel = cls->create<IEditorPanel>())
        {
            m_dockArea->layout().attachPanel(panel);
            m_panels.pushBack(panel);
        }
    }
}
    
//--

void MainWindow::update()
{
    m_statusBar->update();

    for (const auto& panel : m_panels)
        panel->update();
}

void MainWindow::handleExternalCloseRequest()
{
    GetService<EditorService>()->saveConfig();

    for (const auto& panel : m_panels)
        if (!panel->handleEditorClose())
            return;

    GetLaunchPlatform().requestExit("Editor window closed");
}   

//--

void MainWindow::queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const
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

