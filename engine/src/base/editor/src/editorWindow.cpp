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

    MainWindow::MainWindow()
    {
        createChild<ui::WindowTitleBar>("BoomerEditor");
        customMinSize(1200, 900);

        m_dockArea = createChild<ui::DockContainer>();
        m_dockArea->layout().notebook()->styleType("FileNotebook"_id);

        base::InplaceArray<base::SpecificClassType<IResourceEditorOpener>, 10> openerClasses;
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

    void MainWindow::loadConfig(const ConfigGroup& config)
    {
        auto& depot = base::GetService<Editor>()->managedDepot();

        // create asset browser
        {
            m_assetBrowser = base::CreateSharedPtr<AssetBrowser>(&depot);
            m_assetBrowser->loadConfig(config["AssetBrowser"]);
            m_dockArea->layout().bottom().attachPanel(m_assetBrowser);
        }

        // create background baking log
        {
            auto backgroundBakingLog = base::CreateSharedPtr<BackgroundBakerPanel>();
            backgroundBakingLog->loadConfig(config["BackgroundBake"]);
            m_dockArea->layout().bottom().attachPanel(backgroundBakingLog, false);
        }

        // open previously opened resources
        auto openedFilePaths = config.readOrDefault<base::Array<base::StringBuf>>("OpenedFiles");
        for (const auto& path : openedFilePaths)
        {
            if (auto file = depot.findManagedFile(path))
                openFile(file);
        }
    }

    void MainWindow::collectOpenedFiles(base::Array<ManagedFile*>& outFiles) const
    {
        AssetItemList list;

        m_dockArea->iterateSpecificPanels<ResourceEditor>([this, &list](ResourceEditor* editor)
            {
                editor->collectOpenedFiles(list);
                return false;
            });

        for (auto* file : list.files)
            outFiles.pushBack(file);
    }

    void MainWindow::saveConfig(ConfigGroup& config) const
    {
        // save general window settings
        m_assetBrowser->saveConfig(config["AssetBrowser"]);

        // save list of opened files
        {
            base::Array<ManagedFile*> openedFiles;
            base::Array<base::StringBuf> openedFilePaths;
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
                editor->collectModifiedFiles(modifiedFiles);
                return false;
            });

        // if we have modified files make sure we have a chance to save them
        if (!modifiedFiles.files.empty())
        {
            SaveFiles(this, modifiedFiles) = [](ui::IElement*, int result)
            {
                if (result)
                    base::platform::GetLaunchPlatform().requestExit("Editor window closed");
            };
        }
        else
        {
            // just close the window
            base::platform::GetLaunchPlatform().requestExit("Editor window closed");
        }
    }   

    void MainWindow::requestEditorClose(const base::Array<ResourceEditor*>& editors)
    {
        AssetItemList modifiedFiles;
        base::Array<ResourceEditor*> editorsToClose;
        for (const auto& editor : editors)
        {
            const auto prevCount = modifiedFiles.files.size();
            editor->saveConfig();
            editor->collectModifiedFiles(modifiedFiles);

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
                ret &= editor->saveFile(files);
                return true;
            });

        return ret;
    }

    bool MainWindow::selectFile(ManagedFile* file)
    {
        m_assetBrowser->showFile(file);
        return true;
    }

    ManagedFile* MainWindow::selectedFile() const
    {
        return m_assetBrowser->selectedFile();
    }

    ManagedDirectory* MainWindow::selectedDirectory() const
    {
        return m_assetBrowser->selectedDirectory();
    }

    bool MainWindow::openFile(ManagedFile* file)
    {
        // no file to open
        if (!file)
            return false;

        TFileSet files;
        files.insert(file);

        // activate existing editor
        if (m_dockArea->iterateSpecificPanels<ResourceEditor>([this, &files](ResourceEditor* editor)
            {
                if (editor->containsFile(files))
                {
                    editor->showFile(files);
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
                auto editorConfig = base::GetService<Editor>()->config()[editorName];

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
            auto editorConfig = base::GetService<Editor>()->config()["Generic"];
            auto editor = base::CreateSharedPtr<GenericResourceEditor>(editorConfig, file);
            auto resourceEditor = base::rtti_cast<ResourceEditor>(editor);
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

    void MainWindow::queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const
    {
        outSetup.flagForceActive = true;
        outSetup.flagAllowResize = true;
        outSetup.flagShowOnTaskBar = true;
        outSetup.title = "BoomerEditor";
        outSetup.mode = ui::WindowInitialPlacementMode::Maximize;
    }

    //--

} // editor

