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

namespace ed
{

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
                beingInsertRows(ui::ModelIndex(), m_items.size(), 1);

                auto entry = CreateSharedPtr<Entry>();
                entry->m_item = item;
                entry->m_itemType = TypeFromItem(item);
                m_items.pushBack(entry);

                endInsertRows();

                index = this->index(item);
            }
        }

        return index;
    }

    void AssetBrowserDirContentModel::handleDestroyItemRepresentation(const ManagedItemPtr& item)
    {
        if (auto index = this->index(item))
        {
            beingRemoveRows(ui::ModelIndex(), index.row(), 1);
            m_items.erase(index.row(), 1);
            endRemoveRows();
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
        if (!file->fileFormat().cookableOutputs().empty())
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
                auto entry = CreateSharedPtr<Entry>();
                entry->m_item = dir->parentDirectory();
                entry->m_itemType = TYPE_PARENT_DIRECTORY;
                entry->m_customName = "..";
                m_items.pushBack(entry);
            }

            for (auto& dir : dir->directories())
            {
                if (!dir->isDeleted())
                {
                    auto entry = CreateSharedPtr<Entry>();
                    entry->m_item = dir;
                    entry->m_itemType = TYPE_DIRECTORY;
                    m_items.pushBack(entry);
                }
            }

            for (auto& file : dir->files())
            {
                if (!file->isDeleted())
                {
                    if (canDisplayFile(file))
                    {
                        auto entry = CreateSharedPtr<Entry>();
                        entry->m_item = file;
                        entry->m_itemType = TYPE_FILE;
                        m_items.pushBack(entry);
                    }
                }
            }
        }
    }

    ManagedDirectory* AssetBrowserDirContentModel::directory(const ui::ModelIndex& index) const
    {
        auto entry  = index.unsafe<Entry>();
        return entry ? rtti_cast<ManagedDirectory>(entry->m_item) : nullptr;
    }

    ManagedFile* AssetBrowserDirContentModel::file(const ui::ModelIndex& index) const
    {
        auto entry  = index.unsafe<Entry>();
        return entry ? rtti_cast<ManagedFile>(entry->m_item) : nullptr;
    }

    ManagedItem* AssetBrowserDirContentModel::item(const ui::ModelIndex& index) const
    {
        auto entry  = index.unsafe<Entry>();
        return entry ? entry->m_item : nullptr;
    }

    Array<ManagedFile*> AssetBrowserDirContentModel::files(const Array<ui::ModelIndex>& indices) const
    {
        Array<ManagedFile*> files;
        files.reserve(indices.size());

        for (auto& index : indices)
            if (auto entry  = index.unsafe<Entry>())
                if (auto file = rtti_cast<ManagedFile>(entry->m_item))
                    files.emplaceBack(file);

        return files;
    }

    Array<ManagedItem*> AssetBrowserDirContentModel::items(const Array<ui::ModelIndex>& indices) const
    {
        Array<ManagedItem*> files;
        files.reserve(indices.size());

        for (auto& index : indices)
            if (auto entry  = index.unsafe<Entry>())
                files.emplaceBack(entry->m_item);

        return files;
    }

    ui::ModelIndex AssetBrowserDirContentModel::index(const ManagedItem* ptr) const
    {
        if (!ptr)
            return ui::ModelIndex();

        /*if (m_observedDirs.contains(ptr->parentDirectory()))
            return ui::ModelIndex();*/

        for (int i = 0; i <= m_items.lastValidIndex(); ++i)
            if (m_items[i]->m_item == ptr)
                return ui::ModelIndex(this, i, 0, m_items[i]);

        return ui::ModelIndex();
    }

    uint32_t AssetBrowserDirContentModel::rowCount(const ui::ModelIndex& parent) const
    {
        if (!parent)
            return m_items.size();
        return 0;
    }

    bool AssetBrowserDirContentModel::hasChildren(const ui::ModelIndex& parent) const
    {
        return false;
    }

    bool AssetBrowserDirContentModel::hasIndex(int row, int col, const ui::ModelIndex& parent) const
    {
        if (parent)
            return false;

        if (row >= 0 && row <= m_items.lastValidIndex())
            return true;

        return false;
    }

    ui::ModelIndex AssetBrowserDirContentModel::parent(const ui::ModelIndex& item) const
    {
        return ui::ModelIndex();
    }

    ui::ModelIndex AssetBrowserDirContentModel::index(int row, int column, const ui::ModelIndex& parent) const
    {
        if (!parent && row >= 0 && row <= m_items.lastValidIndex())
            return ui::ModelIndex(this, row, 0, m_items[row]);

        return ui::ModelIndex();
    }

    bool AssetBrowserDirContentModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex) const
    {
        auto firstEntry  = first.unsafe<Entry>();
        auto secondEntry  = second.unsafe<Entry>();

        if (firstEntry->m_itemType != secondEntry->m_itemType)
            return firstEntry->m_itemType < secondEntry->m_itemType;

        return firstEntry->m_item->name() < secondEntry->m_item->name();
    }

    bool AssetBrowserDirContentModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex) const
    {
        auto entry = id.unsafe<Entry>();
        if (entry)
            return filter.testString(entry->m_item->name());
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
            auto menu = CreateSharedPtr<ui::MenuButtonContainer>();
            DepotMenuContext context;
            context.contextDirectory = files->directory();
            BuildDepotContextMenu(view, *menu, context, depotItems);

            if (menu->childrenList())
            {
                auto ret = CreateSharedPtr<ui::PopupWindow>();
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

            if (auto* file = rtti_cast<ManagedFile>(entry->m_item))
            {
                txt << "  [img:page]  ";

                if (m_rootDir)
                    txt << file->depotPath().stringAfterFirst(m_rootDir->depotPath());
                else
                    txt << file->depotPath();

                txt << "  [i][color:#888](" << file->fileFormat().description() << ")[/i][/color]";
            }
            else if (auto* dir = rtti_cast<ManagedDirectory>(entry->m_item))
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

        if (auto file = rtti_cast<ManagedFile>(entry->m_item))
        {
            return CreateSharedPtr<AssetBrowserFileDragDrop>(file);
        }
        else if (auto dir = rtti_cast<ManagedDirectory>(entry->m_item))
        {
            return CreateSharedPtr<AssetBrowserDirectoryDragDrop>(dir);
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
                        content = CreateSharedPtr<AssetBrowserPlaceholderFileVis>(file, m_iconSize);
                }
                else if (auto* dir = rtti_cast<ManagedDirectoryPlaceholder>(depotItem))
                {
                    if (!content)
                        content = CreateSharedPtr<AssetBrowserPlaceholderDirectoryVis>(dir, m_iconSize);
                }
                else if (auto* file = rtti_cast<ManagedFile>(depotItem))
                {
                    if (!content)
                        content = CreateSharedPtr<AssetBrowserFileVis>(file, m_iconSize);
                }
                else if (auto* dir = rtti_cast<ManagedDirectory>(depotItem))
                {
                    if (!content)
                    {
                        auto parentDir = m_rootDir && (dir == m_rootDir->parentDirectory());
                        content = CreateSharedPtr<AssetBrowserDirectoryVis>(dir, m_iconSize, parentDir);
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

} // ed