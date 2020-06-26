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

        if (nullptr != m_model)
        {
            uint32_t numRows = m_model->rowCount();
            for (uint32_t i = 0; i < numRows; ++i)
            {
                if (auto index = m_model->index(i, 0))
                {
                    attachViewElement(index);
                }
            }

            rebuildSortedOrder();
            rebuildDisplayList();
        }
    }

    void ListView::modelRowsAboutToBeAdded(const ModelIndex& parent, int first, int count)
    {
        TBaseClass::modelRowsAboutToBeAdded(parent, first, count);
    }

    void ListView::modelRowsAdded(const ModelIndex& parent, int first, int count)
    {
        TBaseClass::modelRowsAdded(parent, first, count);

        ViewItem* parentItem = nullptr;
        if (resolveItemFromModelIndex(parent, parentItem) && count)
        {
            ModelIndexReindexerInsert reindexer(parent, first, count);

            auto& children = parentItem ? parentItem->m_children.m_orderedChildren : m_mainRows.m_orderedChildren;
            ASSERT(first <= (int)children.size());

            for (int i = first; i <= children.lastValidIndex(); ++i)
            {
                auto item = children[i];
                reindexer.reindex(item->m_index, item->m_index);
            }

            children.insertWith(first, count, nullptr);

            for (int i = 0; i < count; ++i)
            {
                auto item = m_pool.alloc();

                item->m_index = m_model->index(i + first, 0, parent);
                item->m_parent = parentItem;
                children[i + first] = item;

                visualizeViewElement(item);
            }

            rebuildSortedOrder();
            rebuildDisplayList();
        }
    }

    void ListView::modelRowsAboutToBeRemoved(const ModelIndex& parent, int first, int count)
    {
        TBaseClass::modelRowsAboutToBeRemoved(parent, first, count);
    }

    void ListView::modelRowsRemoved(const ModelIndex& parent, int first, int count)
    {
        ViewItem* parentItem = nullptr;
        if (resolveItemFromModelIndex(parent, parentItem) && count)
        {
            ModelIndexReindexerRemove reindexer(parent, first, count);

            auto& children = parentItem ? parentItem->m_children : m_mainRows;
            ASSERT(first + count <= (int)children.m_orderedChildren.size());

            for (int i = count-1; i >= 0; --i)
            {
                auto* item = children.m_orderedChildren[first + i];
                destroyViewElement(item);
            }

            //children.m_orderedChildren.erase(first, count);

            for (int i = first; i <= children.m_orderedChildren.lastValidIndex(); ++i)
            {
                auto* item = children.m_orderedChildren[i];
                reindexer.reindex(item->m_index, item->m_index);
            }

            buildSortedList(children.m_orderedChildren, children.m_displayOrder);
        }

        TBaseClass::modelRowsRemoved(parent, first, count);
    }

    void ListView::destroyViewElement(ViewItem* item)
    {
        if (nullptr == item)
            return;

        {
            auto& parentChildren = item->m_parent ? item->m_parent->m_children : m_mainRows;
            parentChildren.m_orderedChildren.remove(item);
            parentChildren.m_displayOrder.remove(item);
        }

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
