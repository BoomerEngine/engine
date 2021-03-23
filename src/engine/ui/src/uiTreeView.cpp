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

#include "core/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

class TreeViewItem : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(TreeViewItem, IElement);

public:
    TreeViewItem(uint8_t depth = 0)
        : m_depth(depth)
    {
        layoutMode(LayoutMode::Horizontal);

        // offset by depth in the tree TODO: styling!!
        auto margins = Offsets(20.0f * depth, 0.0f, 0.0f, 0.0f);

        m_expandButton = createNamedChild<Button>("Button"_id, ButtonModeBit::EventOnClick);
        m_expandButton->createNamedChild<TextLabel>("ExpandIcon"_id);
        m_expandButton->customMargins(margins);
    }

    ButtonPtr m_expandButton;
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
        return item->m_expanded;// && item->m_expandable;

    return false;
}

void TreeView::collectExpandedItems(Array<ModelIndex>& outIndices) const
{
    for (auto* item : m_mainRows.m_orderedChildren)
        if (item->m_expanded)
            collectExpandedItems(item, outIndices);
}

void TreeView::collectExpandedItems(const ViewItem* item, Array<ModelIndex>& outIndices) const
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
    for (auto* rootItem : m_mainRows.m_orderedChildren)
        changeExpandState(rootItem, true, recrusive);
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
            InplaceArray<ModelIndex, 128> children;
            m_model->children(item->m_index, children);

            for (const auto& childIndex : children)
            {
                auto* childItem = m_pool.alloc();
                childItem->m_index = childIndex;
                childItem->m_depth = item->m_depth + 1;
                item->m_children.m_orderedChildren.pushBack(childItem);

                visualizeViewElement(childItem);
            }

            buildSortedList(item->m_children);
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
        auto itemContainer = RefNew<TreeViewItem>(item->m_depth);
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

    DEBUG_CHECK(m_mainRows.m_orderedChildren.empty());
    DEBUG_CHECK(m_mainRows.m_displayOrder.empty());

    if (nullptr != m_model)
    {
        Array<ModelIndex> initialChildren;
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

void TreeView::modelItemsAdded(const ModelIndex& parent, const Array<ModelIndex>& items)
{
    TBaseClass::modelItemsAdded(parent, items);

    ViewItem* parentItem = nullptr;
    if (resolveItemFromModelIndex(parent, parentItem))
    {
        if (!parentItem || parentItem->m_expanded)
        {
            auto& children = parentItem ? parentItem->m_children : m_mainRows;
            auto hadChildren = !children.m_orderedChildren.empty();

            for (const auto& itemIndex : items)
            {
                auto item = m_pool.alloc();

                item->m_index = itemIndex;
                item->m_parent = parentItem;
                item->m_depth = parentItem ? parentItem->m_depth + 1 : 0;
                children.m_orderedChildren.pushBack(item);

                visualizeViewElement(item);
            }

            buildSortedList(children);
            rebuildDisplayList();
        }

        if (parentItem)
        {
            updateItemWithMode(parentItem, ItemUpdateModeBit::Item);
            updateExpandStyle(parentItem);
        }
    }

}

void TreeView::modelItemsRemoved(const ModelIndex& parent, const Array<ModelIndex>& items)
{
    TBaseClass::modelItemsRemoved(parent, items);

    ViewItem* parentItem = nullptr;
    if (resolveItemFromModelIndex(parent, parentItem))
    {
        if (!parentItem || parentItem->m_expanded)
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

        if (parentItem)
        {
            updateItemWithMode(parentItem, ItemUpdateModeBit::Item);
            updateExpandStyle(parentItem);
        }
    }
}

//---

InputActionPtr TreeView::handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt)
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

bool TreeView::handleKeyEvent(const InputKeyEvent& evt)
{
    if (evt.pressedOrRepeated())
    {
        ViewItem* item = nullptr;
        if (resolveItemFromModelIndex(m_current, item))
        {
            switch (evt.keyCode())
            {
                case InputKey::KEY_RETURN:
                {
                    if (!call(EVENT_ITEM_ACTIVATED, item->m_index))
                        changeExpandState(item, !item->m_expanded);
                    return true;
                }

                case InputKey::KEY_NUMPAD_ADD:
                {
                    changeExpandState(item, true);
                    return true;
                }

                case InputKey::KEY_NUMPAD_SUBTRACT:
                {
                    changeExpandState(item, false);
                    return true;
                }

                case InputKey::KEY_NUMPAD_MULTIPLY:
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

END_BOOMER_NAMESPACE_EX(ui)
