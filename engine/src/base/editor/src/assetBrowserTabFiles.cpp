/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "assetBrowserTabFiles.h"
#include "assetFileListModel.h"
#include "assetFileListVisualizations.h"
#include "assetBrowser.h"
#include "assetBrowserDialogs.h"

#include "editorService.h"
#include "managedDirectory.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "managedFilePlaceholder.h"
#include "managedDirectoryPlaceholder.h"

#include "base/resource/include/resourceFactory.h"
#include "base/io/include/ioSystem.h"
#include "base/app/include/localServiceContainer.h"
#include "base/image/include/image.h"
#include "base/ui/include/uiListView.h"
#include "base/ui/include/uiTrackBar.h"
#include "base/ui/include/uiButton.h"
#include "base/ui/include/uiEditBox.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiMessageBox.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiDockNotebook.h"
#include "base/ui/include/uiSearchBar.h"
#include "base/resource_compiler/include/importInterface.h"
#include "base/resource_compiler/include/importFileService.h"
#include "base/io/include/fileFormat.h"
#include "base/ui/include/uiRenderer.h"
#include "base/ui/include/uiElementConfig.h"

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserTabFiles);
    RTTI_END_TYPE();

    AssetBrowserTabFiles::AssetBrowserTabFiles(ManagedDepot* depot, AssetBrowserContext env)
        : ui::DockPanel()
        , m_context(env)
        , m_depot(depot)
        , m_fileEvents(this)
    {
        layoutVertical();
        closeButton(true);

        // toolbar
        {
            auto toolbar = createChild<ui::ToolBar>();
            toolbar->createButton("AssetBrowserTab.Refresh"_id, ui::ToolbarButtonSetup().icon("arrow_refresh").caption("Force refresh").tooltip("Refresh folder structure (rescan physical directory)"));
            toolbar->createSeparator();
            toolbar->createButton("AssetBrowserTab.Lock"_id, ui::ToolbarButtonSetup().icon("lock").caption("Lock tab").tooltip("Prevent current tab from being closed"));
            toolbar->createButton("AssetBrowserTab.Bookmark"_id, ui::ToolbarButtonSetup().icon("star").caption("Favourite").tooltip("Bookmark current directory"));
            toolbar->createSeparator();
            toolbar->createButton("AssetBrowserTab.Icons"_id, ui::ToolbarButtonSetup().icon("list_bullet").caption("List").tooltip("Toggle simple list view "));
            toolbar->createSeparator();
            toolbar->createButton("AssetBrowserTab.Import"_id, ui::ToolbarButtonSetup().icon("file_go").caption("[tag:#45C]Import[/tag]").tooltip("Import assets here"));
            toolbar->createButton("AssetBrowserTab.New"_id, ui::ToolbarButtonSetup().icon("file_add").caption("[tag:#5A5]Create[/tag]").tooltip("Create assets here"));
            toolbar->createSeparator();

            if (auto bar = toolbar->createChild<ui::TrackBar>())
            {
                bar->range(16, 256);
                bar->resolution(0);
                bar->allowEditBox(false);
                bar->customStyle<float>("width"_id, 200.0f);
                bar->value(m_iconSize);
                bar->bind(ui::EVENT_TRACK_VALUE_CHANGED) = [this](double value) { iconSize(value); };
            }
        }

        // filter
        auto filter = createChild<ui::SearchBar>();

        // file list
        m_files = createChild<ui::ListView>();
        m_files->columnCount(0);
        m_files->expand();
        filter->bindItemView(m_files);

        actions().bindShortcut("AssetBrowserTab.Lock"_id, "Ctrl+L");
        actions().bindShortcut("AssetBrowserTab.List"_id, "Ctrl+K");
        actions().bindShortcut("AssetBrowserTab.Duplicate"_id, "Ctrl+T");
        actions().bindShortcut("AssetBrowserTab.Close"_id, "Ctrl+F4");
        actions().bindShortcut("AssetBrowserTab.Flatten"_id, "Ctrl+B");
        actions().bindShortcut("AssetBrowserTab.Refresh"_id, "F5");
        actions().bindShortcut("AssetBrowserTab.Back"_id, "Backspace");
        actions().bindShortcut("AssetBrowserTab.Navigate"_id, "Enter");
        actions().bindShortcut("AssetBrowserTab.Delete"_id, "Delete");
        actions().bindShortcut("AssetBrowserTab.Import"_id, "Insert");

        actions().bindCommand("AssetBrowserTab.Duplicate"_id) = [this]()
        {
            duplicateTab();
        };

        actions().bindCommand("AssetBrowserTab.Close"_id) = [this]()
        {
            handleCloseRequest();
        };

        actions().bindCommand("AssetBrowserTab.Back"_id) = [this]()
        {
            if (m_dir)
            {
                if (auto parent = m_dir->parentDirectory())
                    directory(parent, m_dir);
            }
        };

        actions().bindCommand("AssetBrowserTab.Delete"_id) = [this]()
        {
            DeleteDepotItems(this, selectedItems());
        };
            
        actions().bindCommand("AssetBrowserTab.Navigate"_id) = [this]()
        {
            /*if (auto dir = (depot::ManagedDirectory*)data)
            {
                directory(dir->());
            }*/
        };

        actions().bindCommand("AssetBrowserTab.Refresh"_id) = [this]()
        {
            m_dir->populate();
        };

        //--

        actions().bindCommand("AssetBrowserTab.Lock"_id) = [this]()
        {
            locked(!locked());
        };

        actions().bindToggle("AssetBrowserTab.Lock"_id) = [this]() { return locked(); };

        //--

        actions().bindCommand("AssetBrowserTab.Bookmark"_id) = [this]() { m_dir->bookmark(!m_dir->isBookmarked()); };
        actions().bindToggle("AssetBrowserTab.Bookmark"_id) = [this]() { return m_dir ? m_dir->isBookmarked() : false; };

        actions().bindCommand("AssetBrowserTab.Flatten"_id) = [this]() { flat(!flat()); };
        actions().bindToggle("AssetBrowserTab.Flatten"_id) = [this]() { return flat(); };

        actions().bindCommand("AssetBrowserTab.Icons"_id) = [this]() { list(!list()); };
        actions().bindToggle("AssetBrowserTab.Icons"_id) = [this]() { return !list(); };

        //--

        actions().bindCommand("AssetBrowserTab.Import"_id) = [this]() { importNewFile(nullptr); };
        actions().bindCommand("AssetBrowserTab.New"_id) = [this]() {
            auto menu = CreateSharedPtr<ui::MenuButtonContainer>();
            buildNewAssetMenu(menu);
            menu->show(this);
        };

        //--

        m_files->bind(ui::EVENT_ITEM_ACTIVATED) = [this](ui::ModelIndex index)
        {
            if (auto dir = m_filesModel->directory(index))
            {
                directory(dir, m_dir);
                m_files->focus();
            }
            else if (auto file = m_filesModel->file(index))
            {
                if (!file->open())
                    ui::PostWindowMessage(this, ui::MessageType::Error, "EditAsset"_id, TempString("Failed to open '{}'", file->depotPath()));
            }
        };

        m_files->bind(ui::EVENT_CONTEXT_MENU) = [this]()
        {
            return showGenericContextMenu();
        };

        updateTitle();
    }

    AssetBrowserTabFiles::~AssetBrowserTabFiles()
    {
        m_files->model(nullptr);
        m_filesModel.reset();
    }

    void AssetBrowserTabFiles::handleCloseRequest()
    {
        if (locked())
        {
            ui::MessageBoxSetup setup;
            setup.title("Close locked tab").yes().no().defaultNo().question();

            StringBuilder txt;
            txt.appendf("Tab '{}' is locked, close anyway?", directory()->name());
            setup.message(txt.view());

            if (ui::MessageButton::Yes != ui::ShowMessageBox(this, setup))
                return;
        }

        close();
    }

    void AssetBrowserTabFiles::updateTitle()
    {
        StringBuilder ret;

        if (m_locked)
            ret << "[img:lock] ";
        else
            ret << "[img:table] ";

        if (m_dir)
            ret << m_dir->name();

        if (m_flat)
            ret << " (Flat)";

        title(ret.toString());
    }

    ManagedItem* AssetBrowserTabFiles::selectedItem() const
    {
        return m_filesModel->item(m_files->current());
    }

    ManagedFile* AssetBrowserTabFiles::selectedFile() const
    {
        return m_filesModel->file(m_files->current());
    }

    Array<ManagedFile*> AssetBrowserTabFiles::selectedFiles() const
    {
        return m_filesModel->files(m_files->selection().keys());
    }

    Array<ManagedItem*> AssetBrowserTabFiles::selectedItems() const
    {
        return m_filesModel->items(m_files->selection().keys());
    }

    void AssetBrowserTabFiles::locked(bool isLocked)
    {
        if (m_locked != isLocked)
        {
            m_locked = isLocked;
            updateTitle();
        }
    }

    void AssetBrowserTabFiles::flat(bool isFlattened)
    {
        if (m_flat != isFlattened)
        {
            m_flat = isFlattened;
            updateTitle();
        }
    }

    void AssetBrowserTabFiles::list(bool isList)
    {
        if (m_list != isList)
        {
            m_list = isList;
            m_files->columnCount(isList ? 0 : 1);
        }
    }

    void AssetBrowserTabFiles::filterFormat(const ManagedFileFormat* filterFormat, bool toggle)
    {
        DEBUG_CHECK(filterFormat);

        if (filterFormat)
        {
            if (toggle && !m_filterFormats.contains(filterFormat))
            {
                m_filterFormats.insert(filterFormat);
            }
            else if (!toggle && m_filterFormats.contains(filterFormat))
            {
                m_filterFormats.remove(filterFormat);
            }
        }
    }

    void AssetBrowserTabFiles::filterName(StringView txt)
    {
        if (m_filterName != txt)
        {
            m_filterName = StringBuf(txt);
        }
    }

    void AssetBrowserTabFiles::directory(ManagedDirectory* dir, ManagedItem* autoSelectItem)
    {
        if (m_dir != dir)
        {
            m_dir = dir;
            refreshFileList();

            if (autoSelectItem)
                selectItem(autoSelectItem);

            // select first item if nothing else is selected
            if (!m_files->current() && m_filesModel)
                m_files->select(m_filesModel->first());

            updateTitle();

            call(EVENT_DIRECTORY_CHANGED);
        }
    }

    void AssetBrowserTabFiles::refreshFileList()
    {
        m_filesModel = CreateSharedPtr<AssetBrowserDirContentModel>(m_depot);
        m_filesModel->initializeFromDir(m_dir, m_flat);
        
        m_files->sort(0);
        m_files->model(m_filesModel);
    }

    void AssetBrowserTabFiles::duplicateTab()
    {
        auto copyTab = CreateSharedPtr<AssetBrowserTabFiles>(m_depot, m_context);
        if (m_dir)
        {
            copyTab->flat(flat());
            copyTab->list(list());
            copyTab->locked(locked());

            copyTab->directory(m_dir);
            copyTab->selectItems(selectedItems());

            if (auto notebook = findParent<ui::DockNotebook>())
            {
                if (auto node = notebook->layoutNode())
                    node->attachPanel(copyTab, true);
            }
        }
    }

    bool AssetBrowserTabFiles::selectItem(ManagedItem* ptr)
    {
        if (auto index = m_filesModel->index(ptr))
        {
            m_files->select(index);
            m_files->ensureVisible(index);
            return true;
        }

        return false;
    }

    bool AssetBrowserTabFiles::selectItems(const Array<ManagedItem*>& items)
    {
        Array<ui::ModelIndex> indices;
        indices.reserve(items.size());

        for (const auto& item : items)
            if (auto index = m_filesModel->index(item))
                indices.pushBack(index);

        if (!indices.empty())
        {
            m_files->select(indices);
            return true;
        }

        return false;
    }

    void AssetBrowserTabFiles::configSave(const ui::ConfigBlock& block) const
    {
        block.write("Locked", m_locked);
        block.write("Flat", m_flat);
        block.write("List", m_list);

        auto parentDir = m_dir->parentDirectory();

        {
            StringBuf dirPath;
            if (m_dir && !m_dir->isDeleted())
                dirPath = m_dir->depotPath();
            block.write<StringBuf>("Path", dirPath);
        }

        {
            StringBuf currentFilePath;
            if (auto file = selectedItem())
                if (file != parentDir)
                    currentFilePath = file->name();
            block.write<StringBuf>("CurrentFile", currentFilePath);
        }

        {
            Array<StringBuf> selectedFiles;
            for (auto& file : selectedItems())
                if (file != parentDir)
                    selectedFiles.pushBack(file->name());
            block.write("SelectedFile", selectedFiles);
        }
    }

    void AssetBrowserTabFiles::configLoad(const ui::ConfigBlock& block)
    {
        m_locked = block.readOrDefault("Locked", false);
        m_flat = block.readOrDefault("Flat", false);

        list(block.readOrDefault("List", false));

        if (m_dir)
        {
            {
                Array<ui::ModelIndex> indices;
                auto selectedFiles = block.readOrDefault<Array<StringBuf>>("SelectedFile");
                for (auto& name : selectedFiles)
                {
                    if (auto file = m_dir->file(name))
                        if (auto index = m_filesModel->index(file))
                            indices.pushBack(index);
                }

                m_files->select(indices, ui::ItemSelectionMode(ui::ItemSelectionModeBit::Clear) | ui::ItemSelectionModeBit::Select, false);
            }

            {
                auto currentFile = block.readOrDefault<StringBuf>("CurrentFile");

                if (auto file = m_dir->file(currentFile))
                {
                    if (auto index = m_filesModel->index(file))
                    {
                        m_files->select(index, ui::ItemSelectionModeBit::UpdateCurrent, false);
                        m_files->ensureVisible(index);
                    }
                }
            }

            if (m_files->selection().empty() && m_filesModel)
                m_files->select(m_filesModel->first());
        }

        updateTitle();
    }

    //--

    void AssetBrowserTabFiles::buildNewAssetMenu(ui::MenuButtonContainer* menu)
    {
        menu->createCallback("New directory", "[img:folder_new]", "Ctrl+Shift+D") = [this]()
        {
            createNewDirectory();
        };

        menu->createSeparator();

        for (const auto* format : ManagedFileFormatRegistry::GetInstance().creatableFormats())
        {
            menu->createCallback(TempString("{}", format->description()), "[img:file_add]", "") = [this, format]()
            {
                createNewFile(format);
            };
        }
    }

    void AssetBrowserTabFiles::buildImportAssetMenu(ui::MenuButtonContainer* menu)
    {
        for (const auto* format : ManagedFileFormatRegistry::GetInstance().importableFormats())
        {
            menu->createCallback(TempString("{}", format->description()), "[img:file_go]", "") = [this, format]()
            {
                importNewFile(format);
            };
        }
    }

    bool AssetBrowserTabFiles::showGenericContextMenu()
    {
        auto menu = CreateSharedPtr<ui::MenuButtonContainer>();

        // new directory
        if (m_dir)
        {
            // new asset sub menu
            {
                auto newAssetSubMenu = CreateSharedPtr<ui::MenuButtonContainer>();
                buildNewAssetMenu(newAssetSubMenu);
                menu->createSubMenu(newAssetSubMenu->convertToPopup(), "New", "[img:file_add]");
            }

            // create asset sub menu
            {
                auto createAssetSubMenu = CreateSharedPtr<ui::MenuButtonContainer>();
                buildImportAssetMenu(createAssetSubMenu);
                menu->createSubMenu(createAssetSubMenu->convertToPopup(), "Import", "[img:file_go]");
            }

            menu->createSeparator();
        }

        menu->show(m_files);
        return true;
    }

    void AssetBrowserTabFiles::iconSize(uint32_t size)
    {
        if (m_iconSize != size)
        {
            m_filesModel->iconSize(m_iconSize);

            m_iconSize = size;
            m_files->forEachVisualization<IAssetBrowserVisItem>([size](IAssetBrowserVisItem* item)
                {
                    item->resizeIcon(size);
                });
        }
    }

    void AssetBrowserTabFiles::createNewDirectory()
    {
        if (m_dir)
        {
            if (const auto addHocDir = base::CreateSharedPtr<ManagedDirectoryPlaceholder>(depot(), m_dir, "New Directory"))
            {
                if (auto index = m_filesModel->addAdHocElement(addHocDir))
                {
                    selectItem(addHocDir);

                    auto addHocDirRef = addHocDir.weak();

                    m_directoryPlaceholders.pushBack(addHocDir);
                    m_fileEvents.bind(addHocDir->eventKey(), EVENT_MANAGED_PLACEHOLDER_ACCEPTED) = [this, addHocDirRef]()
                    {
                        if (auto dir = addHocDirRef.lock())
                            finishDirPlaceholder(dir);
                    };
                    m_fileEvents.bind(addHocDir->eventKey(), EVENT_MANAGED_PLACEHOLDER_DISCARDED) = [this, addHocDirRef]()
                    {
                        if (auto dir = addHocDirRef.lock())
                            cancelDirPlaceholder(dir);
                    };
                }
            }
        }
    }

    void AssetBrowserTabFiles::createNewFile(const ManagedFileFormat* format)
    {
        if (m_dir)
        {
            const auto initialFileName = m_dir->adjustFileName(TempString("New{}", format->description()));
            if (const auto addHocFile = base::CreateSharedPtr<ManagedFilePlaceholder>(depot(), m_dir, initialFileName, format))
            {
                if (auto index = m_filesModel->addAdHocElement(addHocFile))
                {
                    selectItem(addHocFile);

                    auto addHocFileRef = addHocFile.weak();

                    m_filePlaceholders.pushBack(addHocFile);
                    m_fileEvents.bind(addHocFile->eventKey(), EVENT_MANAGED_PLACEHOLDER_ACCEPTED) = [this, addHocFileRef]()
                    {
                        if (auto file = addHocFileRef.lock())
                            finishFilePlaceholder(file);
                    };
                    m_fileEvents.bind(addHocFile->eventKey(), EVENT_MANAGED_PLACEHOLDER_DISCARDED) = [this, addHocFileRef]()
                    {
                        if (auto file = addHocFileRef.lock())
                            cancelFilePlaceholder(file);
                    };
                }
            }
        }
    }

    void AssetBrowserTabFiles::finishFilePlaceholder(ManagedFilePlaceholderPtr ptr)
    {
        m_fileEvents.unbind(ptr->eventKey());
        m_filesModel->removeAdHocElement(ptr);

        if (m_filePlaceholders.remove(ptr))
        {
            const auto fileName = ptr->shortName();
            if (auto file = m_dir->createFile(fileName, ptr->format()))
            {
                selectItem(file);
            }
        }
    }

    void AssetBrowserTabFiles::cancelFilePlaceholder(ManagedFilePlaceholderPtr ptr)
    {
        m_fileEvents.unbind(ptr->eventKey());
        m_filesModel->removeAdHocElement(ptr);
        m_filePlaceholders.remove(ptr);
    }

    void AssetBrowserTabFiles::finishDirPlaceholder(ManagedDirectoryPlaceholderPtr ptr)
    {
        m_fileEvents.unbind(ptr->eventKey());
        m_filesModel->removeAdHocElement(ptr);

        if (m_directoryPlaceholders.remove(ptr))
        {
            const auto dirName = ptr->name();
            if (auto file = m_dir->createDirectory(dirName))
            {
                selectItem(file);
            }
        }
    }

    void AssetBrowserTabFiles::cancelDirPlaceholder(ManagedDirectoryPlaceholderPtr ptr)
    {
        m_fileEvents.unbind(ptr->eventKey());
        m_filesModel->removeAdHocElement(ptr);
        m_directoryPlaceholders.remove(ptr);
    }

    static io::OpenSavePersistentData GImportFiles;

    bool ImportNewFiles(ui::IElement* owner, const ManagedFileFormat* format, ManagedDirectory* parentDir)
    {
        // get the native class to use for importing
        auto nativeClass = format ? format->nativeResourceClass() : res::IResource::GetStaticClass();

        // get all extensions we support
        InplaceArray<StringView, 20> extensions;
        res::IResourceImporter::ListImportableExtensionsForClass(nativeClass, extensions);

        // nothing to import
        if (extensions.empty())
            return false;

        // get list of files to import
        Array<StringBuf> assetPaths;

        {
            // export to file formats
            InplaceArray<io::FileFormat, 20> importFormats;
            for (const auto& ext : extensions)
            {
                io::FileFormat format(StringBuf(ext), TempString("Format {}", ext));
                importFormats.emplaceBack(format);
            }

            // ask for files
            auto nativeHandle = GetService<Editor>()->windowNativeHandle(owner);
            Array<StringBuf> importPaths;
            if (!base::io::ShowFileOpenDialog(nativeHandle, true, importFormats, importPaths, GImportFiles))
                return false;

            // convert the absolute paths to the source paths
            StringBuilder failedPathsMessage;
            assetPaths.reserve(importPaths.size());
            for (const auto& absolutePath : importPaths)
            {
                const auto* fileService = GetService<res::ImportFileService>();

                StringBuf sourceAssetPath;
                if (fileService->translateAbsolutePath(absolutePath, sourceAssetPath))
                {
                    assetPaths.emplaceBack(std::move(sourceAssetPath));
                }
                else
                {
                    if (failedPathsMessage.empty())
                        failedPathsMessage << "Following paths are not under the source asset repository:\n";
                    failedPathsMessage << absolutePath;
                    failedPathsMessage << "\n";
                }
            }

            if (!failedPathsMessage.empty())
                ui::PostWindowMessage(owner, ui::MessageType::Error, "ImportAsset"_id, failedPathsMessage.view());
        }

        // nothing to import ?
        if (assetPaths.empty())
            return false;

        // add to import window
        GetService<Editor>()->mainWindow().addNewImportFiles(parentDir, nativeClass, assetPaths);
        return true;
    }

    bool AssetBrowserTabFiles::importNewFile(const ManagedFileFormat* format)
    {
        return ImportNewFiles(this, format, m_dir);
    }

    //--

} // ed