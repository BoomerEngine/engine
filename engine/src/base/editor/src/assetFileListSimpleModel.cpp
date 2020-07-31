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

#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiCheckBox.h"

namespace ed
{
    
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

    Array<ManagedItem*> AssetItemsSimpleListModel::exportList(bool checkedOnly) const
    {
        Array<ManagedItem*> ret;
        ret.reserve(m_items.size());

        for (const auto* item : m_items)
            if (item->checked || !checkedOnly)
                ret.pushBack(item->item);

        return ret;
    }

    AssetItemsSimpleListModel::Item* AssetItemsSimpleListModel::itemForIndex(const ui::ModelIndex& id) const
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

    //--

} // ed