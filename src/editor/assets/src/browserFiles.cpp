/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "browserService.h"

#include "browserFiles.h"
#include "browserFilesModel.h"

#include "importPrepare.h"

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
#include "engine/ui/include/uiRenderer.h"
#include "engine/ui/include/uiElementConfig.h"

#include "core/resource/include/factory.h"
#include "core/io/include/io.h"
#include "core/io/include/fileFormat.h"
#include "core/app/include/localServiceContainer.h"
#include "core/image/include/image.h"
#include "core/resource_compiler/include/importInterface.h"
#include "core/resource/include/depot.h"
#include "core/containers/include/path.h"
#include "core/resource/include/metadata.h"
#include "core/resource_compiler/include/sourceAssetService.h"

#pragma optimize("",off)

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

void AssetBrowserTabFilesSetup::configLoad(const ui::ConfigBlock& block)
{
    block.read("Flat", flat);
    block.read("Small", small);
    block.read("List", list);
    block.read("AllFiles", allFiles);
    block.read("Thumbnails", thumbnails);
    block.read("IconSize", iconSize);
}

void AssetBrowserTabFilesSetup::configSave(const ui::ConfigBlock& block) const
{
    block.write("Flat", flat);
    block.write("Small", small);
    block.write("List", list);
    block.write("AllFiles", allFiles);
    block.write("Thumbnails", thumbnails);
    block.write("IconSize", iconSize);
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserTabFiles);
RTTI_END_TYPE();

AssetBrowserTabFiles::AssetBrowserTabFiles(const AssetBrowserTabFilesSetup& setup)
    : ui::DockPanel()
    , m_fileEvents(this)
    , m_internalTimer(this)
    , m_setup(setup)
{
    layoutVertical();
    tabCloseButton(true);

    bindFileSystemEvents();

    initToolbar();
    initList();
    initShortcuts();

    updateTitle();
    updateToolbar();

    m_internalTimer = [this]() { trySelectItem(); };
    m_internalTimer.startRepeated(0.2f);
}

AssetBrowserTabFiles::~AssetBrowserTabFiles()
{
}

void AssetBrowserTabFiles::initList()
{
    // filter
    auto filter = createChild<ui::SearchBar>();

    // file list
    m_files = createChild<ui::ListViewEx>();
    m_files->layoutIcons();
    m_files->sort(0, true);
    m_files->expand();

    filter->bindItemView(m_files);

    //--

    m_files->bind(ui::EVENT_ITEM_ACTIVATED) = [this](ui::CollectionItemPtr item)
    {
        if (auto dir = rtti_cast<AssetBrowserParentDirectoryVis>(item))
        {
            const auto currentPath = m_depotPath;
            directory(dir->depotPath());
            select(currentPath);
            m_files->focus();
        }
        else if (auto dir = rtti_cast<AssetBrowserDirectoryVis>(item))
        {
            directory(dir->depotPath());
            m_files->focus();
        }
        else if (auto file = rtti_cast<AssetBrowserFileVis>(item))
        {
            if (!GetService<AssetBrowserService>()->openEditor(this, file->depotPath()))
                ui::PostWindowMessage(this, ui::MessageType::Error, "EditAsset"_id, TempString("Failed to open '{}'", file->depotPath()));
        }
    };

    m_files->bind(ui::EVENT_CONTEXT_MENU) = [this]()
    {
        auto menu = RefNew<ui::MenuButtonContainer>();
        buildContextMenu(*menu);
        menu->show(this);

        return true;
    };

    //--
}

void AssetBrowserTabFiles::updateTitle()
{
    if (m_depotPath)
    {
        StringBuilder txt;
        txt << m_depotPath.view().directoryName();

        if (m_setup.flat)
            txt << " (flat)";

        tabTitle(txt.view());
    }

    if (GetService<AssetBrowserService>()->checkDirectoryBookmark(m_depotPath))
        tabIcon("star");
    else
        tabIcon("table");
}

StringBuf AssetBrowserTabFiles::selectedFile() const
{
    if (const auto& file = m_files->current<AssetBrowserFileVis>())
        return file->depotPath();
    return "";
}

