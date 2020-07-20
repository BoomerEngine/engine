/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "assetBrowserTabFiles.h"
#include "assetBrowserContextMenu.h"
#include "assetFileListModel.h"
#include "assetBrowser.h"

#include "editorService.h"
#include "managedDirectory.h"
#include "managedFile.h"
#include "managedFileFormat.h"

#include "base/resource/include/resourceFactory.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/absolutePath.h"
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

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserTabFiles);
    RTTI_END_TYPE();

    AssetBrowserTabFiles::AssetBrowserTabFiles(ManagedDepot* depot, AssetBrowserContext env)
        : ui::DockPanel()
        , m_context(env)
        , m_depot(depot)
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
                bar->bind("OnValueChanged"_id, this) = [](AssetBrowserTabFiles* bar, double value)
                {
                    bar->iconSize(value);
                };
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
            AssetItemList items;
            collectItems(items, true);
            if (!items.empty())
                DeleteFiles(this, items);
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

        m_files->bind("OnItemActivated"_id, this) = [this](AssetBrowserTabFiles* tabs, ui::ModelIndex index)
        {
            if (auto dir = m_filesModel->directory(index))
            {
                directory(dir, m_dir);
                m_files->focus();
            }
            else if (auto file = m_filesModel->file(index))
            {
                if (!GetService<Editor>()->openFile(file))
                {
                    //ui::ShowMessageBox(this, ui::MessageBoxSetup().error().title("Edit asset").message("No editor found capable of editing selected file").error());
                    ui::PostWindowMessage(this, ui::MessageType::Error, "EditAsset"_id, TempString("Failed to open '{}'", file->depotPath()));
                }
            }
        };

        m_files->bind("OnContextMenu"_id, this) = [this]()
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

    ui::IElement* AssetBrowserTabFiles::handleFocusForwarding()
    {
        return m_files;
    }

    void AssetBrowserTabFiles::handleCloseRequest()
    {
        if (locked())
        {
            StringBuilder txt;
            txt.appendf("Tab '{}' is locked, close anyway?", directory()->name());

            ui::ShowMessageBox(this, ui::MessageBoxSetup().title("Close locked tab").message(txt.c_str()).yes().no().defaultNo().question()) = [](AssetBrowserTabFiles* tab, ui::MessageButton button)
            {
                if (button == ui::MessageButton::Yes)
                    tab->close();
            };
        }
        else
        {
            close();
        }
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

    void AssetBrowserTabFiles::collectItems(AssetItemList& outList, bool resursive) const
    {
        m_filesModel->collectItems(m_files->selection().keys(), outList, resursive);
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

    void AssetBrowserTabFiles::filterName(StringView<char> txt)
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
            if (!m_files->current())
                m_files->select(m_files->model()->index(0, 0, ui::ModelIndex()));

            //GetService<Editor>()->managedDepot().thumbnailService().requestFolder(m_dir->depotPath());

            updateTitle();

            call("OnDirectoryChanged"_id);
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

    /*void AssetBrowserTabFiles::showDirectoryCreationDialog()
    {
        // ask for directory name
        StringBuf newDirectoryName = "NewDirectory";
        if (!ui::ShowInputBox((), ui::InputBoxSetup().title("New directory").message("Enter name of new directory:"), newDirectoryName))
            return;

        // empty name
        if (newDirectoryName.empty())
            return;

        // if directory exists just open it
        {
            auto existingDirectory = m_dir->childDirectory(newDirectoryName.c_str());
            if (existingDirectory)
            {
                directory(existingDirectory);
                return;
            }
        }

        // create child directory
        auto newChildDirectory = m_dir->addChildDirectory(newDirectoryName);
        if (!newChildDirectory)
        {
            ui::ShowMessageBox((), ui::MessageBoxSetup().title("New directory").error().message("Failed to create new directory"));
            return;
        }

        // switch to new directory
        directory(newChildDirectory);
    }

    static StringBuf BuildCoreName(const StringBuf& name)
    {
        StringBuilder txt;

        bool firstChar = true;

        for (auto ch : name.view())
        {
            if (ch <= ' ')
                continue;

            if (firstChar && isupper(ch))
                ch = tolower(ch);

            txt.appendch(ch);
            firstChar = false;
        }

        return txt.toString();
    }

    void AssetBrowserTabFiles::showAssetCreationDialog(const depot::ManagedFileFormat* format)
    {
        if (!format)
            return;

        if (!format->nativeResourceClass())
        {
            TRACE_ERROR("Format '{}' does not support direct resource creation", format->description());
            return;
        }

        TRACE_INFO("Format selected: '{}'", format->description());

        auto coreName = BuildCoreName(format->description());
        TRACE_INFO("Core name: '{}'", coreName);

        uint32_t index = 0;
        StringBuf fileName;
        for (;;)
        {
            fileName = TempString("{}{}.{}", coreName, index, format->extension());
            if (!m_dir->file(fileName))
                break;
            index += 1;
        }

        // ask for file name
        if (!ui::ShowInputBox((), ui::InputBoxSetup().title("New file").message("Enter name of new file:"), fileName))
            return;

        // remove the ext
        fileName = fileName.stringBeforeFirst(".", true);
        if (fileName.empty())
            return;

        // force the extension
        fileName = TempString("{}.{}", fileName, format->extension());

        // create depot path
        auto depotPath = StringBuf(TempString("{}{}", m_dir->depotPath(), fileName));

        // create a file in the depot
        if (auto file = GetService<Editor>()->managedDepot().createFile(depotPath, format->nativeResourceClass()))
        {
            if (auto index = m_filesModel->index(file))
                m_files->select(index);
        }
        else
        {
            StringBuf text = TempString("Failed to create file '{}'", fileName);
            ui::ShowMessageBox(m_files, ui::MessageBoxSetup().error().title("Create file").message(text.c_str()));
        }
    }*/

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

    void AssetBrowserTabFiles::saveConfig(ConfigGroup& path) const
    {
        path.write("Locked", m_locked);
        path.write("Flat", m_flat);
        path.write("List", m_list);

        auto parentDir = m_dir->parentDirectory();

        {
            StringBuf dirPath;
            if (m_dir && !m_dir->isDeleted())
                dirPath = m_dir->depotPath();
            path.write<StringBuf>("Path", dirPath);
        }

        {
            StringBuf currentFilePath;
            if (auto file = selectedItem())
                if (file != parentDir)
                    currentFilePath = file->name();
            path.write<StringBuf>("CurrentFile", currentFilePath);
        }

        {
            Array<StringBuf> selectedFiles;
            for (auto& file : selectedItems())
                if (file != parentDir)
                    selectedFiles.pushBack(file->name());
            path.write("SelectedFile", selectedFiles);
        }
    }

    bool AssetBrowserTabFiles::loadConfig(const ConfigGroup& path)
    {
        m_locked = path.readOrDefault("Locked", false);
        m_flat = path.readOrDefault("Flat", false);

        list(path.readOrDefault("List", false));

        if (m_dir)
        {
            {
                Array<ui::ModelIndex> indices;
                auto selectedFiles = path.readOrDefault<Array<StringBuf>>("SelectedFile");
                for (auto& name : selectedFiles)
                {
                    if (auto file = m_dir->file(name))
                        if (auto index = m_filesModel->index(file))
                            indices.pushBack(index);
                }

                m_files->select(indices, ui::ItemSelectionMode(ui::ItemSelectionModeBit::Clear) | ui::ItemSelectionModeBit::Select, false);
            }

            {
                auto currentFile = path.readOrDefault<StringBuf>("CurrentFile");

                if (auto file = m_dir->file(currentFile))
                {
                    if (auto index = m_filesModel->index(file))
                    {
                        m_files->select(index, ui::ItemSelectionModeBit::UpdateCurrent, false);
                        m_files->ensureVisible(index);
                    }
                }
            }

            if (m_files->selection().empty())
            {
                auto index = m_files->model()->index(0, 0, ui::ModelIndex());
                m_files->select(index);
            }
        }

        updateTitle();
        return true;
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
            m_files->forEachVisualization<AssetBrowserFileVisItem>([size](AssetBrowserFileVisItem* item)
                {
                    item->resizeIcon(size);
                });
        }
    }

    void AssetBrowserTabFiles::createNewDirectory()
    {

    }

    void AssetBrowserTabFiles::createNewFile(const ManagedFileFormat* format)
    {

    }

    static io::OpenSavePersistentData GImportFiles;

    bool ImportNewFiles(ui::IElement* owner, const ManagedFileFormat* format, ManagedDirectory* parentDir)
    {
        // get the native class to use for importing
        auto nativeClass = format ? format->nativeResourceClass() : res::IResource::GetStaticClass();

        // get all extensions we support
        InplaceArray<StringView<char>, 20> extensions;
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
            Array<io::AbsolutePath> importPaths;
            if (!IO::GetInstance().showFileOpenDialog(nativeHandle, true, importFormats, importPaths, GImportFiles))
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
        return GetService<Editor>()->addImportFiles(assetPaths, nativeClass, parentDir);
    }

    bool AssetBrowserTabFiles::importNewFile(const ManagedFileFormat* format)
    {
        return ImportNewFiles(this, format, m_dir);
    }

    //--

    void AssetItemList::clear()
    {
        fileSet.reset();
        dirSet.reset();
        files.reset();
        dirs.reset();
    }

    void AssetItemList::collect(ManagedItem* item, bool recrusive)
    {
        if (auto* file = rtti_cast<ManagedFile>(item))
            collectFile(file);
        else if (auto* dir = rtti_cast<ManagedDirectory>(item))
            collectDir(dir, recrusive);
    }

    void AssetItemList::collectDir(ManagedDirectory* dir, bool recrusive)
    {
        if (dirSet.insert(dir))
        {
            if (recrusive)
            {
                for (const auto& child : dir->directories())
                    collectDir(child);
            }

            dirs.emplaceBack(dir);
            
            if (recrusive)
            {
                for (const auto& file : dir->files())
                    collectFile(file);
            }
        }
    }

    void AssetItemList::collectFile(ManagedFile* file)
    {
        if (fileSet.insert(file))
            files.emplaceBack(file);

        // TODO: get the ".manifest" files for the deleted file as well
    }

    //--

} // ed