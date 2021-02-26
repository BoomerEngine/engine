/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "editorService.h"
#include "editorMainWindow.h"
#include "editorBackgroundTask.h"

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
#include "engine/ui/include/uiMessageBox.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiProgressBar.h"
#include "core/resource/include/resourceMetadata.h"
#include "core/resource_compiler/include/importFileList.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MainWindowStatusBar);
    RTTI_METADATA(ui::ElementClassNameMetadata).name("StatusBar");
RTTI_END_TYPE();

MainWindowStatusBar::MainWindowStatusBar()
{
    layoutHorizontal();

    {
        auto regionLeft = createChild<ui::IElement>();
        regionLeft->customHorizontalAligment(ui::ElementHorizontalLayout::Left);
        regionLeft->customVerticalAligment(ui::ElementVerticalLayout::Middle);

        //--

        auto button = regionLeft->createChild<ui::Button>(ui::ButtonModeBit::EventOnClick);
        button->bind(ui::EVENT_CLICKED) = [this]() { cmdShowJobDetails(); };

        auto inner = button->createChild<ui::IElement>();
        inner->layoutHorizontal();
        inner->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
        inner->customVerticalAligment(ui::ElementVerticalLayout::Middle);

        m_backgroundJobStatus = inner->createChild<ui::TextLabel>("[img:tick]");
        m_backgroundJobStatus->customVerticalAligment(ui::ElementVerticalLayout::Middle);
        m_backgroundJobStatus->customMargins(5, 0, 5, 0);

        m_backgroundJobProgress = inner->createChild<ui::ProgressBar>(true);
        m_backgroundJobProgress->customVerticalAligment(ui::ElementVerticalLayout::Middle);
        m_backgroundJobProgress->customMinSize(700, 10);
        m_backgroundJobProgress->customMargins(5, 0, 5, 0);
    }

    {
        auto regionCenter = createChild<ui::IElement>();
        regionCenter->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        regionCenter->customVerticalAligment(ui::ElementVerticalLayout::Middle);
        regionCenter->customProportion(1.0f);
    }

    {
        auto regionRight = createChild<ui::IElement>();
        regionRight->customHorizontalAligment(ui::ElementHorizontalLayout::Right);
        regionRight->customVerticalAligment(ui::ElementVerticalLayout::Middle);

           
    }

    resetBackgroundJobStatus();
}

void MainWindowStatusBar::resetBackgroundJobStatus()
{
    m_activeBackgroundJob.reset();
    m_backgroundJobStatus->text("[img:valid] Ready");
    m_backgroundJobProgress->position(1.0f, "Done");
    m_backgroundJobProgress->visibility(false);
}

void MainWindowStatusBar::pushBackgroundJobToHistory(IBackgroundTask* job, BackgroundTaskStatus status)
{

}

void MainWindowStatusBar::updateBackgroundJobStatus(IBackgroundTask* job, uint64_t count, uint64_t total, StringView text, bool hasErrors)
{
    if (job)
    {
        m_activeBackgroundJob = AddRef(job);
        m_backgroundJobStatus->text(TempString("[img:hourglass] {}", job->description()));

        float prc = total ? ((double)count / (double)total) : 1.0f;
        m_backgroundJobProgress->position(prc, text);
        m_backgroundJobProgress->visibility(true);
    }
    else
    {
        resetBackgroundJobStatus();
    }
}

void MainWindowStatusBar::cmdCancelBackgroundJob()
{

}

void MainWindowStatusBar::cmdShowJobDetails()
{
    if (m_activeBackgroundJob)
        GetEditor()->showBackgroundJobDialog(m_activeBackgroundJob);
}

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
    : IBaseResourceContainerWindow("Main", BuildWindowCaption())
{
    customMinSize(1900, 1000);   

    m_statusBar = createChild<MainWindowStatusBar>();
    m_statusBar->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_statusBar->customVerticalAligment(ui::ElementVerticalLayout::Bottom);
}

MainWindow::~MainWindow()
{
}

void MainWindow::configLoad(const ui::ConfigBlock& block)
{
    TBaseClass::configLoad(block);
}

void MainWindow::configSave(const ui::ConfigBlock& block) const
{
    TBaseClass::configSave(block);    
}    

    
//--

bool MainWindow::singularEditorOnly() const
{
    return true;
}

void MainWindow::handleExternalCloseRequest()
{
    // make sure all files are saved before closing main window
    const auto& modifiedFiles = GetEditor()->managedDepot().modifiedFilesList();
    if (SaveDepotFiles(this, modifiedFiles))
    {
        GetEditor()->saveConfig();

        platform::GetLaunchPlatform().requestExit("Editor window closed");
    }
}   
    
//--

void MainWindow::queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const
{
    TBaseClass::queryInitialPlacementSetup(outSetup);

    outSetup.flagForceActive = true;
    outSetup.flagAllowResize = true;
    outSetup.flagShowOnTaskBar = true;
    outSetup.title = "Boomer Editor";
    outSetup.mode = ui::WindowInitialPlacementMode::Maximize;
}

//--

END_BOOMER_NAMESPACE_EX(ed)

