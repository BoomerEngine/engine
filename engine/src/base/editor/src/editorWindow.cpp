/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "editorService.h"
#include "editorWindow.h"

#include "assetBrowser.h"
#include "assetBrowserTabFiles.h"
#include "assetBrowserDialogs.h"
#include "assetFileImportPrepareTab.h"
#include "assetFileImportProcessTab.h"

#include "resourceEditor.h"

#include "managedFileFormat.h"
#include "managedFile.h"
#include "managedDirectory.h"
#include "managedItemCollection.h"
#include "managedFileNativeResource.h"

#include "base/canvas/include/canvas.h"
#include "base/app/include/launcherPlatform.h"
#include "base/ui/include/uiRenderer.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiDockNotebook.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource_compiler/include/importFileList.h"
#include "base/ui/include/uiElementConfig.h"

namespace ed
{

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

        txt.appendf(" [color:#888][size:--]({} {})[/color][/size]", __DATE__, __TIME__);
        return txt.toString();
    }

    MainWindow::MainWindow()
        : Window(ui::WindowFeatureFlagBit::DEFAULT_FRAME, BuildWindowCaption())
    {
        customMinSize(1200, 900);

        m_dockArea = createChild<ui::DockContainer>();
        m_dockArea->layout().notebook()->styleType("FileNotebook"_id);
    }

    MainWindow::~MainWindow()
    {
    }

    ui::DockLayoutNode& MainWindow::layout()
    {
        return m_dockArea->layout();
    }

    void MainWindow::configLoad(const ui::ConfigBlock& block)
    {
        auto& depot = GetService<Editor>()->managedDepot();

        // create asset browser
        {
            m_assetBrowserTab = RefNew<AssetBrowser>(&depot);
            m_assetBrowserTab->configLoad(block.tag("AssetBrowser"));
            m_dockArea->layout().bottom().attachPanel(m_assetBrowserTab);
        }

        // create asset import list
        {
            m_assetImportPrepareTab = RefNew<AssetImportPrepareTab>();
            m_assetImportPrepareTab->configLoad(block.tag("AssetImportPrepare"));
            m_assetImportPrepareTab->expand();

            m_assetImportPrepareTab->bind(EVENT_START_ASSET_IMPORT) = [this]()
            {
                if (auto files = m_assetImportPrepareTab->compileResourceList())
                {
                    m_dockArea->activatePanel(m_assetImportProcessTab);
                    m_assetImportProcessTab->startAssetImport(files);
                }
            };

            m_dockArea->layout().bottom().attachPanel(m_assetImportPrepareTab, false);
        }

        // create asset processing tab (import queue)
        {
            m_assetImportProcessTab = RefNew<AssetImportMainTab>();
            m_assetImportProcessTab->configLoad(block.tag("AssetImportProcess"));
            m_assetImportProcessTab->expand();

            m_dockArea->layout().bottom().attachPanel(m_assetImportProcessTab, false);
        }

        // make sure asset browser is the first visible
        m_dockArea->activatePanel(m_assetBrowserTab);

        // load the dock panel config
        m_dockArea->configLoad(block.tag("Docking"));

        // open previously opened resources
        auto openedFilePaths = block.readOrDefault<Array<StringBuf>>("OpenedFiles");
        for (const auto& path : openedFilePaths)
        {
            if (auto file = depot.findManagedFile(path))
                file->open();
        }
    }

    void MainWindow::configSave(const ui::ConfigBlock& block) const
    {
        // save tabs
        m_assetBrowserTab->configSave(block.tag("AssetBrowser"));
        m_assetImportPrepareTab->configSave(block.tag("AssetImportPrepare"));
        m_assetImportProcessTab->configSave(block.tag("AssetImportProcess"));

        // save list of opened files
        {
            Array<StringBuf> openedFilePaths;
            m_dockArea->iterateSpecificPanels<ResourceEditor>([this, &openedFilePaths](ResourceEditor* editor)
                {
                    if (editor->file())
                        openedFilePaths.pushBack(editor->file()->depotPath());
                    return false;
                });

            block.write("OpenedFiles", openedFilePaths);
        }
    }

    bool MainWindow::iterateMainTabs(const std::function<bool(ui::DockPanel * panel)>& enumFunc, ui::DockPanelIterationMode mode) const
    {
        return m_dockArea->iteratePanels(enumFunc, mode);
    }

    void MainWindow::attachMainTab(ui::DockPanel* mainTab, bool focus)
    {
        m_dockArea->layout().attachPanel(mainTab, focus);
    }

    void MainWindow::detachMainTab(ui::DockPanel* mainTab)
    {
        m_dockArea->layout().detachPanel(mainTab);
    }

    bool MainWindow::activateMainTab(ui::DockPanel* mainTab)
    {
        if (m_dockArea->layout().activatePanel(mainTab))
        {
            requestActivate();
            return true;
        }

        return false;
    }

    //--

    void MainWindow::handleExternalCloseRequest()
    {
        // if we have modified files make sure we have a chance to save them, this is also last change to cancel the close
        const auto& modifiedFiles = GetService<Editor>()->managedDepot().modifiedFilesList();
        if (!SaveDepotFiles(this, modifiedFiles))
            return;

        // just close the window
        platform::GetLaunchPlatform().requestExit("Editor window closed");
    }   
    
    bool MainWindow::selectFile(ManagedFile* file)
    {
        if (file && !file->isDeleted())
        {
            if (m_assetBrowserTab->showFile(file))
            {
                m_dockArea->activatePanel(m_assetBrowserTab);
                return true;
            }

            ui::PostWindowMessage(this, ui::MessageType::Warning, "AssetBrowser"_id, TempString("Unable to locate file '{}'", file->name()));
        }

        return false;
    }

    bool MainWindow::selectDirectory(ManagedDirectory* dir, bool exploreContent)
    {
        if (dir && !dir->isDeleted())
        {
            if (m_assetBrowserTab->showDirectory(dir, exploreContent))
            {
                m_dockArea->activatePanel(m_assetBrowserTab);
                return true;
            }

            ui::PostWindowMessage(this, ui::MessageType::Warning, "AssetBrowser"_id, TempString("Unable to locate directory '{}'", dir->depotPath()));
        }

        return false;
    }

    ManagedFile* MainWindow::selectedFile() const
    {
        return m_assetBrowserTab->selectedFile();
    }

    ManagedDirectory* MainWindow::selectedDirectory() const
    {
        return m_assetBrowserTab->selectedDirectory();
    }

    void MainWindow::addNewImportFiles(const ManagedDirectory* currentDirectory, TImportClass resourceClass, const Array<StringBuf>& selectedAssetPaths)
    {
        if (!currentDirectory)
        {
            currentDirectory = selectedDirectory();
            if (!currentDirectory)
            {
                ui::PostNotificationMessage(this, ui::MessageType::Warning, "ImportAsset"_id, "No directory selected to import to");
                return;
            }
        }

        m_assetImportPrepareTab->addNewImportFiles(currentDirectory, resourceClass, selectedAssetPaths);
        m_dockArea->activatePanel(m_assetImportPrepareTab);
    }

    void MainWindow::addReimportFiles(const Array<ManagedFileNativeResource*>& files)
    {
        m_assetImportPrepareTab->addReimportFiles(files);
        m_dockArea->activatePanel(m_assetImportPrepareTab);
    }

    void MainWindow::addReimportFile(ManagedFileNativeResource* file, const res::ResourceConfigurationPtr& reimportConfiguration, bool quickstart)
    {
        if (quickstart)
        {
            if (auto metadata = file->loadMetadata())
            {
                if (metadata->importDependencies.size() >= 1)
                {
                    base::res::ImportFileEntry entry;
                    entry.assetPath = metadata->importDependencies[0].importPath;
                    entry.depotPath = file->depotPath();
                    entry.userConfiguration = reimportConfiguration;

                    if (auto fileList = RefNew<res::ImportList>(entry))
                    {
                        if (m_assetImportProcessTab->startAssetImport(fileList))
                        {
                            m_dockArea->activatePanel(m_assetImportProcessTab);
                            return;
                        }
                    }
                }
            }
        }

        m_assetImportPrepareTab->addReimportFile(file, reimportConfiguration);
        m_dockArea->activatePanel(m_assetImportPrepareTab);
    }

    //--

    void MainWindow::queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const
    {
        outSetup.flagForceActive = true;
        outSetup.flagAllowResize = true;
        outSetup.flagShowOnTaskBar = true;
        outSetup.title = "Boomer Editor";
        outSetup.mode = ui::WindowInitialPlacementMode::Maximize;
    }

    //--

} // editor

