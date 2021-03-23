/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "browserService.h"
#include "browserTreeModel.h"

#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiSearchBar.h"
#include "engine/ui/include/uiTreeViewEx.h"

#include "core/resource/include/depot.h"
#include "engine/ui/include/uiElementConfig.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserTreeDirectoryItem);
RTTI_END_TYPE();

AssetBrowserTreeDirectoryItem::AssetBrowserTreeDirectoryItem(StringView depotPath, StringView name)
    : m_depotPath(depotPath)
    , m_name(name)
{
    m_label = createChild<ui::TextLabel>(TempString("{}", m_name));
    checkIfSubFoldersExist();
}

AssetBrowserTreeDirectoryItem* AssetBrowserTreeDirectoryItem::findChild(StringView name, bool autoExpand /*= true*/)
{
    if (autoExpand)
        expand();

    for (const auto& child : children())
        if (auto dir = rtti_cast<AssetBrowserTreeDirectoryItem>(child))
            if (dir->name() == name)
                return dir;

    return nullptr;
}

void AssetBrowserTreeDirectoryItem::updateLabel()
{
    StringBuilder txt;

    int code = 0;
    GetService<AssetBrowserService>()->checkFileMark(m_depotPath, code);

    if (code == FILE_MARK_CUT)
        txt.append("[color:#FFF8]"); // make everything half transparent as we are cutting it

    if (expanded())
        txt.appendf("[img:folder_open] {}", m_name);
    else
        txt.appendf("[img:folder_closed] {}", m_name);

    if (code == FILE_MARK_CUT)
        txt << " [img:cut]";
    else if (code == FILE_MARK_COPY)
        txt << " [img:copy]";

    if (GetService<AssetBrowserService>()->checkDirectoryBookmark(m_depotPath))
        txt << " [img:star]";

    m_label->text(txt.view());
}

void AssetBrowserTreeDirectoryItem::updateButtonState()
{
    updateLabel();
    TBaseClass::updateButtonState();
}

StringBuf AssetBrowserTreeDirectoryItem::queryTooltipString() const
{
    return m_depotPath;
}

ui::DragDropDataPtr AssetBrowserTreeDirectoryItem::queryDragDropData(const BaseKeyFlags& keys, const ui::Position& position) const
{
    return nullptr;
}

ui::DragDropHandlerPtr AssetBrowserTreeDirectoryItem::handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    return nullptr;
}

void AssetBrowserTreeDirectoryItem::handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{

}

bool AssetBrowserTreeDirectoryItem::handleItemContextMenu(ui::ICollectionView* view, const ui::CollectionItems& items, const ui::Position& pos, InputKeyMask controlKeys)
{
    return false;
}

bool AssetBrowserTreeDirectoryItem::handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const
{
    return filter.testString(m_name);
}

void AssetBrowserTreeDirectoryItem::handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outData) const
{
    outData.index = uniqueIndex();
    outData.caption = m_name;
}

void AssetBrowserTreeDirectoryItem::checkIfSubFoldersExist()
{
    m_hasChildren = false;
    GetService<DepotService>()->enumDirectoriesAtPath(m_depotPath, [this](StringView name)
        {
            m_hasChildren = true;
        });
}

bool AssetBrowserTreeDirectoryItem::handleItemCanExpand() const
{
    return m_hasChildren;
}

void AssetBrowserTreeDirectoryItem::handleItemExpand()
{
    GetService<DepotService>()->enumDirectoriesAtPath(m_depotPath, [this](StringView name)
        {
            auto child = RefNew<AssetBrowserTreeDirectoryItem>(TempString("{}{}/", m_depotPath, name), name);
            addChild(child);
        });
}

void AssetBrowserTreeDirectoryItem::handleItemCollapse()
{
    removeAllChildren();
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserTreeEngineRoot);
RTTI_END_TYPE();

AssetBrowserTreeEngineRoot::AssetBrowserTreeEngineRoot()
    : AssetBrowserTreeDirectoryItem("/engine/", "<engine>")
{}

void AssetBrowserTreeEngineRoot::updateLabel()
{
    m_label->text(TempString("[img:database] Engine"));
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserTreeProjectRoot);
RTTI_END_TYPE();

AssetBrowserTreeProjectRoot::AssetBrowserTreeProjectRoot()
    : AssetBrowserTreeDirectoryItem("/project/", "<project>")
{}

void AssetBrowserTreeProjectRoot::updateLabel()
{
    m_label->text(TempString("[img:database] Project"));
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserTreePanel);
RTTI_END_TYPE();