Array<StringBuf> AssetBrowserTabFiles::selectedFiles(bool* outHasAlsoDirsSelected) const
{
    Array<StringBuf> ret;
    
    m_files->selection().visit< IAssetBrowserDepotVisItem>([&ret, outHasAlsoDirsSelected](const IAssetBrowserDepotVisItem* item)
        {
            if (auto file = rtti_cast<AssetBrowserFileVis>(item))
            {
                ret.pushBack(file->depotPath());
            }
            else if (auto file = rtti_cast<AssetBrowserDirectoryVis>(item))
            {
                if (outHasAlsoDirsSelected)
                    *outHasAlsoDirsSelected = true;
            }

        });

    return ret;
}

Array<StringBuf> AssetBrowserTabFiles::selectedDirs() const
{
    Array<StringBuf> ret;

    m_files->selection().visit< IAssetBrowserDepotVisItem>([&ret](const IAssetBrowserDepotVisItem* item)
        {
            if (auto file = rtti_cast<AssetBrowserDirectoryVis>(item))
                ret.pushBack(file->depotPath());
        });

    return ret;
}

Array<StringBuf> AssetBrowserTabFiles::selectedItems() const
{
    Array<StringBuf> ret;

    m_files->iterate<AssetBrowserDirectoryVis>([&ret](const AssetBrowserDirectoryVis* item)
        {
            ret.pushBack(item->depotPath());
        });

    m_files->iterate<AssetBrowserFileVis>([&ret](const AssetBrowserFileVis* item)
        {
            ret.pushBack(item->depotPath());
        });

    return ret;
}

void AssetBrowserTabFiles::setup(const AssetBrowserTabFilesSetup& setup)
{
    m_setup = setup;
    updateTitle();
    refreshFileList();
}

void AssetBrowserTabFiles::directory(StringView depotPath)
{
    if (m_depotPath != depotPath)
    {
        m_depotPath = StringBuf(depotPath);
        refreshFileList();
        updateTitle();

        call(EVENT_DIRECTORY_CHANGED);
    }
}

bool AssetBrowserTabFiles::shouldShowFile(StringView depotPath) const
{
    if (m_setup.allFiles)
        return true;

    return depotPath.endsWith(ResourceMetadata::FILE_EXTENSION);
}

void AssetBrowserTabFiles::collectFlatFileItems(StringView depotPath, ui::ListViewEx* list, HashSet<StringBuf>& outObservedDirs) const
{
    outObservedDirs.insert(StringBuf(depotPath));

    GetService<DepotService>()->enumFilesAtPath(depotPath, [this, depotPath](StringView name)
        {
            if (shouldShowFile(name))
            {
                auto child = RefNew<AssetBrowserFileVis>(m_setup, TempString("{}{}", depotPath, name));
                m_files->addItem(child);
            }
        });

    // local child directories
    GetService<DepotService>()->enumDirectoriesAtPath(depotPath, [this, depotPath, list, &outObservedDirs](StringView name)
        {
            collectFlatFileItems(TempString("{}{}/", depotPath, name), list, outObservedDirs);
        });
}

void AssetBrowserTabFiles::collectNormalFileItems(StringView depotPath, ui::ListViewEx* list, HashSet<StringBuf>& outObservedDirs) const
{
    outObservedDirs.insert(m_depotPath);

    if (m_setup.allowDirs)
    {
        // parent directory
        if (const auto parentDir = m_depotPath.view().parentDirectory())
        {
            auto entry = RefNew<AssetBrowserParentDirectoryVis>(m_setup, parentDir);
            list->addItem(entry);
        }

        // local child directories
        GetService<DepotService>()->enumDirectoriesAtPath(m_depotPath, [this, list](StringView name)
            {
                auto child = RefNew<AssetBrowserDirectoryVis>(m_setup, TempString("{}{}/", m_depotPath, name));
                list->addItem(child);
            });
    }

    // local files
    GetService<DepotService>()->enumFilesAtPath(m_depotPath, [this, list](StringView name)
        {
            if (shouldShowFile(name))
            {
                auto child = RefNew<AssetBrowserFileVis>(m_setup, TempString("{}{}", m_depotPath, name));
                list->addItem(child);
            }
        });
}

