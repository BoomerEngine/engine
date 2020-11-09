/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#include "build.h"
#include "uiListView.h"
#include "uiTextLabel.h"

#include "base/input/include/inputStructures.h"

namespace ui
{

    //---

    RTTI_BEGIN_TYPE_CLASS(ListView);
        RTTI_METADATA(ElementClassNameMetadata).name("ListView");
    RTTI_END_TYPE();

    ListView::ListView()
    {
        layoutMode(LayoutMode::Vertical);
        verticalScrollMode(ScrollMode::Auto);
    }

    void ListView::modelReset()
    {
        discardAllElements();

        DEBUG_CHECK(m_mainRows.m_orderedChildren.empty());
        DEBUG_CHECK(m_mainRows.m_displayOrder.empty());

        if (nullptr != m_model)
        {
            base::Array<ModelIndex> initialChildren;
            m_model->children(ModelIndex(), initialChildren);

            for (const auto& index : initialChildren)
            {
                auto* item = m_pool.alloc();
                item->m_index = index;
                m_mainRows.m_orderedChildren.pushBack(item);

                visualizeViewElement(item);
            }

            rebuildSortedOrder();
            rebuildDisplayList();
        }
    }

    void ListView::modelItemsAdded(const ModelIndex& parent, const base::Array<ModelIndex>& items)
    {
        TBaseClass::modelItemsAdded(parent, items);

        ViewItem* parentItem = nullptr;
        if (resolveItemFromModelIndex(parent, parentItem))
        {
            auto& children = parentItem ? parentItem->m_children.m_orderedChildren : m_mainRows.m_orderedChildren;

            for (const auto& itemIndex : items)
            {
                auto item = m_pool.alloc();

                item->m_index = itemIndex;
                item->m_parent = parentItem;
                children.pushBack(item);

                visualizeViewElement(item);
            }

            rebuildSortedOrder();
            rebuildDisplayList();
        }
    }

    void ListView::modelItemsRemoved(const ModelIndex& parent, const base::Array<ModelIndex>& items)
    {
        TBaseClass::modelItemsRemoved(parent, items);

        ViewItem* parentItem = nullptr;
        if (resolveItemFromModelIndex(parent, parentItem))
        {
            auto& children = parentItem ? parentItem->m_children : m_mainRows;

            for (const auto& itemIndex : items)
            {
                if (auto* childItem = children.findItem(itemIndex))
                {
                    children.m_orderedChildren.remove(childItem);
                    children.m_displayOrder.remove(childItem);

                    destroyViewElement(childItem);
                }
            }

            // TODO: this can be unnecessary
            //buildSortedList(children.m_orderedChildren, children.m_displayOrder);
        }
    }

    void ListView::destroyViewElement(ViewItem* item)
    {
        if (nullptr == item)
            return;

        auto& parentChildren = item->m_parent ? item->m_parent->m_children : m_mainRows;
        DEBUG_CHECK(!parentChildren.m_orderedChildren.contains(item));
        DEBUG_CHECK(!parentChildren.m_displayOrder.contains(item));

        unvisualizeViewElement(item);
        m_displayList.unlink(item);

        auto children = std::move(item->m_children.m_orderedChildren);
        item->m_children.m_orderedChildren.reset();
        item->m_children.m_displayOrder.reset();

        for (auto* child : children)
            destroyViewElement(child);
    }

    void ListView::unvisualizeViewElement(ViewItem* item)
    {
        if (item->m_content)
        {
            detachChild(item->m_content);
            item->m_content.reset();
        }
    }

    void ListView::visualizeViewElement(ViewItem* item)
    {
        auto oldContent = item->m_content;

        m_model->visualize(item->m_index, m_columnCount, item->m_content);

        if (oldContent != item->m_content)
        {
            if (oldContent)
                detachChild(oldContent);

            if (item->m_content)
            {
                item->m_content->hitTest(true);
                item->m_content->name("Item"_id);

                if (layoutMode() == LayoutMode::Vertical)
                    item->m_content->customHorizontalAligment(ElementHorizontalLayout::Expand);
                else if (layoutMode() == LayoutMode::Horizontal)
                    item->m_content->customVerticalAligment(ElementVerticalLayout::Expand);

                attachChild(item->m_content);
            }
        }
    }

    void ListView::updateItem(ViewItem* item)
    {
        if (item && item->m_content)
            m_model->visualize(item->m_index, m_columnCount, item->m_content);
    }

    //--

} // ui