AssetBrowserTreePanel::AssetBrowserTreePanel()
    : ui::DockPanel("[img:database] Depot", "")
    , m_depotEvents(this)
{
    bindEvents();

    {
        auto filter = createChild<ui::SearchBar>();

        m_depotTree = createChild<ui::TreeViewEx>();
        m_depotTree->sort(0);

        {
            m_engineRoot = RefNew<AssetBrowserTreeEngineRoot>();
            m_depotTree->addRoot(m_engineRoot);
        }

        {
            m_projectRoot = RefNew<AssetBrowserTreeProjectRoot>();
            m_depotTree->addRoot(m_projectRoot);
        }

        filter->bindItemView(m_depotTree);

        m_depotTree->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
        {
            if (auto depotPath = directory())
                call(EVENT_DEPOT_ACTIVE_DIRECTORY_CHANGED, depotPath);
        };
    }
}

//--

StringBuf AssetBrowserTreePanel::directory()
{
    if (auto dir = m_depotTree->selection().first<AssetBrowserTreeDirectoryItem>())
        return dir->depotPath();
    return "";
}

void AssetBrowserTreePanel::directory(StringView directoryDepotPath)
{
    if (auto* dir = findDirectory(directoryDepotPath, true))
    {
        dir->expand();
        m_depotTree->select(dir, ui::ItemSelectionModeBit::Default, true);
    }
}

AssetBrowserTreeDirectoryItem* AssetBrowserTreePanel::findDirectory(StringView depotPath, bool autoExpand) const
{
    InplaceArray<StringView, 12> parts;
    depotPath.slice("/", false, parts);

    if (parts.empty())
        return nullptr;

    AssetBrowserTreeDirectoryItem* dir = nullptr;
    if (parts[0] == "engine")
        dir = m_engineRoot;
    else if (parts[0] == "project")
        dir = m_projectRoot;

    if (!dir)
        return nullptr;

    for (uint32_t i = 1; i < parts.size(); ++i)
    {
        dir = dir->findChild(parts[i], autoExpand);
        if (!dir)
            break;
    }

    return dir;
}


//--

void AssetBrowserTreePanel::configLoad(const ui::ConfigBlock& block)
{
    // expand directories
    {
        auto expandedDirectories = block.readOrDefault<Array<StringBuf>>("ExpandedDirectories");
        for (auto& path : expandedDirectories)
            if (auto* dir = findDirectory(path, true))
                dir->expand();
    }

    // select directory
    {
        auto selectedDirectoryPath = block.readOrDefault<StringBuf>("SelectedDirectory");
        if (selectedDirectoryPath)
        {
            if (auto* dir = findDirectory(selectedDirectoryPath, true))
                m_depotTree->select(dir);
        }
    }
}

void AssetBrowserTreePanel::configSave(const ui::ConfigBlock& block) const
{
    // collect opened directories from the depot tree
    {
        Array<StringBuf> expandedFolders;
        m_depotTree->visit<AssetBrowserTreeDirectoryItem>([&expandedFolders](const AssetBrowserTreeDirectoryItem* item)
            {
                expandedFolders.pushBack(item->depotPath());
            });

        block.write("ExpandedDirectories", expandedFolders);
    }

    // get the focused folder
    if (auto selected = m_depotTree->current<AssetBrowserTreeDirectoryItem>())
        block.write("SelectedDirectory", selected->depotPath());
    else
        block.write("SelectedDirectory", "");
}

//---

void AssetBrowserTreePanel::bindEvents()
{
    m_depotEvents.bind(GetService<DepotService>()->eventKey(), EVENT_DEPOT_DIRECTORY_REMOVED) = [this](StringBuf depotPath)
    {
        handleDepotDirectoryAdded(depotPath);
    };

    m_depotEvents.bind(GetService<DepotService>()->eventKey(), EVENT_DEPOT_DIRECTORY_ADDED) = [this](StringBuf depotPath)
    {
        handleDepotDirectoryRemoved(depotPath);
    };

    m_depotEvents.bind(GetService<AssetBrowserService>()->eventKey(), EVENT_DEPOT_DIRECTORY_BOOKMARKED) = [this](StringBuf depotPath)
    {
        handleDepotDirectoryUpdated(depotPath);
    };

    m_depotEvents.bind(GetService<AssetBrowserService>()->eventKey(), EVENT_DEPOT_ASSET_MARKED) = [this](StringBuf depotPath)
    {
        handleDepotDirectoryUpdated(depotPath);
    };
}

void AssetBrowserTreePanel::handleDepotDirectoryUpdated(StringView path)
{
    if (auto* dir = findDirectory(path, false))
        dir->updateLabel();
}

void AssetBrowserTreePanel::handleDepotDirectoryAdded(StringView path)
{
    const auto parentDir = path.parentDirectory();

    if (auto parentItem = findDirectory(parentDir, false))
    {
        auto childDir = RefNew<AssetBrowserTreeDirectoryItem>(path, path.directoryName());
        parentItem->addChild(childDir);
    }
}

void AssetBrowserTreePanel::handleDepotDirectoryRemoved(StringView path)
{
    if (auto item = findDirectory(path, false))
        m_depotTree->remove(item);
}

//--

END_BOOMER_NAMESPACE_EX(ed)
