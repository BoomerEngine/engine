/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "assetBrowserTabFiles.h"
#include "assetFileListVisualizations.h"
#include "assetBrowser.h"
#include "assetBrowserDialogs.h"

#include "editorService.h"
#include "assetFormat.h"

#include "core/resource/include/factory.h"
#include "core/io/include/io.h"
#include "core/app/include/localServiceContainer.h"
#include "core/image/include/image.h"
#include "engine/ui/include/uiListViewEx.h"
#include "engine/ui/include/uiTrackBar.h"
#include "engine/ui/include/uiButton.h"
#include "engine/ui/include/uiEditBox.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiMessageBox.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDockNotebook.h"
#include "engine/ui/include/uiSearchBar.h"
#include "core/resource_compiler/include/importInterface.h"
#include "core/resource_compiler/include/importFileService.h"
#include "core/io/include/fileFormat.h"
#include "engine/ui/include/uiRenderer.h"
#include "engine/ui/include/uiElementConfig.h"

#pragma optimize("",off)

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserTabFiles);
RTTI_END_TYPE();

AssetBrowserTabFiles::AssetBrowserTabFiles(AssetBrowserContext env)
    : ui::DockPanel()
    , m_context(env)
    , m_fileEvents(this)
{
    layoutVertical();
    tabCloseButton(true);

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
    m_files = createChild<ui::ListViewEx>();
    m_files->layoutIcons();
    //m_files->columnCount(0);
    m_files->expand();
    //filter->bindItemView(m_files);

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
        if (m_depotPath)
        {
            if (auto parentDirectory = m_depotPath.view().parentDirectory())
                directory(parentDirectory, m_depotPath.view().directoryName());
        }
    };

    actions().bindCommand("AssetBrowserTab.Delete"_id) = [this]()
    {
        //DeleteDepotItems(this, selectedItems());
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
        //m_dir->populate();
    };

    //--

    actions().bindCommand("AssetBrowserTab.Lock"_id) = [this]()
    {
        tabLocked(!tabLocked());
    };

    actions().bindToggle("AssetBrowserTab.Lock"_id) = [this]() { return tabLocked(); };

    //--

    //actions().bindCommand("AssetBrowserTab.Bookmark"_id) = [this]() { m_dir->bookmark(!m_dir->isBookmarked()); };
    //actions().bindToggle("AssetBrowserTab.Bookmark"_id) = [this]() { return m_dir ? m_dir->isBookmarked() : false; };

    actions().bindCommand("AssetBrowserTab.Flatten"_id) = [this]() { flat(!flat()); };
    actions().bindToggle("AssetBrowserTab.Flatten"_id) = [this]() { return flat(); };

    actions().bindCommand("AssetBrowserTab.Icons"_id) = [this]() { list(!list()); };
    actions().bindToggle("AssetBrowserTab.Icons"_id) = [this]() { return !list(); };

    //--

    actions().bindCommand("AssetBrowserTab.Import"_id) = [this]() { importNewFile(nullptr); };
    actions().bindCommand("AssetBrowserTab.New"_id) = [this]() {
        auto menu = RefNew<ui::MenuButtonContainer>();
        buildNewAssetMenu(menu);
        menu->show(this);
    };

    //--

    m_files->bind(ui::EVENT_ITEM_ACTIVATED) = [this](ui::CollectionItemPtr item)
    {
        if (auto dir = rtti_cast<AssetBrowserDirectoryVis>(item))
        {
            directory(dir->depotPath(), dir->displayName());
            m_files->focus();
        }
        else if (auto file = rtti_cast<AssetBrowserFileVis>(item))
        {
            if (!GetEditor()->openFileEditor(file->depotPath()))
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
}

void AssetBrowserTabFiles::updateTitle()
{
    if (m_depotPath)
    {
        const auto dirPath = m_depotPath.view().beforeLast("/").afterLast("/");
        tabTitle(dirPath ? dirPath : "Tab");
    }

    tabIcon("table");
}

StringBuf AssetBrowserTabFiles::selectedFile() const
{
    if (const auto& file = m_files->current<AssetBrowserFileVis>())
        return file->depotPath();
    return "";
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
        if (isList)
            m_files->layoutVertical();
        else
            m_files->layoutIcons();
    }
}

