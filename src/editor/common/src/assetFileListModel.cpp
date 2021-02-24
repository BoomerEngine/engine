/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "assetFileListModel.h"
#include "assetFileListVisualizations.h"
#include "assetBrowserTabFiles.h"
#include "assetBrowser.h"

#include "managedDirectory.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "managedDepot.h"
#include "managedDepotContextMenu.h"
#include "managedFilePlaceholder.h"
#include "managedFileNativeResource.h"
#include "managedFilePlaceholder.h"
#include "managedDirectoryPlaceholder.h"

#include "base/ui/include/uiImage.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiWindowPopup.h"
#include "base/ui/include/uiAbstractItemView.h"
#include "base/ui/include/uiCheckBox.h"
#include "base/ui/include/uiStyleValue.h"

BEGIN_BOOMER_NAMESPACE(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserDirContentModel);
RTTI_END_TYPE();

AssetBrowserDirContentModel::AssetBrowserDirContentModel(ManagedDepot* depot)
    : m_rootDir(nullptr)
    , m_depotEvents(this)
    , m_depot(depot)
{
    m_depotEvents.bind(depot->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_CREATED) = [this](ManagedDirectoryPtr dir) {
        handleCreateItemRepresentation(dir);
    };
    m_depotEvents.bind(depot->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_DELETED) = [this](ManagedDirectoryPtr dir) {
        handleDestroyItemRepresentation(dir);
    };
    m_depotEvents.bind(depot->eventKey(), EVENT_MANAGED_DEPOT_FILE_CREATED) = [this](ManagedFilePtr file) {
        handleCreateItemRepresentation(file);
    };
    m_depotEvents.bind(depot->eventKey(), EVENT_MANAGED_DEPOT_FILE_DELETED) = [this](ManagedFilePtr file) {
        handleDestroyItemRepresentation(file);
    };
}

AssetBrowserDirContentModel::~AssetBrowserDirContentModel()
{}

ui::ModelIndex AssetBrowserDirContentModel::handleCreateItemRepresentation(const ManagedItemPtr& item)
{
    ui::ModelIndex index;

    if (m_observedDirs.contains(item->parentDirectory()))
    {
        index = this->index(item);
        if (!index && canDisplayItem(item))
        {
            auto entry = RefNew<Entry>();
            entry->item = item;
            entry->itemType = TypeFromItem(item);
            entry->index = ui::ModelIndex(this, entry);
            m_items.pushBack(entry);

            index = entry->index;

            notifyItemAdded(ui::ModelIndex(), entry->index);

        }
    }

    return index;
}

void AssetBrowserDirContentModel::handleDestroyItemRepresentation(const ManagedItemPtr& item)
{
    for (auto i : m_items.indexRange())
    {
        if (m_items[i]->item == item)
        {
            auto savedEntry = m_items[i];
            m_items.erase(i);
            notifyItemRemoved(ui::ModelIndex(), savedEntry->index);
            break;
        }
    }
}

bool AssetBrowserDirContentModel::canDisplayItem(const ManagedItem* item) const
{
    if (auto file = rtti_cast<ManagedFile>(item))
        return canDisplayFile(file);
    return true;
}

bool AssetBrowserDirContentModel::canDisplayFile(const ManagedFile* file) const
{
    if (!file->fileFormat().nativeResourceClass().empty())
        return true;
    return false;
}

int AssetBrowserDirContentModel::TypeFromItem(const ManagedItem* item)
{
    if (item->is<ManagedFilePlaceholder>())
        return TYPE_ADHOC_FILE;
    if (item->is<ManagedFile>())
        return TYPE_FILE;
    if (item->is<ManagedDirectory>())
        return TYPE_DIRECTORY;

    return TYPE_FILE;
}

ui::ModelIndex AssetBrowserDirContentModel::addAdHocElement(ManagedItem* item)
{
    if (!m_adHocElements.contains(item))
    {
        auto itemPtr = AddRef(item);
        m_adHocElements.pushBack(itemPtr);
        return handleCreateItemRepresentation(itemPtr);
    }

    return ui::ModelIndex();
}

