/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "editorService.h"
#include "editorConfig.h"
#include "editorWindow.h"
#include "assetBrowser.h"
#include "assetBrowserTabFiles.h"
#include "assetBrowserContextMenu.h"
#include "assetFileImportPrepareTab.h"
#include "assetFileImportProcessTab.h"
#include "backgroundBakerPanel.h"
#include "resourceEditor.h"
#include "managedFileFormat.h"

#include "base/canvas/include/canvas.h"
#include "base/app/include/launcherPlatform.h"
#include "base/ui/include/uiRenderer.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiDockNotebook.h"
#include "base/ui/include/uiDockContainer.h"

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
    {
        createChild<ui::WindowTitleBar>(BuildWindowCaption());
        customMinSize(1200, 900);

        m_dockArea = createChild<ui::DockContainer>();
        m_dockArea->layout().notebook()->styleType("FileNotebook"_id);

        InplaceArray<SpecificClassType<IResourceEditorOpener>, 10> openerClasses;
        RTTI::GetInstance().enumClasses(openerClasses);

        for (const auto cls : openerClasses)
        {
            if (auto opener = cls->createPointer<IResourceEditorOpener>())
                m_editorOpeners.pushBack(opener);
        }
    }

    MainWindow::~MainWindow()
    {
        m_editorOpeners.clearPtr();
    }

    ui::DockLayoutNode& MainWindow::layout()
    {
        return m_dockArea->layout();
    }

    void MainWindow::loadConfig(const ConfigGroup& config)
    {
        auto& depot = GetService<Editor>()->managedDepot();

        // create asset browser
        {
            m_assetBrowserTab = CreateSharedPtr<AssetBrowser>(&depot);
            m_assetBrowserTab->loadConfig(config["AssetBrowser"]);
            m_dockArea->layout().bottom().attachPanel(m_assetBrowserTab);
        }

        // create asset import list
        {
            m_assetImportPrepareTab = CreateSharedPtr<AssetImportPrepareTab>();
            //m_assetImportPrepareTab->loadConfig(config["AssetImportPrepare"]);
            m_assetImportPrepareTab->expand();

            m_assetImportPrepareTab->OnStartImport = [this]()
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
            m_assetImportProcessTab = CreateSharedPtr<AssetImportMainTab>();
            //m_assetImportProcessTab->loadConfig(config["AssetImportProcess"]);
            m_assetImportProcessTab->expand();

            m_dockArea->layout().bottom().attachPanel(m_assetImportProcessTab, false);
        }

        // make sure asset browser is the first visible
        m_dockArea->activatePanel(m_assetBrowserTab);

        // open previously opened resources
        auto openedFilePaths = config.readOrDefault<Array<StringBuf>>("OpenedFiles");
        for (const auto& path : openedFilePaths)
        {
            if (auto file = depot.findManagedFile(path))
                openFile(file);
        }
    }

    void MainWindow::collectOpenedFiles(Array<ManagedFile*>& outFiles) const
    {
        AssetItemList list;

        m_dockArea->iterateSpecificPanels<ResourceEditor>([this, &list](ResourceEditor* editor)
            {
                if (editor->file())
                    list.collectFile(editor->file());
                return false;
            });

        for (auto* file : list.files)
            outFiles.pushBack(file);
    }

    void MainWindow::saveConfig(ConfigGroup& config) const
    {
        // save tabs
        m_assetBrowserTab->saveConfig(config["AssetBrowser"]);
//      m_assetImportPrepareTab->saveConfig(config["AssetImportPrepare"]);
        //assetImportProcessTab->saveConfig(config["AssetImportProcess"]);

        // save list of opened files
        {
            Array<ManagedFile*> openedFiles;
            Array<StringBuf> openedFilePaths;
            collectOpenedFiles(openedFiles);
            for (const auto& file : openedFiles)
                if (auto path = file->depotPath())
                    openedFilePaths.pushBack(path);
            config.write("OpenedFiles", openedFilePaths);
        }
    }

    //--

    void MainWindow::handleExternalCloseRequest()
    {
        AssetItemList modifiedFiles;

        m_dockArea->iterateSpecificPanels<ResourceEditor>([this, &modifiedFiles](ResourceEditor* editor)
            {
                if (editor->file() && editor->modified())
                    modifiedFiles.collectFile(editor->file());
                return false;
            });

        // if we have modified files make sure we have a chance to save them
        if (!modifiedFiles.files.empty())
        {
            SaveFiles(this, modifiedFiles) = [](ui::IElement*, int result)
            {
                if (result)
                    platform::GetLaunchPlatform().requestExit("Editor window closed");
            };
        }
        else
        {
            // just close the window
            platform::GetLaunchPlatform().requestExit("Editor window closed");
        }
    }   

    void MainWindow::requestEditorClose(const Array<ResourceEditor*>& editors)
    {
        AssetItemList modifiedFiles;
        Array<ResourceEditor*> editorsToClose;
        for (const auto& editor : editors)
        {
            const auto prevCount = modifiedFiles.files.size();
            if (editor->file() && editor->modified())
                modifiedFiles.collectFile(editor->file());

            if (modifiedFiles.files.size() == prevCount)
                editor->close();
            else
                editorsToClose.pushBack(editor);
        }

        if (!modifiedFiles.empty())
        {
            SaveFiles(this, modifiedFiles) = [this, editorsToClose](ui::IElement*, int result)
            {
                if (result)
                {
                    for (const auto& editor : editorsToClose)
                        editor->close();
                }
            };
        }
    }

    bool MainWindow::canOpenFile(const ManagedFileFormat& format) const
    {
        for (auto* opener : m_editorOpeners)
            if (opener->canOpen(format))
                return true;

        if (!format.cookableOutputs().empty() || format.nativeResourceClass())
            return true;

        return false;
    }

    bool MainWindow::saveFile(ManagedFile* file)
    {
        TFileSet files;
        if (file)
            files.insert(file);
        return saveFiles(files);
    }

    bool MainWindow::saveFiles(const TFileSet& files)
    {
        bool ret = true;

        m_dockArea->iterateSpecificPanels<ResourceEditor>([&files, &ret](ResourceEditor* editor)
            {
                if (files.contains(editor->file()))
                    ret &= editor->save();
                return true;
            });

        return ret;
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

    ManagedFile* MainWindow::selectedFile() const
    {
        return m_assetBrowserTab->selectedFile();
    }

    ManagedDirectory* MainWindow::selectedDirectory() const
    {
        return m_assetBrowserTab->selectedDirectory();
    }

    bool MainWindow::openFile(ManagedFile* file)
    {
        // no file to open
        if (!file)
            return false;

        // activate existing editor
        if (m_dockArea->iterateSpecificPanels<ResourceEditor>([this, file](ResourceEditor* editor)
            {
                if (editor->file() == file)
                {
                    m_dockArea->activatePanel(editor);
                    return true;
                }

                return false;
            }))
            return true;
            

        // look for a handler capable of servicing file
        for (auto* opener : m_editorOpeners)
        {
            if (opener->canOpen(file->fileFormat()))
            {
                auto editorName = opener->cls()->name().view().afterLastOrFull("::");
                auto editorConfig = GetService<Editor>()->config()[editorName];

                if (auto editor = opener->createEditor(editorConfig, file))
                {
                    if (editor->initialize())
                    {
                        m_dockArea->layout().attachPanel(editor);
                        return true;
                    }
                }
            }
        }

        /*// try to create a generic editor
        {
            auto editorConfig = GetService<Editor>()->config()["Generic"];
            auto editor = CreateSharedPtr<GenericResourceEditor>(editorConfig, file);
            auto resourceEditor = rtti_cast<ResourceEditor>(editor);
            if (resourceEditor->initialize())
            {
                m_dockNotebook->attachTab(resourceEditor);
                m_dockArea->selectPanel(resourceEditor);
                return true;
            }
        }*/

        TRACE_WARNING("No editor to service file '{}'", file->depotPath());
        return false;
    }

    void MainWindow::addNewImportFiles(const ManagedDirectory* currentDirectory, TImportClass resourceClass, const Array<StringBuf>& selectedAssetPaths)
    {
        m_assetImportPrepareTab->addNewImportFiles(currentDirectory, resourceClass, selectedAssetPaths);
        m_dockArea->activatePanel(m_assetImportPrepareTab);
    }

    void MainWindow::addReimportFiles(const Array<ManagedFileNativeResource*>& files)
    {
        m_assetImportPrepareTab->addReimportFiles(files);
        m_dockArea->activatePanel(m_assetImportPrepareTab);
    }

    void MainWindow::addReimportFile(ManagedFileNativeResource* file, const res::ResourceConfigurationPtr& reimportConfiguration)
    {
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

