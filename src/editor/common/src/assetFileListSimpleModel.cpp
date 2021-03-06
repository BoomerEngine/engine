/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "assetFileListSimpleModel.h"

#include "managedDirectory.h"
#include "managedFile.h"

#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiCheckBox.h"
#include "managedFileFormat.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)
    
//--

AssetItemsSimpleListModel::AssetItemsSimpleListModel()
{}

AssetItemsSimpleListModel::~AssetItemsSimpleListModel()
{
    m_items.clear();
    m_itemMap.clear();
}

bool AssetItemsSimpleListModel::hasChildren(const ui::ModelIndex& parent) const
{
    return !parent && !m_items.empty();
}

ui::ModelIndex AssetItemsSimpleListModel::parent(const ui::ModelIndex& item) const
{
    return ui::ModelIndex();
}

void AssetItemsSimpleListModel::children(const ui::ModelIndex& parent, Array<ui::ModelIndex>& outChildrenIndices) const
{
    if (!parent)
    {
        outChildrenIndices.reserve(m_items.size());

        for (const auto& item : m_items)
            outChildrenIndices.pushBack(item->index);
    }
}

void AssetItemsSimpleListModel::clear()
{
    m_items.clear();
    m_itemMap.clear();
    reset();
}

ui::ModelIndex AssetItemsSimpleListModel::index(ManagedItem* item) const
{
    Item* localItem = nullptr;
    if (m_itemMap.find(item, localItem))
        return localItem->index;
    return ui::ModelIndex();
}

void AssetItemsSimpleListModel::removeItem(ManagedItem* item)
{
    if (item)
    {
        Item* localItem = nullptr;
        if (m_itemMap.find(item, localItem))
        {
            RefPtr<Item> itemRef = AddRef(localItem);
            m_items.remove(localItem);
            m_itemMap.remove(item);

            notifyItemRemoved(ui::ModelIndex(), itemRef->index);
        }
    }
}

void AssetItemsSimpleListModel::addItem(ManagedItem* item, bool checked /*= true*/, StringView comment /*= ""*/)
{
    if (item)
    {
        Item* localItem = nullptr;
        if (m_itemMap.find(item, localItem))
        {
            localItem->checked = checked;
            localItem->item = item;
            localItem->comment = StringBuf(comment);
            localItem->name = item->depotPath();

            requestItemUpdate(localItem->index);
        }
        else
        {
            int localItemIndex = m_items.size();

            auto localItem = RefNew<Item>();
            localItem->index = ui::ModelIndex(this, localItem);
            localItem->checked = checked;// && (localItemIndex & 1);
            localItem->item = item;
            localItem->comment = StringBuf(comment);
            localItem->name = item->depotPath();

            m_items.pushBack(localItem);
            m_itemMap[item] = localItem;
                
            notifyItemAdded(ui::ModelIndex(), localItem->index);
        }
    }
}

void AssetItemsSimpleListModel::comment(ManagedItem* item, StringView comment)
{
    Item* localItem = nullptr;
    if (m_itemMap.find(item, localItem))
    {
        if (localItem->comment != comment)
        {
            localItem->comment = StringBuf(comment);
            requestItemUpdate(localItem->index);
        }
    }
}

bool AssetItemsSimpleListModel::checked(ManagedItem* item) const
{
    Item* localItem = nullptr;
    if (m_itemMap.find(item, localItem))
        return localItem->checked;

    return false;
}

void AssetItemsSimpleListModel::checked(ManagedItem* item, bool selected) const
{
    Item* localItem = nullptr;
    if (m_itemMap.find(item, localItem))
    {
        if (localItem->checked != selected)
        {
            localItem->checked = selected;
            requestItemUpdate(localItem->index);
        }
    }
}

bool AssetItemsSimpleListModel::checked(const ui::ModelIndex& item) const
{
    if (item.model() == this)
        if (auto* entry = item.unsafe<Item>())
            return entry->checked;

    return false;
}

void AssetItemsSimpleListModel::checked(const ui::ModelIndex& item, bool checked) const
{
    if (item.model() == this)
    {
        if (auto* entry = item.unsafe<Item>())
        {
            if (entry->checked != checked)
            {
                entry->checked = checked;
                requestItemUpdate(item);
            }
        }
    }
}

Array<ManagedItem*> AssetItemsSimpleListModel::exportList(bool checkedOnly) const
{
    Array<ManagedItem*> ret;
    ret.reserve(m_items.size());

    for (const auto& item : m_items)
        if (item->checked || !checkedOnly)
            ret.pushBack(item->item);

    return ret;
}

bool AssetItemsSimpleListModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex) const
{
    if (first.model() == this && second.model() == this)
    {
        const auto* firstData = first.unsafe<Item>();
        const auto* secondData = second.unsafe<Item>();
        if (firstData && secondData)
            return firstData->name < secondData->name;
    }

    return first < second;
}

bool AssetItemsSimpleListModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex) const
{
    if (id.model() == this)
    {
        if (auto* data = id.unsafe<Item>())
        {
            return filter.testString(data->name.view());
        }
    }

    return false;
}