void AssetBrowserTabFiles::refreshFileList()
{
    clearPendingSelectItem();

    m_files->clear();

    if (m_setup.list)
        m_files->layoutVertical();
    else
        m_files->layoutIcons();

    m_observerDepotDirectories.clear();

    if (m_depotPath)
    {
        if (m_setup.flat)
            collectFlatFileItems(m_depotPath, m_files, m_observerDepotDirectories);
        else
            collectNormalFileItems(m_depotPath, m_files, m_observerDepotDirectories);
    }
}

void AssetBrowserTabFiles::select(StringView depotPath)
{
    m_itemToSelectTimeout = NativeTimePoint::Now() + 1.0;
    m_itemToSelect = StringBuf(depotPath);
}

void AssetBrowserTabFiles::clearPendingSelectItem()
{
    m_itemToSelectTimeout.clear();
    m_itemToSelect.clear();
}

void AssetBrowserTabFiles::trySelectItem()
{
    if (m_itemToSelect)
    {
        auto file = m_files->find<IAssetBrowserDepotVisItem>([this](IAssetBrowserDepotVisItem* item)
            {
                return item->depotPath() == m_itemToSelect;
            });

        if (file)
        {
            m_files->select(file);
            clearPendingSelectItem();
        }
        else if (m_itemToSelectTimeout.reached())
        {
            clearPendingSelectItem();
        }
    }
}

