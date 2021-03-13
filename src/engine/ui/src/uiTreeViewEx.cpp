/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#include "build.h"
#include "uiTreeViewEx.h"
#include "uiTextLabel.h"
#include "uiInputAction.h"
#include "uiButton.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

#pragma optimize("", off)

//---

RTTI_BEGIN_TYPE_CLASS(ITreeItem);
    RTTI_METADATA(ElementClassNameMetadata).name("TreeViewItemEx");
RTTI_END_TYPE();

ITreeItem::ITreeItem()
{
    hitTest(true);
    layoutMode(LayoutMode::Horizontal);

    m_expandButton = createInternalChildWithType<Button>("TreeViewExpandButton"_id, ButtonModeBit::EventOnClick);
    m_expandButton->createChild<TextLabel>();

    m_expandButton->bind(EVENT_CLICKED) = [this]() { toggleExpand(); };
}

ITreeItem::~ITreeItem()
{

}

void ITreeItem::updateButtonState()
{
    if (m_expanded)
    {
        m_expandButton->removeStyleClass("nochildren"_id);
        m_expandButton->addStyleClass("expanded"_id);
    }
    else if (!handleItemCanExpand())
    {
        m_expandButton->addStyleClass("nochildren"_id);
    }
    else
    {
        m_expandButton->removeStyleClass("nochildren"_id);
        m_expandButton->removeStyleClass("expanded"_id);
    }
}

void ITreeItem::updateDisplayDepth(int depth)
{
    if (m_displayDepth != depth)
    {
        m_displayDepth = depth;

        auto margins = Offsets(20.0f * depth, 0.0f, 0.0f, 0.0f);
        m_expandButton->customMargins(margins);

        for (const auto& child : m_children)
            child->updateDisplayDepth(depth + 1);
    }
}

void ITreeItem::toggleExpand()
{
    if (!expanded())
        expand();
    else
        collapse();
}

void ITreeItem::expand()
{
    if (!m_expanded)
    {
        m_expanded = true;
        handleItemExpand();

        if (!m_children.empty() && m_expanded)
            invalidateDisplayList();
        else
            m_expanded = false;

        updateButtonState();
    }
}

void ITreeItem::collapse()
{
    if (m_expanded)
    {
        m_expanded = false;
        handleItemCollapse();

        if (!m_expanded)
            invalidateDisplayList();

        updateButtonState();
    }
}

void ITreeItem::expandAll()
{
    expand();
    for (const auto& child : m_children)
        child->expandAll();
}

void ITreeItem::collapseAll()
{
    for (const auto& child : m_children)
        child->collapseAll();
    collapse();
}

//--

void ITreeItem::addChild(ITreeItem* child)
{
    DEBUG_CHECK_RETURN_EX(child != nullptr, "Invalid child");
    DEBUG_CHECK_RETURN_EX(child->view() == nullptr, "Child already in view");
    DEBUG_CHECK_RETURN_EX(!m_children.contains(child), "Child already in parent");

    child->updateButtonState();
    child->updateDisplayDepth(m_displayDepth + 1);
    child->m_parent = this;

    m_children.emplaceBack(AddRef(child));

    internalCreateChildViewItem(child);
    invalidateDisplayList();
}

void ITreeItem::removeChild(ITreeItem* child)
{
    DEBUG_CHECK_RETURN_EX(child != nullptr, "Invalid child");
    DEBUG_CHECK_RETURN_EX(child->view() == view(), "Child not in this tree");

    auto index = m_children.find(child);
    DEBUG_CHECK_RETURN_EX(index != INDEX_NONE, "Child not in parent");

    child->m_parent = nullptr;
    child->internalDestroyViewItem();
    m_children.erase(index);
    invalidateDisplayList();
}

void ITreeItem::removeAllChildren()
{
    if (!m_children.empty())
    {
        auto old = std::move(m_children);
        for (const auto& child : old)
            child->internalDestroyViewItem();

        invalidateDisplayList();
        updateButtonState();
    }
}

//--

bool ITreeItem::handleItemCanExpand() const
{
    return !m_children.empty();
}

void ITreeItem::handleItemExpand()
{
    // great place to create ad-hoc elements
}

void ITreeItem::handleItemCollapse()
{
    // great place to remove ad-hoc elements
}

bool ITreeItem::handleKeyEvent(const input::KeyEvent& evt)
{
    if (evt.pressedOrRepeated())
    {
        switch (evt.keyCode())
        {
            case input::KeyCode::KEY_NUMPAD_ADD:
            {
                expand();
                return true;
            }

            case input::KeyCode::KEY_NUMPAD_SUBTRACT:
            {
                collapse();
                return true;
            }

            case input::KeyCode::KEY_NUMPAD_MULTIPLY:
            {
                if (evt.keyMask().isShiftDown())
                    collapseAll();
                else
                    expandAll();
                return true;
            }
        }
    }

    return TBaseClass::handleKeyEvent(evt);
}

//---

RTTI_BEGIN_TYPE_CLASS(TreeViewEx);
    RTTI_METADATA(ElementClassNameMetadata).name("TreeView");
RTTI_END_TYPE();

TreeViewEx::TreeViewEx()
{
    layoutMode(LayoutMode::Vertical);
    verticalScrollMode(ScrollMode::Auto);
}

void TreeViewEx::clear()
{
    m_displayList.clear();
    m_displayListInvalid = false;

    auto roots = std::move(m_roots);

    for (const auto& root : roots)
        internalRemoveItem(root);
}

