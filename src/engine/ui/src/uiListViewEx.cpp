/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#include "build.h"
#include "uiListViewEx.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

RTTI_BEGIN_TYPE_CLASS(IListItem);
RTTI_METADATA(ElementClassNameMetadata).name("ListViewItem");
RTTI_END_TYPE();

IListItem::IListItem()
{
    layoutMode(LayoutMode::Horizontal);
    hitTest(true);
}

IListItem::~IListItem()
{}

void IListItem::removeSelfFromList()
{
    if (auto* listPtr = view())
        listPtr->removeItem(this);
}

//---


RTTI_BEGIN_TYPE_CLASS(ListViewEx);
    RTTI_METADATA(ElementClassNameMetadata).name("ListView");
RTTI_END_TYPE();

ListViewEx::ListViewEx()
{
    layoutMode(LayoutMode::Vertical);
    verticalScrollMode(ScrollMode::Auto);
}

void ListViewEx::internalAttachItem(ViewItem* item)
{
    item->item->removeStyleClass("selected"_id);
    item->item->removeStyleClass("current"_id);
    attachChild(item->item);
}

void ListViewEx::internalDetachItem(ViewItem* item)
{
    detachChild(item->item);
}

void ListViewEx::rebuildDisplayList()
{
    m_displayList.reset();

    for (auto* item : m_viewItems)
        if (item->item->handleItemFilter(m_filter))
            m_displayList.pushBack(item);

    for (auto* item : m_staticElements)
        m_displayList.pushBack(item);

    m_displayListInvalid = false;
}

void ListViewEx::clear()
{
    m_displayList.clear();
    m_selection.clear();
    m_selectionItems.clear();

    auto oldViewItems = std::move(m_viewItems);
    for (auto* viewItem : oldViewItems)
        internalRemoveItem(viewItem);
}

void ListViewEx::addItem(IListItem* item)
{
    DEBUG_CHECK_RETURN_EX(item, "Invalid item");
    DEBUG_CHECK_RETURN_EX(item->m_view.unsafe() == nullptr, "Item already in a list");

    internalAddItem(item);
}

void ListViewEx::removeItem(IListItem* item)
{
    DEBUG_CHECK_RETURN_EX(item, "Invalid item");
    DEBUG_CHECK_RETURN_EX(item->m_view.unsafe() == this, "Item not in this list");
    DEBUG_CHECK_RETURN_EX(item->m_viewItem != nullptr, "Item not in this list");

    auto* viewItem = (ViewItem*)item->m_viewItem;
    internalRemoveItem(viewItem);
}

//--

END_BOOMER_NAMESPACE_EX(ui)