void AssetBrowserTabFiles::configSave(const ui::ConfigBlock& block) const
{
    m_setup.configSave(block);

    block.write<bool>("Locked", tabLocked());
    block.write<StringBuf>("Path", m_depotPath);

    /*{
        StringBuf currentStringBuf;
        if (auto file = selectedItem())
            currentStringBuf = file->name();
        block.write<StringBuf>("CurrentFile", currentStringBuf);
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

    m_setup.configLoad(block);

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

    InplaceArray<ResourceClass, 10> allCreatableResources;
    IResourceFactory::GetAllResourceClasses(allCreatableResources);

    for (const auto cls : allCreatableResources)
    {
        const auto desc = IResource::GetResourceDescriptionForClass(cls);
        menu->createCallback(TempString("{}", desc), "[img:file_add]", "") = [this, cls]()
        {
            createNewFilePlaceholder(cls);
        };
    }
}

void AssetBrowserTabFiles::buildImportAssetMenu(ui::MenuButtonContainer* menu)
{
    InplaceArray<ResourceClass, 10> allImportableFormats;
    IResourceImporter::ListImportableResourceClasses(allImportableFormats);

    for (const auto cls : allImportableFormats)
    {
        const auto desc = IResource::GetResourceDescriptionForClass(cls);
        menu->createCallback(TempString("{}", desc), "[img:file_go]", "") = [this, cls]()
        {
            importNewFile(cls);
        };
    }
}

void AssetBrowserTabFiles::createNewDirectory(StringView name)
{
    DEBUG_CHECK_RETURN_EX(m_depotPath, "Directory can be only create in other directory");

    const auto newPath = StringBuf(TempString("{}{}/", m_depotPath, name));
    DEBUG_CHECK_RETURN_EX(ValidateDepotDirPath(newPath), "Invalid path");

    if (GetService<DepotService>()->createDirectories(newPath))
    {
        select(newPath); // NOTE: may happen only after a moment
    }
}

bool AssetBrowserTabFiles::createNewFile(StringView name, ResourceClass format)
{
    DEBUG_CHECK_RETURN_EX_V(ValidateFileName(name), "Invalid file name", false);
    DEBUG_CHECK_RETURN_EX_V(m_depotPath, "Invalid context", false);
    DEBUG_CHECK_RETURN_EX_V(format, "Invalid format", false);

    auto factory = IResourceFactory::CreateFactoryForResource(format);
    DEBUG_CHECK_RETURN_EX_V(factory, "No resource factory for format", false);

    auto res = factory->createResource();
    if (!res)
    {
        ui::PostWindowMessage(this, ui::MessageType::Error, "CreateAsset"_id, TempString("Failed to create '{}'", format));
        return false;
    }

    const auto resourceExtension = ResourceMetadata::FILE_EXTENSION;
    const auto fullResourceDepotPath = StringBuf(TempString("{}{}.{}", m_depotPath, name, resourceExtension));

    ResourceID idToSet;
    if (const auto existingMetadata = GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(fullResourceDepotPath))
    {
        if (!existingMetadata->ids.empty())
        {
            StringBuilder txt;

            txt.appendf("Asset '{}' already exists in '{}' under ID '{}'\n \n", name, m_depotPath, existingMetadata->ids[0]);

            if (existingMetadata->resourceClassType && res->is(existingMetadata->resourceClassType))
            {
                txt.append("Would you like to assign new ID to the asset or just replace the content?");
            }
            else
            {
                txt.appendf("Current asset class is '{}' that is not compatible with '{}'.\nIt's not a good idea to keep the same ID but change the class.\n \n", existingMetadata->resourceClassType, res->cls());
                txt.append("Would you like to assign new ID to the asset or just replace the content?");
            }

            ui::MessageBoxSetup setup;
            setup.type(ui::MessageType::Warning);
            setup.title("Create new asset");
            setup.message(txt.view());
            setup.yes().no().cancel().defaultCancel();
            setup.caption(ui::MessageButton::Yes, "Yes, replace ID and content");
            setup.caption(ui::MessageButton::No, "No, replace just content");
            setup.caption(ui::MessageButton::Cancel, "Cancel");
            setup.m_constructiveButton = ui::MessageButton::No;
            setup.m_destructiveButton = ui::MessageButton::Yes;

            const auto ret = ui::ShowMessageBox(this, setup);
            if (ret == ui::MessageButton::Cancel)
                return false;

            if (ret == ui::MessageButton::Yes)
                idToSet = ResourceID::Create();
        }
    }

    if (!GetService<DepotService>()->createFile(fullResourceDepotPath, res, idToSet))
    {
        ui::PostWindowMessage(this, ui::MessageType::Error, "CreateAsset"_id, TempString("Failed to create '{}'", fullResourceDepotPath));
        return false;
    }

    ui::PostWindowMessage(this, ui::MessageType::Info, "CreateAsset"_id, TempString("Created '{}'", fullResourceDepotPath));

    select(fullResourceDepotPath);
    return true;
}

void AssetBrowserTabFiles::createNewDirectoryPlaceholder()
{
    if (m_depotPath)
    {
        auto addHocDir = RefNew<AssetBrowserPlaceholderDirectoryVis>(m_setup, m_depotPath, "New Directory");
        addHocDir->bind(EVENT_ASSET_PLACEHOLDER_ACCEPTED) = [this](StringBuf name)
        {
            createNewDirectory(name);
        };

        m_files->addItem(addHocDir);
        m_files->select(addHocDir);
        m_files->ensureVisible(addHocDir);
    }
}

void AssetBrowserTabFiles::createNewFilePlaceholder(ResourceClass format)
{
    if (m_depotPath)
    {
        const auto desc = IResource::GetResourceDescriptionForClass(format);

        auto addHocDir = RefNew<AssetBrowserPlaceholderFileVis>(m_setup, format, m_depotPath, TempString("New{}", desc));
        addHocDir->bind(EVENT_ASSET_PLACEHOLDER_ACCEPTED) = [this, format](StringBuf name)
        {
            createNewFile(name, format);
        };

        m_files->addItem(addHocDir);
        m_files->select(addHocDir);
        m_files->ensureVisible(addHocDir);
    }
}

static HashMap<ResourceClass, OpenSavePersistentData> GImportFiles;

bool AssetBrowserTabFiles::importNewFile(ResourceClass cls)
{
    DEBUG_CHECK_RETURN_EX_V(m_depotPath, "Files can be only imported to specific directory", false);
    DEBUG_CHECK_RETURN_EX_V(cls, "Invalid resource format", false);

    // get all extensions we support
    InplaceArray<StringView, 20> extensions;
    IResourceImporter::ListImportableExtensionsForClass(cls, extensions);
    DEBUG_CHECK_RETURN_EX_V(!extensions.empty(), "Nothing to import from", false);

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
        Array<StringBuf> importPaths;
        if (!ShowFileOpenDialog(windowNativeHandle(), true, importFormats, importPaths, GImportFiles[cls]))
            return false;

        // convert the absolute paths to the source paths
        StringBuilder failedPathsMessage;
        assetPaths.reserve(importPaths.size());
        for (const auto& absolutePath : importPaths)
        {
            StringBuf sourceAssetPath;
            if (GetService<SourceAssetService>()->translateAbsolutePath(absolutePath, sourceAssetPath))
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
            ui::PostWindowMessage(this, ui::MessageType::Error, "ImportAsset"_id, failedPathsMessage.view());
    }

    // nothing to import ?
    if (assetPaths.empty())
        return false;

    // add to import window
    auto dlg = RefNew<AssetImportPrepareDialog>();
    for (const auto& path : assetPaths)
        dlg->importFile(cls, m_depotPath, path);
    dlg->runModal(this);
    return true;
}

//--

void AssetBrowserTabFiles::bindFileSystemEvents()
{
    const auto key = GetService<DepotService>()->eventKey();
    m_fileEvents.bind(key, EVENT_DEPOT_FILE_ADDED) = [this](StringBuf path)
    {
        processEventFileAdded(path);
    };

    m_fileEvents.bind(key, EVENT_DEPOT_FILE_REMOVED) = [this](StringBuf path)
    {
        processEventFileRemoved(path);
    };

    m_fileEvents.bind(key, EVENT_DEPOT_DIRECTORY_ADDED) = [this](StringBuf path)
    {
        processEventDirectoryAdded(path);
    };

    m_fileEvents.bind(key, EVENT_DEPOT_DIRECTORY_REMOVED) = [this](StringBuf path)
    {
        processEventDirectoryRemoved(path);
    };

    const auto assetKey = GetService<AssetBrowserService>()->eventKey();
    m_fileEvents.bind(assetKey, EVENT_DEPOT_DIRECTORY_BOOKMARKED) = [this](StringBuf path)
    {
        processEventDirectoryBookmarked(path);
    };

    m_fileEvents.bind(assetKey, EVENT_DEPOT_ASSET_MARKED) = [this](StringBuf path)
    {
        processEventFileMarked(path);
    };
}

void AssetBrowserTabFiles::processEventFileAdded(StringBuf depotPath)
{
    const auto directory = depotPath.view().baseDirectory();
    if (m_observerDepotDirectories.contains(directory))
    {
        if (shouldShowFile(depotPath))
        {
            auto item = RefNew<AssetBrowserFileVis>(m_setup, depotPath);
            m_files->addItem(item);
        }
    }
}

void AssetBrowserTabFiles::processEventFileRemoved(StringBuf depotPath)
{
    const auto directory = depotPath.view().baseDirectory();
    if (m_observerDepotDirectories.contains(directory))
    {
        auto item = m_files->find<AssetBrowserFileVis>([depotPath](AssetBrowserFileVis* item)
            {
                return item->depotPath() == depotPath;
            });

        if (item)
            m_files->removeItem(item);
    }
}

void AssetBrowserTabFiles::processEventDirectoryAdded(StringBuf depotPath)
{
    if (m_setup.allowDirs)
    {
        const auto directory = depotPath.view().parentDirectory();
        if (m_observerDepotDirectories.contains(directory))
        {
            auto item = RefNew<AssetBrowserDirectoryVis>(m_setup, depotPath);
            m_files->addItem(item);
        }
    }
}

void AssetBrowserTabFiles::processEventDirectoryRemoved(StringBuf depotPath)
{
    const auto directory = depotPath.view().parentDirectory();
    if (m_observerDepotDirectories.contains(directory))
    {
        auto item = m_files->find<AssetBrowserDirectoryVis>([depotPath](AssetBrowserDirectoryVis* item)
            {
                return item->depotPath() == depotPath;
            });

        if (item)
            m_files->removeItem(item);
    }
}

void AssetBrowserTabFiles::processEventFileMarked(StringBuf depotPath)
{
    m_files->iterate<IAssetBrowserDepotVisItem>([depotPath](IAssetBrowserDepotVisItem* item)
        {
            if (item->depotPath() == depotPath)
                item->updateIcons();
        });
}

void AssetBrowserTabFiles::processEventDirectoryBookmarked(StringBuf depotPath)
{
    if (m_depotPath == depotPath)
    {
        updateTitle();
        updateToolbar();
    }

    m_files->iterate<AssetBrowserDirectoryVis>([depotPath](AssetBrowserDirectoryVis* item)
        {
            if (item->depotPath() == depotPath)
                item->updateIcons();
        });
}

//--

END_BOOMER_NAMESPACE_EX(ed)