void AssetItemsSimpleListModel::visualize(const ui::ModelIndex& id, int columnCount, ui::ElementPtr& content) const
{
    if (id.model() == this)
    {
        if (auto* item = id.unsafe<Item>())
        {
            if (!content)
            {
                content = RefNew<ui::IElement>();
                content->customPadding(2);
                content->layoutHorizontal();

                {
                    auto elem = content->createNamedChild<ui::CheckBox>("State"_id);
                    elem->customMargins(5, 0, 5, 0);
                    elem->state(item->checked);
                    elem->bind(ui::EVENT_CLICKED) = [this, item](ui::CheckBox* box) {
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
}

//--

AssetPlainFilesSimpleListModel::AssetPlainFilesSimpleListModel()
{}

AssetPlainFilesSimpleListModel::~AssetPlainFilesSimpleListModel()
{
    m_items.clear();
    m_itemMap.clear();
}

bool AssetPlainFilesSimpleListModel::hasChildren(const ui::ModelIndex& parent) const
{
    return !parent && !m_items.empty();
}

ui::ModelIndex AssetPlainFilesSimpleListModel::parent(const ui::ModelIndex& item) const
{
    return ui::ModelIndex();
}

void AssetPlainFilesSimpleListModel::children(const ui::ModelIndex& parent, Array<ui::ModelIndex>& outChildrenIndices) const
{
    if (!parent)
    {
        outChildrenIndices.reserve(m_items.size());

        for (const auto& item : m_items)
            outChildrenIndices.pushBack(item->index);
    }
}

void AssetPlainFilesSimpleListModel::clear()
{
    m_items.clear();
    m_itemMap.clear();
    reset();
}

ui::ModelIndex AssetPlainFilesSimpleListModel::index(ManagedFile* item) const
{
    Item* localItem = nullptr;
    if (m_itemMap.find(item, localItem))
        return localItem->index;
    return ui::ModelIndex();
}

ManagedFile* AssetPlainFilesSimpleListModel::file(const ui::ModelIndex& index) const
{
    if (index.model() == this)
    {
        if (auto* localItem = index.unsafe<Item>())
            return localItem->file;
    }

    return nullptr;
}

void AssetPlainFilesSimpleListModel::removeFile(ManagedFile* item)
{
    if (item)
    {
        Item* localItem = nullptr;
        if (m_itemMap.find(item, localItem))
        {
            RefPtr<Item> itemRef = AddRef(localItem);
            m_items.remove(localItem);
            m_itemMap.remove(item);

            notifyItemRemoved(ui::ModelIndex(), itemRef->index);
        }
    }
}

void AssetPlainFilesSimpleListModel::addFile(ManagedFile* file, bool locked)
{
    Item* localItem = nullptr;
    if (!m_itemMap.find(file, localItem))
    {
        int localItemIndex = m_items.size();

        auto localItem = RefNew<Item>();
        localItem->index = ui::ModelIndex(this, localItem);
        localItem->file = file;
        localItem->name = StringBuf(file->name().view().fileStem());
        localItem->modified = file->isModified();
        localItem->directory = file->parentDirectory()->depotPath();
        localItem->locked = locked;

        StringBuilder displayText;
        file->fileFormat().printTags(displayText, " ");
        localItem->tags = displayText.toString();

        m_items.pushBack(localItem);
        m_itemMap[file] = localItem;

        notifyItemAdded(ui::ModelIndex(), localItem->index);
    }
}
    
bool AssetPlainFilesSimpleListModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex) const
{
    if (first.model() == this && second.model() == this)
    {
        const auto* firstData = first.unsafe<Item>();
        const auto* secondData = second.unsafe<Item>();
        if (firstData && secondData)
            return firstData->name < secondData->name;
    }

    return first < second;
}

bool AssetPlainFilesSimpleListModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex) const
{
    if (id.model() == this)
    {
        if (auto* data = id.unsafe<Item>())
        {
            return filter.testString(data->name.view());
        }
    }

    return false;
}

void AssetPlainFilesSimpleListModel::visualize(const ui::ModelIndex& id, int columnCount, ui::ElementPtr& content) const
{
    if (id.model() == this)
    {
        if (auto* item = id.unsafe<Item>())
        {
            if (!content)
            {
                content = RefNew<ui::IElement>();
                content->customPadding(2);
                content->layoutMode(ui::LayoutMode::Columns);

                {
                    auto elem = content->createNamedChild<ui::TextLabel>("Modified"_id);
                    elem->customMargins(5, 0, 5, 0);
                    elem->text(item->modified ? "[img:save]" : "");
                }

                {
                    auto elem = content->createNamedChild<ui::TextLabel>("Locked"_id);
                    elem->customMargins(5, 0, 5, 0);
                    elem->text(item->locked ? "[img:lock]" : "");
                }

                {
                    auto elem = content->createNamedChild<ui::TextLabel>("Tags"_id, "");
                    elem->text(item->tags);
                }

                {
                    auto elem = content->createNamedChild<ui::TextLabel>("Name"_id);
                    elem->text(item->name);
                }

                {
                    auto elem = content->createNamedChild<ui::TextLabel>("Directory"_id, "");
                    elem->text(item->directory);
                }
            }
        }
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)