void AssetBrowserTabFiles::filterFormat(ClassType cls, bool toggle)
{
    if (cls)
    {
        if (toggle && !m_filterFormats.contains(cls))
        {
            m_filterFormats.insert(cls);
        }
        else if (!toggle && m_filterFormats.contains(cls))
        {
            m_filterFormats.remove(cls);
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

void AssetBrowserTabFiles::directory(StringView depotPath, StringView autoSelectName)
{
    if (m_depotPath != depotPath)
    {
        m_depotPath = StringBuf(depotPath);
        refreshFileList();

        if (autoSelectName)
        {
            auto* item = m_files->find<IAssetBrowserDepotVisItem>([autoSelectName](IAssetBrowserDepotVisItem* item)
                {
                    return item->displayName() == autoSelectName;
                });

            if (item)
                m_files->select(item);
        }

        // select first item if nothing else is selected
        /*if (!m_files->current<IAssetBrowserVisItem>())
            m_files->select(m_filesModel->first());*/

        updateTitle();

        call(EVENT_DIRECTORY_CHANGED);
    }
}

void AssetBrowserTabFiles::refreshFileList()
{
    m_files->clear();

    if (m_depotPath)
    {
        // parent directory
        if (const auto parentDir = m_depotPath.view().parentDirectory())
        {
            auto entry = RefNew<AssetBrowserDirectoryVis>(parentDir, true);
            m_files->addItem(entry);
        }

        // local child directories
        GetService<DepotService>()->enumDirectoriesAtPath(m_depotPath, [this](StringView name)
            {
                auto child = RefNew<AssetBrowserDirectoryVis>(TempString("{}{}/", m_depotPath, name), false);
                m_files->addItem(child);
            });

        // local files
        GetService<DepotService>()->enumFilesAtPath(m_depotPath, [this](StringView name)
            {
                auto child = RefNew<AssetBrowserFileVis>(TempString("{}{}", m_depotPath, name));
                m_files->addItem(child);
            });
    }
}

void AssetBrowserTabFiles::duplicateTab()
{
    auto copyTab = RefNew<AssetBrowserTabFiles>(m_context);
    if (m_depotPath)
    {
        copyTab->flat(flat());
        copyTab->list(list());
        copyTab->tabLocked(tabLocked());

        copyTab->directory(m_depotPath);
        //copyTab->selectItems(selectedItems());

        if (auto notebook = findParent<ui::DockNotebook>())
        {
            if (auto node = notebook->layoutNode())
                node->attachPanel(copyTab, true);
        }
    }
}

bool AssetBrowserTabFiles::selectItem(StringView depotPath)
{
    auto file = m_files->find<AssetBrowserFileVis>([depotPath](AssetBrowserFileVis* item)
        {
            return item->depotPath() == depotPath;
        });

    if (file)
    {
        m_files->select(file);
        return true;
    }

    return false;
}

void AssetBrowserTabFiles::configSave(const ui::ConfigBlock& block) const
{
    block.write("Locked", tabLocked());
    block.write("Flat", m_flat);
    block.write("List", m_list);

    block.write<StringBuf>("Path", m_depotPath);

    /*{
        StringBuf currentFilePath;
        if (auto file = selectedItem())
            currentFilePath = file->name();
        block.write<StringBuf>("CurrentFile", currentFilePath);
    }

    {
        Array<StringBuf> selectedFiles;
        for (auto& file : selectedItems())
            selectedFiles.pushBack(file->name());
        block.write("SelectedFile", selectedFiles);
    }*/
}

void AssetBrowserTabFiles::configLoad(const ui::ConfigBlock& block)
{
    bool locked = block.readOrDefault("Locked", false);
    m_flat = block.readOrDefault("Flat", false);

    list(block.readOrDefault("List", false));

    /*if (m_dir)
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
    }*/

    tabLocked(locked);
    updateTitle();
}

//--

void AssetBrowserTabFiles::buildNewAssetMenu(ui::MenuButtonContainer* menu)
{
    menu->createCallback("New directory", "[img:folder_new]", "Ctrl+Shift+D") = [this]()
    {
        createNewDirectoryPlaceholder();
    };

    menu->createSeparator();

    for (const auto* format : ManagedFileFormatRegistry::GetInstance().creatableFormats())
    {
        menu->createCallback(TempString("{}", format->description()), "[img:file_add]", "") = [this, format]()
        {
            createNewFilePlaceholder(format);
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
    auto menu = RefNew<ui::MenuButtonContainer>();

    // new directory
    if (m_depotPath)
    {
        // new asset sub menu
        {
            auto newAssetSubMenu = RefNew<ui::MenuButtonContainer>();
            buildNewAssetMenu(newAssetSubMenu);
            menu->createSubMenu(newAssetSubMenu->convertToPopup(), "New", "[img:file_add]");
        }

        // create asset sub menu
        {
            auto createAssetSubMenu = RefNew<ui::MenuButtonContainer>();
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
        m_iconSize = size;
        m_files->visit<IAssetBrowserVisItem>([size](IAssetBrowserVisItem* item)
            {
                item->resizeIcon(size);
            });
    }
}

void AssetBrowserTabFiles::createNewDirectory(StringView name)
{
    
}

void AssetBrowserTabFiles::createNewFile(StringView name, const ManagedFileFormat* format)
{

}

void AssetBrowserTabFiles::createNewDirectoryPlaceholder()
{
    if (m_depotPath)
    {
        auto addHocDir = RefNew<AssetBrowserPlaceholderDirectoryVis>(m_depotPath, "New Directory");
        m_files->addItem(addHocDir);
        m_files->select(addHocDir);

        m_fileEvents.bind(addHocDir->eventKey(), EVENT_ASSET_PLACEHOLDER_ACCEPTED) = [this](StringBuf name)
        {
            createNewDirectory(name);
        };
    }
}

void AssetBrowserTabFiles::createNewFilePlaceholder(const ManagedFileFormat* format)
{
    if (m_depotPath)
    {
        auto addHocDir = RefNew<AssetBrowserPlaceholderFileVis>(format, m_depotPath, TempString("New{}", format->description()));
        m_files->addItem(addHocDir);
        m_files->select(addHocDir);

        m_fileEvents.bind(addHocDir->eventKey(), EVENT_ASSET_PLACEHOLDER_ACCEPTED) = [this, format](StringBuf name)
        {
            createNewFile(name, format);
        };
    }
}

void AssetBrowserTabFiles::duplicateFile(StringView depotPath)
{
    /*
    const auto initialFileName = m_dir->adjustFileName(TempString("{}_copy", sourceFile->name().view().fileStem()));
    if (const auto addHocFile = RefNew<ManagedFilePlaceholder>(depot(), m_dir, initialFileName, &sourceFile->fileFormat()))
    {
        if (auto index = m_filesModel->addAdHocElement(addHocFile))
        {
            selectItem(addHocFile);

            auto addHocFileRef = addHocFile.weak();

            m_filePlaceholders.pushBack(addHocFile);
            m_fileEvents.bind(addHocFile->eventKey(), EVENT_MANAGED_PLACEHOLDER_ACCEPTED) = [this, addHocFileRef, sourceFile]()
            {
                if (auto file = addHocFileRef.lock())
                    finishFileDuplicate(file, sourceFile);
            };
            m_fileEvents.bind(addHocFile->eventKey(), EVENT_MANAGED_PLACEHOLDER_DISCARDED) = [this, addHocFileRef]()
            {
                if (auto file = addHocFileRef.lock())
                    cancelFilePlaceholder(file);
            };
        }
    }*/
}

static OpenSavePersistentData GImportFiles;

bool ImportNewFiles(ui::IElement* owner, StringView depotPath, const ManagedFileFormat* format)
{
    // get the native class to use for importing
    auto nativeClass = format ? format->nativeResourceClass() : IResource::GetStaticClass();

    // get all extensions we support
    InplaceArray<StringView, 20> extensions;
    IResourceImporter::ListImportableExtensionsForClass(nativeClass, extensions);

    // nothing to import
    if (extensions.empty())
        return false;

    // get list of files to import
    Array<StringBuf> assetPaths;

    {
        // export to file formats
        InplaceArray<FileFormat, 20> importFormats;
        for (const auto& ext : extensions)
        {
            FileFormat format(StringBuf(ext), TempString("Format {}", ext));
            importFormats.emplaceBack(format);
        }

        // ask for files
        auto nativeHandle = GetEditor()->windowNativeHandle(owner);
        Array<StringBuf> importPaths;
        if (!ShowFileOpenDialog(nativeHandle, true, importFormats, importPaths, GImportFiles))
            return false;

        // convert the absolute paths to the source paths
        StringBuilder failedPathsMessage;
        assetPaths.reserve(importPaths.size());
        for (const auto& absolutePath : importPaths)
        {
            const auto* fileService = GetService<SourceAssetService>();

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
    GetEditor()->importFiles(depotPath, nativeClass, assetPaths);
    return true;
}

bool AssetBrowserTabFiles::importNewFile(const ManagedFileFormat* format)
{
    if (m_depotPath)
        return ImportNewFiles(this, m_depotPath, format);
    else
        return false;
}

//--

END_BOOMER_NAMESPACE_EX(ed)