void AssetBrowserDirContentModel::removeAdHocElement(ManagedItem* item)
{
    auto itemPtr = AddRef(item);
    if (m_adHocElements.remove(itemPtr))
    {
        handleDestroyItemRepresentation(itemPtr);
    }
}

void AssetBrowserDirContentModel::initializeFromDir(ManagedDirectory* dir, bool flat)
{
    reset();
    m_items.clear();
    m_observedDirs.reset();
    m_rootDir = dir;

    if (dir)
    {
        dir->populate();

        m_observedDirs.insert(dir);

        if (dir->parentDirectory())
        {
            auto entry = RefNew<Entry>();
            entry->index = ui::ModelIndex(this, entry);
            entry->item = dir->parentDirectory();
            entry->itemType = TYPE_PARENT_DIRECTORY;
            entry->customName = "..";
            m_items.pushBack(entry);
        }

        for (auto& dir : dir->directories())
        {
            if (!dir->isDeleted())
            {
                auto entry = RefNew<Entry>();
                entry->index = ui::ModelIndex(this, entry);
                entry->item = dir;
                entry->itemType = TYPE_DIRECTORY;
                m_items.pushBack(entry);
            }
        }

        for (auto& file : dir->files())
        {
            if (!file->isDeleted())
            {
                if (canDisplayFile(file))
                {
                    auto entry = RefNew<Entry>();
                    entry->index = ui::ModelIndex(this, entry);
                    entry->item = file;
                    entry->itemType = TYPE_FILE;
                    m_items.pushBack(entry);
                }
            }
        }
    }
}

ManagedDirectory* AssetBrowserDirContentModel::directory(const ui::ModelIndex& index) const
{
    auto entry  = index.unsafe<Entry>();
    return entry ? rtti_cast<ManagedDirectory>(entry->item) : nullptr;
}

ManagedFile* AssetBrowserDirContentModel::file(const ui::ModelIndex& index) const
{
    auto entry  = index.unsafe<Entry>();
    return entry ? rtti_cast<ManagedFile>(entry->item) : nullptr;
}

ManagedItem* AssetBrowserDirContentModel::item(const ui::ModelIndex& index) const
{
    auto entry  = index.unsafe<Entry>();
    return entry ? entry->item : nullptr;
}
    
ui::ModelIndex AssetBrowserDirContentModel::index(const ManagedItem* ptr) const
{
    for (const auto& item : m_items)
        if (item->item == ptr)
            return item->index;

    return ui::ModelIndex();
}

ui::ModelIndex AssetBrowserDirContentModel::first() const
{
    if (!m_items.empty())
        return m_items.front()->index;
    return ui::ModelIndex();
}

Array<ManagedFile*> AssetBrowserDirContentModel::files(const Array<ui::ModelIndex>& indices) const
{
    Array<ManagedFile*> files;
    files.reserve(indices.size());

    for (auto& index : indices)
        if (auto entry  = index.unsafe<Entry>())
            if (auto file = rtti_cast<ManagedFile>(entry->item))
                files.emplaceBack(file);

    return files;
}

Array<ManagedItem*> AssetBrowserDirContentModel::items(const Array<ui::ModelIndex>& indices) const
{
    Array<ManagedItem*> files;
    files.reserve(indices.size());

    for (auto& index : indices)
        if (auto entry  = index.unsafe<Entry>())
            files.emplaceBack(entry->item);

    return files;
}

bool AssetBrowserDirContentModel::hasChildren(const ui::ModelIndex& parent) const
{
    return !parent && !m_items.empty();
}

void AssetBrowserDirContentModel::children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const
{
    if (!parent)
    {
        outChildrenIndices.reserve(m_items.size());

        for (const auto& item : m_items)
            outChildrenIndices.pushBack(item->index);
    }
}

ui::ModelIndex AssetBrowserDirContentModel::parent(const ui::ModelIndex& item) const
{
    return ui::ModelIndex();
}

bool AssetBrowserDirContentModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex) const
{
    auto firstEntry  = first.unsafe<Entry>();
    auto secondEntry  = second.unsafe<Entry>();

    if (firstEntry->itemType != secondEntry->itemType)
        return firstEntry->itemType < secondEntry->itemType;

    return firstEntry->item->name() < secondEntry->item->name();
}

