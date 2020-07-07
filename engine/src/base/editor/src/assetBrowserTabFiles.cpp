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

#include "base/resources/include/resourceFactory.h"
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

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_CLASS(AssetBrowserTabFiles);
    RTTI_END_TYPE();

    AssetBrowserTabFiles::AssetBrowserTabFiles(AssetBrowserContext env)
        : ui::DockPanel()
        , m_context(env)
    {
        layoutVertical();
        closeButton(true);

        // toolbar
        {
            auto toolbar = createChild<ui::ToolBar>();
            toolbar->createButton("AssetBrowserTab.Refresh"_id, "[img:arrow_refresh]", "Refresh folder structure (rescan physical directory)");
            toolbar->createSeparator();
            toolbar->createButton("AssetBrowserTab.Lock"_id, "[img:lock]", "Prevent current tab from being closed");
            toolbar->createButton("AssetBrowserTab.Bookmark"_id, "[img:star]", "Bookmark current directory");
            toolbar->createSeparator();
            toolbar->createButton("AssetBrowserTab.Icons"_id, "[img:list]", "");
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

        m_files->bind("OnItemActivated"_id, this) = [this](AssetBrowserTabFiles* tabs, ui::ModelIndex index)
        {
            if (auto dir = m_filesModel->directory(index))
            {
                directory(dir, m_dir);
                m_files->focus();
            }
            else if (auto file = m_filesModel->file(index))
            {
                if (!base::GetService<Editor>()->openFile(file))
                {
                    //ui::ShowMessageBox(this, ui::MessageBoxSetup().error().title("Edit asset").message("No editor found capable of editing selected file").error());
                    ui::PostWindowMessage(this, ui::MessageType::Error, "EditAsset"_id, base::TempString("Failed to open '{}'", file->depotPath()));
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
            base::StringBuilder txt;
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
        base::StringBuilder ret;

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

    base::Array<ManagedFile*> AssetBrowserTabFiles::selectedFiles() const
    {
        return m_filesModel->files(m_files->selection().keys());
    }

    base::Array<ManagedItem*> AssetBrowserTabFiles::selectedItems() const
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

    void AssetBrowserTabFiles::filterName(base::StringView<char> txt)
    {
        if (m_filterName != txt)
        {
            m_filterName = base::StringBuf(txt);
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

            //base::GetService<Editor>()->managedDepot().thumbnailService().requestFolder(m_dir->depotPath());

            updateTitle();

            call("OnDirectoryChanged"_id);
        }
    }

    void AssetBrowserTabFiles::refreshFileList()
    {
        m_filesModel = base::CreateSharedPtr<AssetBrowserDirContentModel>();
        m_filesModel->initializeFromDir(m_dir, m_flat);
        
        m_files->sort(0);
        m_files->model(m_filesModel);
    }

    void AssetBrowserTabFiles::duplicateTab()
    {
        auto copyTab = base::CreateSharedPtr<AssetBrowserTabFiles>(m_context);
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
        base::StringBuf newDirectoryName = "NewDirectory";
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

    static base::StringBuf BuildCoreName(const base::StringBuf& name)
    {
        base::StringBuilder txt;

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
        base::StringBuf fileName;
        for (;;)
        {
            fileName = base::TempString("{}{}.{}", coreName, index, format->extension());
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
        fileName = base::TempString("{}.{}", fileName, format->extension());

        // create depot path
        auto depotPath = base::StringBuf(base::TempString("{}{}", m_dir->depotPath(), fileName));

        // create a file in the depot
        if (auto file = base::GetService<Editor>()->managedDepot().createFile(depotPath, format->nativeResourceClass()))
        {
            if (auto index = m_filesModel->index(file))
                m_files->select(index);
        }
        else
        {
            base::StringBuf text = base::TempString("Failed to create file '{}'", fileName);
            ui::ShowMessageBox(m_files, ui::MessageBoxSetup().error().title("Create file").message(text.c_str()));
        }
    }*/

    void AssetBrowserTabFiles::selectItem(ManagedItem* ptr)
    {
        if (auto index = m_filesModel->index(ptr))
        {
            m_files->select(index);
            m_files->ensureVisible(index);
        }
    }

    void AssetBrowserTabFiles::selectItems(const base::Array<ManagedItem*>& items)
    {
        base::Array<ui::ModelIndex> indices;
        indices.reserve(items.size());

        for (const auto& item : items)
            if (auto index = m_filesModel->index(item))
                indices.pushBack(index);

        m_files->select(indices);
    }

    void AssetBrowserTabFiles::saveConfig(ConfigGroup& path) const
    {
        path.write("Locked", m_locked);
        path.write("Flat", m_flat);
        path.write("List", m_list);

        auto parentDir = m_dir->parentDirectory();

        {
            base::StringBuf dirPath;
            if (m_dir && !m_dir->isDeleted())
                dirPath = m_dir->depotPath();
            path.write<base::StringBuf>("Path", dirPath);
        }

        {
            base::StringBuf currentFilePath;
            if (auto file = selectedItem())
                if (file != parentDir)
                    currentFilePath = file->name();
            path.write<base::StringBuf>("CurrentFile", currentFilePath);
        }

        {
            base::Array<base::StringBuf> selectedFiles;
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
                base::Array<ui::ModelIndex> indices;
                auto selectedFiles = path.readOrDefault<base::Array<base::StringBuf>>("SelectedFile");
                for (auto& name : selectedFiles)
                {
                    if (auto file = m_dir->file(name))
                        if (auto index = m_filesModel->index(file))
                            indices.pushBack(index);
                }

                m_files->select(indices, ui::ItemSelectionMode(ui::ItemSelectionModeBit::Clear) | ui::ItemSelectionModeBit::Select, false);
            }

            {
                auto currentFile = path.readOrDefault<base::StringBuf>("CurrentFile");

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

    class NewDirPopup : public ui::PopupWindow
    {
    public:
        NewDirPopup(ManagedDirectory* dir, AssetBrowserTabFiles* tab)
            : m_dir(dir)
            , m_tab(tab)
        {
            layoutVertical();
            customPadding(4.0f);

            createChild<ui::TextLabel>("Enter name of new directory:");

            m_name = createChild<ui::EditBox>();
            m_name->customMargins(0, 3, 0, 3);
            m_name->customStyle<float>("width"_id, 350.0f);
            //m_name->hint("Directory name");
            m_name->text("directory");
            m_name->bind("OnTextModified"_id) = [this]() { m_failedToCreate = false; updateDirButton(); };
            m_name->bind("OnTextAccepted"_id) = [this]() { createDirectory(); };

            {
                auto buttons = createChild<ui::IElement>();
                buttons->layoutHorizontal();
                buttons->customMargins(0, 4, 0, 4);

                m_button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:add] Create");
                m_button->bind("OnClick"_id) = [this]() { createDirectory(); };

                m_status = buttons->createChild<ui::TextLabel>();
            }

            updateDirButton();
        }

        void updateDirButton()
        {
            bool canCreate = true;
            base::StringView<char> status = "";

            if (m_name)
            {
                if (auto name = m_name->text())
                {
                    if (ManagedItem::ValidateName(name))
                    {
                        auto existing = m_dir->directory(name);
                        if (existing && !existing->isDeleted())
                        {
                            status = "  [img:error] Exists";
                            canCreate = false;
                        }
                    }
                    else
                    {
                        status = "  [img:error] Invalid Name";
                        canCreate = false;
                    }
                }
                else
                {
                    status = "  [img:error] Empty Name";
                    canCreate = false;
                }
            }

            if (m_failedToCreate)
                status = "  [img:cancel] Error";

            if (m_status)
                m_status->text(status);

            if (m_button)
                m_button->enable(canCreate);
        }

        void createDirectory()
        {
            if (m_button && m_button->isEnabled())
            {
                auto name = m_name->text();
                if (auto newDir = m_dir->createDirectory(name))
                {
                    if (m_tab)
                    {
                        m_tab->selectItem(newDir);
                        m_tab->focus();
                    }
                    requestClose();
                }
                else
                {
                    m_failedToCreate = true;
                    updateDirButton();
                }
            }
        }

        virtual ui::IElement* handleFocusForwarding() override
        {
            return m_name;
        }

    private:
        ManagedDirectory* m_dir = nullptr;
        AssetBrowserTabFiles* m_tab = nullptr;
        bool m_failedToCreate = false;

        ui::EditBox* m_name;
        ui::Button* m_button;
        ui::TextLabel* m_status;
    };

    //--

    class ManagedFormatListModel : public ui::SimpleTypedListModel<const ManagedFileFormat*, const ManagedFileFormat*>
    {
    public:
        ManagedFormatListModel()
        {
            for (const auto* format : ManagedFileFormatRegistry::GetInstance().creatableFormats())
                add(format);
        }

        virtual base::StringBuf content(const ManagedFileFormat* data, int colIndex) const override
        {
            return base::TempString("  [img:page] {} [i][color:#888]({})", data->description(), data->nativeResourceClass()->name());
        }

        virtual bool compare(const ManagedFileFormat* a, const ManagedFileFormat* b, int colIndex) const override
        {
            return a->description() < b->description();
        }

        virtual bool filter(const ManagedFileFormat* data, const ui::SearchPattern& filter, int colIndex) const override
        {
            return filter.testString(data->description());
        }
    };

    static const ManagedFileFormat* GDefaultFileFormat = nullptr;

    class NewFilePopup : public ui::PopupWindow
    {
    public:
        NewFilePopup(ManagedDirectory* dir, AssetBrowserTabFiles* tab)
            : m_dir(dir)
            , m_tab(tab)
        {
            layoutVertical();
            customPadding(4.0f);

            createChild<ui::TextLabel>("Enter name of new file:");

            m_name = createChild<ui::EditBox>();
            m_name->customMargins(0, 3, 0, 3);
            m_name->customStyle<float>("width"_id, 350.0f);

            m_classList = createChild<ui::ListView>();
            m_classListModel = base::CreateSharedPtr< ManagedFormatListModel >();
            m_classList->model(m_classListModel);
            m_classList->select(m_classListModel->index(GDefaultFileFormat));

            {
                auto buttons = createChild<ui::IElement>();
                buttons->layoutHorizontal();
                buttons->customMargins(0, 4, 0, 4);

                m_button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:add] Create");

                m_status = buttons->createChild<ui::TextLabel>();
            }

            m_name->bind("OnTextModified"_id) = [this]() { m_failedToCreate = false; updateButton(); };
            m_name->bind("OnTextAccepted"_id) = [this]() { createFile(); };
            m_classList->bind("OnSelectionChanged"_id) = [this]() { updateButton(); };
            m_button->bind("OnClick"_id) = [this]() { createFile(); };

            updateButton();
        }
        
        base::StringBuf formatExtension() const
        {
            if (m_classList)
            {
                GDefaultFileFormat = m_classListModel->data(m_classList->selectionRoot());
                if (GDefaultFileFormat)
                    return GDefaultFileFormat->extension();
            }

            return "";
        }

        void updateButton()
        {
            bool canCreate = true;
            base::StringView<char> status = "";

            auto ext = formatExtension();
            if (ext && m_name)
            {
                if (auto name = m_name->text())
                {
                    if (ManagedItem::ValidateName(name))
                    {
                        auto existing = m_dir->file(base::TempString("{}.{}", name, ext));
                        if (existing && !existing->isDeleted())
                        {
                            status = "  [img:error] Exists";
                            canCreate = false;
                        }
                    }
                    else
                    {
                        status = "  [img:error] Invalid Name";
                        canCreate = false;
                    }
                }
                else
                {
                    status = "  [img:error] Empty Name";
                    canCreate = false;
                }
            }
            else
            {
                status = "  [img:error] Inalid resource type";
                canCreate = false;
            }

            if (m_failedToCreate)
                status = "  [img:cancel] Error";

            if (m_status)
                m_status->text(status);

            if (m_button)
                m_button->enable(canCreate);
        }

        void createFile()
        {
            if (m_button && m_button->isEnabled())
            {
                auto name = m_name->text();
                const auto* format = m_classListModel->data(m_classList->selectionRoot());
                if (auto newFile = m_dir->createFile(name, *format))
                {
                    if (m_tab)
                    {
                        m_tab->selectItem(newFile);
                        m_tab->focus();
                    }
                    requestClose();
                }
                else
                {
                    m_failedToCreate = true;
                    updateButton();
                }
            }
        }

        virtual ui::IElement* handleFocusForwarding() override
        {
            return m_name;
        }

    private:
        ManagedDirectory* m_dir = nullptr;
        AssetBrowserTabFiles* m_tab = nullptr;
        bool m_failedToCreate = false;

        base::RefPtr<ManagedFormatListModel> m_classListModel;

        ui::EditBox* m_name;
        ui::Button* m_button;
        ui::ListView* m_classList;
        ui::TextLabel* m_status;
    };

    //--

    bool AssetBrowserTabFiles::showGenericContextMenu()
    {
        auto menu = base::CreateSharedPtr<ui::MenuButtonContainer>();

        // new directory
        if (m_dir)
        {
            auto newDirMenu = base::CreateSharedPtr<NewDirPopup>(m_dir, this);
            menu->createSubMenu(newDirMenu, "New directory", "[img:folder_new]");

            auto newFileMenu = base::CreateSharedPtr<NewFilePopup>(m_dir, this);
            menu->createSubMenu(newFileMenu, "New file", "[img:file_add]");
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