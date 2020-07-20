/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "assetBrowser.h"
#include "assetBrowserContextMenu.h"
#include "assetBrowserTabFiles.h"
#include "assetFileListModel.h"
#include "assetFileSmallImportIndicator.h"

#include "managedDirectory.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "managedDepot.h"
#include "managedFileNativeResource.h"

#include "base/ui/include/uiImage.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiWindow.h"
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

    void AssetBrowserDirContentModel::handleCreateItemRepresentation(const ManagedItemPtr& item)
    {
        if (m_observedDirs.contains(item->parentDirectory()))
        {
            if (auto index = this->index(item))
            {
            }
            else if (canDisplayItem(item))
            {
                beingInsertRows(ui::ModelIndex(), m_items.size(), 1);

                auto entry = CreateSharedPtr<Entry>();
                entry->m_item = item;
                entry->m_itemType = item->is<ManagedDirectory>() ? 1 : 2;
                m_items.pushBack(entry);

                endInsertRows();
            }
        }
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
                entry->m_itemType = 0;
                entry->m_customName = "..";
                m_items.pushBack(entry);
            }

            for (auto& dir : dir->directories())
            {
                if (!dir->isDeleted())
                {
                    auto entry = CreateSharedPtr<Entry>();
                    entry->m_item = dir;
                    entry->m_itemType = 1;
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
                        entry->m_itemType = 2;
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

    void AssetBrowserDirContentModel::collectItems(const Array<ui::ModelIndex>& indices, AssetItemList& outList, bool recursive) const
    {
        for (auto& index : indices)
            if (auto entry = index.unsafe<Entry>())
                outList.collect(entry->m_item, recursive);
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
            BuildDepotContextMenu(*menu, files, depotItems);

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

    //--

    AssetBrowserFileVisItem::AssetBrowserFileVisItem(ManagedItem* item, uint32_t size, const StringView<char> customText)
        : m_item(item)
        , m_size(size)
    {
        hitTest(true);

        m_icon = createChild<ui::Image>(item->typeThumbnail());
        m_icon->customMargins(3.0f);
        m_icon->customMaxSize(size, size);
        m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

        if (auto file = rtti_cast<ManagedFile>(item))
        {
            if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            {
                m_importIndicator = m_icon->createChild<AssetFileSmallImportIndicator>(nativeFile);
                m_importIndicator->customMargins(20, 20, 0, 0);
                m_importIndicator->customHorizontalAligment(ui::ElementHorizontalLayout::Left);
                m_importIndicator->customVerticalAligment(ui::ElementVerticalLayout::Top);
                m_importIndicator->visibility(size >= 80);
                m_importIndicator->overlay(true);
            }

            StringBuilder displayText;
            for (const auto& tag : file->fileFormat().tags())
            {
                displayText.appendf("[tag:{}]", tag.color);

                if (tag.baked)
                    displayText.appendf("[img:cog] ");
                else
                    displayText.appendf("[img:file_empty_edit] ");

                if (tag.color.luminanceSRGB() > 0.5f)
                {
                    displayText << "[color:#000]";
                    displayText << tag.name;
                    displayText << "[/color]";
                }
                else
                {
                    displayText << tag.name;
                }

                displayText << "[/tag]";
                displayText << "[br]";
            }

            displayText << file->name().stringBeforeLast(".");

            m_label = createChild<ui::TextLabel>(displayText.toString());
        }
        else
        {
            m_label = createChild<ui::TextLabel>(customText ? customText : item->name());
        }

        m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
        m_label->customMargins(3.0f);
    }

    void AssetBrowserFileVisItem::refreshFileData()
    {

    }

    void AssetBrowserFileVisItem::resizeIcon(uint32_t size)
    {
        if (m_size != size)
        {
            if (m_icon)
                m_icon->customMaxSize(size, size);
            if (m_importIndicator)
                m_importIndicator->visibility(size >= 80);
            m_size = size;
        }
    }

    //--

    void AssetBrowserDirContentModel::visualize(const ui::ModelIndex& id, int columnCount, ui::ElementPtr& content) const
    {
        if (columnCount == 0)
        {
            if (auto* depotItem = item(id))
            {
                if (auto* file = rtti_cast<ManagedFile>(depotItem))
                {
                    if (!content)
                        content = CreateSharedPtr<AssetBrowserFileVisItem>(file, m_iconSize);
                }
                else if (auto* dir = rtti_cast<ManagedDirectory>(depotItem))
                {
                    if (!content)
                    {
                        auto parentDir = m_rootDir && (dir == m_rootDir->parentDirectory());
                        content = CreateSharedPtr<AssetBrowserFileVisItem>(dir, m_iconSize, parentDir ? ".." : "");
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

    AssetItemsSimpleListModel::AssetItemsSimpleListModel()
    {}

    AssetItemsSimpleListModel::~AssetItemsSimpleListModel()
    {
        m_items.clearPtr();
        m_itemMap.clear();
    }

    void AssetItemsSimpleListModel::clear()
    {
        beingRemoveRows(ui::ModelIndex(), 0, m_items.size());
        m_items.clearPtr();
        m_itemMap.clear();
        endRemoveRows();
    }

    ui::ModelIndex AssetItemsSimpleListModel::index(int index) const
    {
        if (index >= 0 && index <= m_items.lastValidIndex())
            return ui::ModelIndex(this, index, 0);
        return ui::ModelIndex();
    }

    ui::ModelIndex AssetItemsSimpleListModel::index(ManagedItem* item) const
    {
        int localItemIndex = -1;
        m_itemMap.find(item, localItemIndex);
        return index(localItemIndex);
    }

    void AssetItemsSimpleListModel::removeItem(ManagedItem* item)
    {
        if (item)
        {
            int localItemIndex = 0;
            if (m_itemMap.find(item, localItemIndex))
            {
                auto* localItem = m_items[localItemIndex];

                beingRemoveRows(ui::ModelIndex(), localItemIndex, 1);
                m_items.erase(localItemIndex);
                endRemoveRows();

                MemDelete(localItem);
                m_itemMap.remove(item);

                for (auto& val : m_itemMap.values())
                    if (val > localItemIndex)
                        val -= 1;
            }
        }
    }

    void AssetItemsSimpleListModel::addItem(ManagedItem* item, bool checked /*= true*/, StringView<char> comment /*= ""*/)
    {
        if (item)
        {
            int localItemIndex = 0;
            if (m_itemMap.find(item, localItemIndex))
            {
                auto* localItem = m_items[localItemIndex];
                localItem->checked = checked;
                localItem->item = item;
                localItem->comment = StringBuf(comment);
                localItem->name = item->depotPath();

                requestItemUpdate(index(localItemIndex));
            }
            else
            {
                int localItemIndex = m_items.size();

                auto localItem = MemNew(Item);
                localItem->checked = checked;// && (localItemIndex & 1);
                localItem->item = item;
                localItem->comment = StringBuf(comment);
                localItem->name = item->depotPath();

                beingInsertRows(ui::ModelIndex(), localItemIndex, 1);
                m_items.pushBack(localItem);
                m_itemMap[item] = localItemIndex;
                endInsertRows();
            }
        }
    }

    void AssetItemsSimpleListModel::comment(ManagedItem* item, StringView<char> comment)
    {
        int localItemIndex = 0;
        if (m_itemMap.find(item, localItemIndex))
        {
            auto* localItem = m_items[localItemIndex];
            if (localItem->comment != comment)
            {
                localItem->comment = StringBuf(comment);
                requestItemUpdate(index(localItemIndex));
            }
        }
    }

    bool AssetItemsSimpleListModel::checked(ManagedItem* item) const
    {
        int localItemIndex = 0;
        if (m_itemMap.find(item, localItemIndex))
        {
            auto* localItem = m_items[localItemIndex];
            return localItem->checked;
        }

        return false;
    }

    void AssetItemsSimpleListModel::checked(ManagedItem* item, bool selected) const
    {
        int localItemIndex = 0;
        if (m_itemMap.find(item, localItemIndex))
        {
            auto* localItem = m_items[localItemIndex];
            if (localItem->checked != selected)
            {
                localItem->checked = selected;
                requestItemUpdate(index(localItemIndex));
            }
        }
    }

    bool AssetItemsSimpleListModel::checked(const ui::ModelIndex& item) const
    {
        if (item.row() >= 0 && item.row() <= m_items.lastValidIndex())
            return m_items[item.row()]->checked;
        return false;
    }

    void AssetItemsSimpleListModel::checked(const ui::ModelIndex& item, bool checked) const
    {
        if (item.row() >= 0 && item.row() <= m_items.lastValidIndex())
        {
            if (m_items[item.row()]->checked != checked)
            {
                m_items[item.row()]->checked = checked;
                requestItemUpdate(item);
            }
        }
    }

    Array<ManagedItem*> AssetItemsSimpleListModel::checked() const
    {
        Array<ManagedItem*> ret;

        ret.reserve(m_items.size());

        for (const auto* item : m_items)
            if (item->checked)
                ret.pushBack(item->item);

        return ret;
    }

    AssetItemsSimpleListModel::Item* AssetItemsSimpleListModel::itemForIndex(const ui::ModelIndex& index) const
    {
        if (id.row() >= 0 && id.row() <= m_items.lastValidIndex())
            return m_items[id.row()];
        return nullptr;
    }

    StringBuf AssetItemsSimpleListModel::content(const ui::ModelIndex& id, int colIndex /*= 0*/) const
    {
        if (const auto* item = itemForIndex(id))
            return item->name;
        return StringBuf::EMPTY();
    }

    uint32_t AssetItemsSimpleListModel::size() const
    {
        return m_items.size();
    }

    void AssetItemsSimpleListModel::visualize(const ui::ModelIndex& id, int columnCount, ui::ElementPtr& content) const
    {
        if (id.row() >= 0 && id.row() <= m_items.lastValidIndex())
        {
            auto* item = m_items[id.row()];

            if (!content)
            {
                content = CreateSharedPtr<ui::IElement>();
                content->customPadding(2);
                content->layoutHorizontal();

                {
                    auto elem = content->createNamedChild<ui::CheckBox>("State"_id);
                    elem->customMargins(5, 0, 5, 0);
                    elem->state(item->checked);
                    elem->bind("OnClick"_id) = [this, item](ui::CheckBox* box, ui::IElement*) {
                        item->checked = box->stateBool();
                    };
                }

                {
                    auto elem = content->createNamedChild<ui::TextLabel>("Name"_id);
                    elem->text(item->name);
                }

                {
                    auto elem = content->createNamedChild<ui::TextLabel>("Comment"_id, "");
                    elem->text(item->comment);
                }                
            }
            else
            {
                if (auto elem = content->findChildByName<ui::CheckBox>("State"_id))
                    elem->state(item->checked);

                if (auto elem = content->findChildByName<ui::TextLabel>("Name"_id))
                    elem->text(item->name);

                if (auto elem = content->findChildByName<ui::TextLabel>("Comment"_id))
                    elem->text(item->comment);
            }
        }
    }

    //--

} // ed