bool AssetBrowserDirContentModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex) const
{
    auto entry = id.unsafe<Entry>();
    if (entry)
        return filter.testString(entry->item->name());
    return true;
}

ui::PopupPtr AssetBrowserDirContentModel::contextMenu(ui::AbstractItemView* view, const Array<ui::ModelIndex>& indices) const
{
    Array<ManagedItem*> depotItems;
    depotItems.reserve(indices.size());

    for (const auto& index : indices)
        if (auto depotItem = item(index))
            depotItems.pushBack(depotItem);

    if (auto files = view->findParent<AssetBrowserTabFiles>())
    {
        auto menu = RefNew<ui::MenuButtonContainer>();
        DepotMenuContext context;
        context.contextDirectory = files->directory();
        context.tab = files;
        BuildDepotContextMenu(view, *menu, context, depotItems);

        if (menu->childrenList())
        {
            auto ret = RefNew<ui::PopupWindow>();
            ret->attachChild(menu);
            return ret;
        }
    }

    return nullptr;
}

StringBuf AssetBrowserDirContentModel::displayContent(const ui::ModelIndex& item, int colIndex/* = 0*/) const
{
    if (auto entry = item.unsafe<Entry>())
    {
        StringBuilder txt;

        if (auto* file = rtti_cast<ManagedFile>(entry->item))
        {
            txt << "  [img:page]  ";

            if (m_rootDir)
                txt << file->depotPath().stringAfterFirst(m_rootDir->depotPath());
            else
                txt << file->depotPath();

            txt << "  [i][color:#888](" << file->fileFormat().description() << ")[/i][/color]";
        }
        else if (auto* dir = rtti_cast<ManagedDirectory>(entry->item))
        {
            auto parentDir = rootDir() && (dir == rootDir()->parentDirectory());

            if (parentDir)
                txt << "  [img:arrow_up]  ";
            else
                txt << "  [img:folder]  ";

            txt << dir->name();

            if (!parentDir)
                txt << "  [i][color:#888](Directory)[/i][/color]";
        }

        return txt.toString();
    }

    return StringBuf::EMPTY();
}

ui::DragDropDataPtr AssetBrowserDirContentModel::queryDragDropData(const input::BaseKeyFlags& keys, const ui::ModelIndex& item)
{
    auto entry  = item.unsafe<Entry>();

    if (auto file = rtti_cast<ManagedFile>(entry->item))
    {
        return RefNew<AssetBrowserFileDragDrop>(file);
    }
    else if (auto dir = rtti_cast<ManagedDirectory>(entry->item))
    {
        return RefNew<AssetBrowserDirectoryDragDrop>(dir);
    }

    return nullptr;
}
    
void AssetBrowserDirContentModel::visualize(const ui::ModelIndex& id, int columnCount, ui::ElementPtr& content) const
{
    if (columnCount == 0)
    {
        if (auto* depotItem = item(id))
        {
            if (auto* file = rtti_cast<ManagedFilePlaceholder>(depotItem))
            {
                if (!content)
                    content = RefNew<AssetBrowserPlaceholderFileVis>(file, m_iconSize);
            }
            else if (auto* dir = rtti_cast<ManagedDirectoryPlaceholder>(depotItem))
            {
                if (!content)
                    content = RefNew<AssetBrowserPlaceholderDirectoryVis>(dir, m_iconSize);
            }
            else if (auto* file = rtti_cast<ManagedFile>(depotItem))
            {
                if (!content)
                    content = RefNew<AssetBrowserFileVis>(file, m_iconSize);
            }
            else if (auto* dir = rtti_cast<ManagedDirectory>(depotItem))
            {
                if (!content)
                {
                    auto parentDir = m_rootDir && (dir == m_rootDir->parentDirectory());
                    content = RefNew<AssetBrowserDirectoryVis>(dir, m_iconSize, parentDir);
                }
            }
        }
    }
    else
    {
        return ui::IAbstractItemModel::visualize(id, columnCount, content);
    }
}

//--

END_BOOMER_NAMESPACE(ed)