void TreeViewEx::addRoot(ITreeItem* item)
{
    DEBUG_CHECK_RETURN_EX(item, "Invalid item");
    DEBUG_CHECK_RETURN_EX(item->view() == nullptr, "Item already in a list");

    item->updateDisplayDepth(0);
    item->updateButtonState();

    internalAddItem(item);
    m_roots.pushBack((ViewItem*)item->viewItem());

    invalidateDisplayList();
}

void TreeViewEx::removeRoot(ITreeItem* item)
{
    DEBUG_CHECK_RETURN_EX(item, "Invalid item");
    DEBUG_CHECK_RETURN_EX(item->view() == this, "Item not in a list");
    DEBUG_CHECK_RETURN_EX(item->viewItem() != nullptr, "Item not in this list");

    auto* viewItem = (ViewItem*)item->viewItem();
    DEBUG_CHECK_RETURN_EX(m_roots.contains(viewItem), "Item not a root");

    internalRemoveItem(viewItem);
    m_roots.remove(viewItem);

    invalidateDisplayList();
}

void TreeViewEx::remove(ITreeItem* item)
{
    DEBUG_CHECK_RETURN_EX(item, "Invalid item");
    DEBUG_CHECK_RETURN_EX(item->view() == this, "Item not in a list");
    DEBUG_CHECK_RETURN_EX(item->viewItem() != nullptr, "Item not in this list");

    auto* viewItem = (ViewItem*)item->viewItem();
    if (m_roots.contains(viewItem))
    {
        removeRoot(item);
    }
    else
    {
        for (auto* root : m_roots)
            if (auto rootItem = rtti_cast<ITreeItem>(root->item))
                if (removeParticularViewItemRecursive(rootItem, nullptr, item))
                    break;
    }
}

bool TreeViewEx::removeParticularViewItemRecursive(ITreeItem* cur, ITreeItem* parent, ITreeItem* itemToRemove)
{
    if (cur == itemToRemove)
    {
        parent->removeChild(itemToRemove);
        return true;
    }

    for (const auto& child : cur->children())
        if (removeParticularViewItemRecursive(child, cur, itemToRemove))
            return true;

    return false;
}

//---

void TreeViewEx::internalAttachItem(ViewItem* item)
{
    item->item->removeStyleClass("selected"_id);
    item->item->removeStyleClass("current"_id);
    attachChild(item->item);

    if (auto treeItem = rtti_cast<ITreeItem>(item->item))
    {
        for (auto& child : treeItem->m_children)
            treeItem->internalCreateChildViewItem(child);
    }
}

void TreeViewEx::internalDetachItem(ViewItem* item)
{
    if (auto treeItem = rtti_cast<ITreeItem>(item->item))
    {
        for (auto& child : treeItem->m_children)
            child->internalDestroyViewItem();

        detachChild(treeItem);
    }
}

void TreeViewEx::collectViewItems(ViewItem* item, int depth, Array<ViewItem*>& outItems)
{
    outItems.pushBack(item);

    if (auto treeItem = rtti_cast<ITreeItem>(item->item))
    {
        if (treeItem->expanded())
        {
            if (m_sortingColumnIndex != INDEX_NONE)
            {
                InplaceArray<ViewItem*, 16> tempList;

                for (auto& child : treeItem->m_children)
                    if (auto* viewItem = child->viewItem())
                        tempList.pushBack((ViewItem*)viewItem);

                sortViewItems(tempList);

                for (auto* viewItem : tempList)
                    collectViewItems(viewItem, depth+1, outItems);
            }
            else
            {
                for (auto& child : treeItem->m_children)
                    if (auto* viewItem = child->viewItem())
                        collectViewItems((ViewItem*)viewItem, depth + 1, outItems);
            }
        }
    }
}

void TreeViewEx::rebuildDisplayList()
{
    m_displayList.reset();

    for (auto* item : m_roots)
        collectViewItems(item, 0, m_displayList);

    for (auto* item : m_staticElements)
        m_displayList.pushBack(item);

    m_displayListInvalid = false;
}

bool TreeViewEx::processActivation()
{
    if (TBaseClass::processActivation())
        return true;

    if (m_current)
    {
        if (auto treeItem = rtti_cast<ITreeItem>(m_current->item))
        {
            treeItem->toggleExpand();
            return true;
        }
    }

    return false;
}

//---

#if 0
InputActionPtr TreeView::handleMouseClick(const ElementArea& area, const input::MouseClickEvent& evt)
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

bool TreeView::handleKeyEvent(const input::KeyEvent& evt)
{
    if (evt.pressedOrRepeated())
    {
        ViewItem* item = nullptr;
        if (resolveItemFromModelIndex(m_current, item))
        {
            switch (evt.keyCode())
            {
                case input::KeyCode::KEY_RETURN:
                {
                    if (!call(EVENT_ITEM_ACTIVATED, item->m_index))
                        changeExpandState(item, !item->m_expanded);
                    return true;
                }

                case input::KeyCode::KEY_NUMPAD_ADD:
                {
                    changeExpandState(item, true);
                    return true;
                }

                case input::KeyCode::KEY_NUMPAD_SUBTRACT:
                {
                    changeExpandState(item, false);
                    return true;
                }

                case input::KeyCode::KEY_NUMPAD_MULTIPLY:
                {
                    changeExpandState(item, true, true);
                    return true;
                }
            }
        }
    }

    return TBaseClass::handleKeyEvent(evt);
}
#endif

//--

END_BOOMER_NAMESPACE_EX(ui)
