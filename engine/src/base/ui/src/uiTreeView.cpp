/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#include "build.h"
#include "uiTreeView.h"
#include "uiTextLabel.h"
#include "uiInputAction.h"
#include "uiButton.h"

#include "base/input/include/inputStructures.h"

namespace ui
{
    //---

    class TreeViewItem : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TreeViewItem, ui::IElement);

    public:
        TreeViewItem(uint8_t depth = 0)
            : m_depth(depth)
        {
            layoutMode(LayoutMode::Horizontal);

            // offset by depth in the tree TODO: styling!!
            auto margins = Offsets(20.0f * depth, 0.0f, 0.0f, 0.0f);

            m_expandButton = createNamedChild<ui::Button>("Button"_id, ButtonModeBit::EventOnClick);
            m_expandButton->createNamedChild<ui::TextLabel>("ExpandIcon"_id);
            m_expandButton->customMargins(margins);
        }

        base::RefPtr<ui::Button> m_expandButton;
        uint8_t m_depth;
    };

    RTTI_BEGIN_TYPE_CLASS(TreeViewItem);
        RTTI_METADATA(ElementClassNameMetadata).name("TreeViewItem");
    RTTI_END_TYPE();

    //---

    RTTI_BEGIN_TYPE_CLASS(TreeView);
        RTTI_METADATA(ElementClassNameMetadata).name("TreeView");
    RTTI_END_TYPE();

    TreeView::TreeView()
    {
        layoutMode(LayoutMode::Vertical);
        verticalScrollMode(ScrollMode::Auto);
    }

    bool TreeView::isExpanded(const ModelIndex& index) const
    {
        ViewItem* item = nullptr;
        if (findViewElement(index, item))
            return item->m_expanded;

        return true;
    }

    void TreeView::collectExpandedItems(base::Array<ModelIndex>& outIndices) const
    {
        for (auto* item : m_mainRows.m_orderedChildren)
            if (item->m_expanded)
                collectExpandedItems(item, outIndices);
    }

    void TreeView::collectExpandedItems(const ViewItem* item, base::Array<ModelIndex>& outIndices) const
    {
        outIndices.pushBack(item->m_index);

        for (const auto* child : item->m_children.m_orderedChildren)
            if (child->m_expanded)
                collectExpandedItems(child, outIndices);
    }

    void TreeView::ensureVisible(const ModelIndex& index)
    {
        expandItem(index.parent());
        TBaseClass::ensureVisible(index);
    }

    void TreeView::expandAll(bool recrusive)
    {
        if (m_model)
        {
            auto count = m_model->rowCount(ModelIndex());
            for (uint32_t i = 0; i < count; ++i)
            {
                if (auto childIndex = m_model->index(i, 0, ModelIndex()))
                {
                    ViewItem* item = nullptr;
                    if (findViewElement(childIndex, item))
                    {
                        changeExpandState(item, true, recrusive);
                    }
                }
            }
        }
    }

    void TreeView::expandItem(const ModelIndex& index)
    {
        if (auto parent = index.parent())
            expandItem(parent);
        
        ViewItem* item = nullptr;
        if (findViewElement(index, item))
            changeExpandState(item, true);
    }

    void TreeView::collapseItem(const ModelIndex& index)
    {
        ViewItem* item = nullptr;
        if (findViewElement(index, item))
            changeExpandState(item, false);
    }

    void TreeView::changeExpandState(ViewItem* item, bool state, bool recursive)
    {
        if (nullptr == item)
            return;

        if (item->m_expanded != state)
        {
            item->m_expanded = state;

            auto hasChildren = m_model->hasChildren(item->m_index);
            if (hasChildren && state)
            {
                auto childrenCount = m_model->rowCount(item->m_index);
                for (uint32_t i = 0; i < childrenCount; ++i)
                {
                    auto childIndex = m_model->index(i, 0, item->m_index);
                    DEBUG_CHECK(childIndex.row() == i);
                    attachViewElement(item, childIndex);
                }
            }
            else if (!state)
            {
                auto children = std::move(item->m_children.m_orderedChildren);
                item->m_children.m_orderedChildren.reset();
                item->m_children.m_displayOrder.reset();

                for (auto* child : children)
                    destroyViewElement(child);
            }

            updateExpandStyle(item);

            rebuildDisplayList(); // TODO: optimize
        }

        if (state && recursive)
        {
            for (auto* child : item->m_children.m_orderedChildren)
                changeExpandState(child, state, recursive);
        }
    }

    void TreeView::updateExpandStyle(ViewItem* item)
    {
        if (!m_model->hasChildren(item->m_index))
        {
            item->m_content->addStyleClass("nochildren"_id);
        }
        else if (item->m_expanded)
        {
            item->m_content->removeStyleClass("nochildren"_id);
            item->m_content->addStyleClass("expanded"_id);
        }
        else
        {
            item->m_content->removeStyleClass("nochildren"_id);
            item->m_content->removeStyleClass("expanded"_id);
        }
    }

    void TreeView::visualizeViewElement(ViewItem* item)
    {
        auto oldContent = item->m_innerContent;

        m_model->visualize(item->m_index, m_columnCount, item->m_innerContent);

        if (!item->m_content)
        {
            auto itemContainer = base::CreateSharedPtr<TreeViewItem>(item->m_depth);
            itemContainer->m_expandButton->bind(EVENT_CLICKED, this) = [item, this]()
            {                
                changeExpandState(item, !item->m_expanded);
            };

            itemContainer->customHorizontalAligment(ElementHorizontalLayout::Expand);
            itemContainer->customVerticalAligment(ElementVerticalLayout::Middle);
            //itemContainer->customProportion(1.0f);

            item->m_content = itemContainer;
            item->m_content->hitTest(true);

            updateExpandStyle(item);

            attachChild(item->m_content);
        }

        if (oldContent != item->m_innerContent && item->m_content)
        {
            if (oldContent)
                item->m_content->detachChild(oldContent);

            if (item->m_innerContent)
            {
                item->m_innerContent->customHorizontalAligment(ElementHorizontalLayout::Expand);
                item->m_innerContent->customVerticalAligment(ElementVerticalLayout::Middle);
                item->m_content->attachChild(item->m_innerContent);
            }
        }
    }

    void TreeView::unvisualizeViewElement(ViewItem* item)
    {
        if (item->m_content)
        {
            detachChild(item->m_content);
            item->m_content.reset();
        }

        item->m_innerContent.reset();
    }

    void TreeView::destroyViewElement(ViewItem* item)
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

    void TreeView::updateItem(ViewItem* item)
    {
        if (item && item->m_innerContent)
            m_model->visualize(item->m_index, m_columnCount, item->m_innerContent);
    }

    void TreeView::modelReset()
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

    void TreeView::modelRowsAboutToBeAdded(const ModelIndex& parent, int first, int count)
    {
        TBaseClass::modelRowsAboutToBeAdded(parent, first, count);
    }

    void TreeView::modelRowsAdded(const ModelIndex& parent, int first, int count)
    {
        TBaseClass::modelRowsAdded(parent, first, count);

        ViewItem* parentItem = nullptr;
        if (resolveItemFromModelIndex(parent, parentItem) && count)
        {
            if (!parentItem || parentItem->m_expanded)
            {
                ModelIndexReindexerInsert reindexer(parent, first, count);

                auto& items = parentItem ? parentItem->m_children : m_mainRows;
                auto& children = items.m_orderedChildren;
                auto hadChildren = !children.empty();
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
                    item->m_depth = parentItem ? parentItem->m_depth + 1 : 0;
                    children[i + first] = item;

                    visualizeViewElement(item);
                }

                buildSortedList(items);
                rebuildDisplayList();
            }

            if (parentItem)
            {
                updateItemWithMode(parentItem, ItemUpdateModeBit::Item);
                updateExpandStyle(parentItem);
            }
        }
    }

    void TreeView::modelRowsAboutToBeRemoved(const ModelIndex& parent, int first, int count)
    {
        TBaseClass::modelRowsAboutToBeRemoved(parent, first, count);
    }

    void TreeView::modelRowsRemoved(const ModelIndex& parent, int first, int count)
    {
        ViewItem* parentItem = nullptr;
        if (resolveItemFromModelIndex(parent, parentItem) && count)
        {
            ModelIndexReindexerRemove reindexer(parent, first, count);

            if (!parentItem || parentItem->m_expanded)
            {
                auto& children = parentItem ? parentItem->m_children : m_mainRows;
                ASSERT(first + count <= (int)children.m_orderedChildren.size());

                base::InplaceArray<ViewItem*, 10> itemsToRemove;

                for (int i = 0; i < count; ++i)
                {
                    auto* item = children.m_orderedChildren[first + i];
                    children.m_displayOrder.remove(item);
                    itemsToRemove.pushBack(item);
                }

                children.m_orderedChildren.erase(first, count);

                for (auto* item : itemsToRemove)
                    destroyViewElement(item);

                for (int i = first; i <= children.m_orderedChildren.lastValidIndex(); ++i)
                {
                    auto* item = children.m_orderedChildren[i];
                    reindexer.reindex(item->m_index, item->m_index);
                }

                buildSortedList(children.m_orderedChildren, children.m_displayOrder);
            }

            if (parentItem)
            {
                updateItemWithMode(parentItem, ItemUpdateModeBit::Item);
                updateExpandStyle(parentItem);
            }
        }

        TBaseClass::modelRowsRemoved(parent, first, count);
    }

    //---

    InputActionPtr TreeView::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.leftDoubleClicked())
        {
            auto index = indexAtPoint(evt.absolutePosition().toVector());
            if (index && !call(EVENT_ITEM_ACTIVATED, index))
            {
                select(index, ItemSelectionModeBit::Default);
                
                ViewItem* item = nullptr;
                if (resolveItemFromModelIndex(m_current, item))
                {
                    changeExpandState(item, !item->m_expanded);
                }
            }

            return InputActionPtr();
        }

        return TBaseClass::handleMouseClick(area, evt);
    }

    bool TreeView::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (evt.pressedOrRepeated())
        {
            ViewItem* item = nullptr;
            if (resolveItemFromModelIndex(m_current, item))
            {
                switch (evt.keyCode())
                {
                    case base::input::KeyCode::KEY_RETURN:
                    {
                        changeExpandState(item, !item->m_expanded);
                        return true;
                    }

                    case base::input::KeyCode::KEY_NUMPAD_ADD:
                    {
                        changeExpandState(item, true);
                        return true;
                    }

                    case base::input::KeyCode::KEY_NUMPAD_SUBTRACT:
                    {
                        changeExpandState(item, false);
                        return true;
                    }

                    case base::input::KeyCode::KEY_NUMPAD_MULTIPLY:
                    {
                        changeExpandState(item, true, true);
                        return true;
                    }
                }
            }
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    //--

} // ui
