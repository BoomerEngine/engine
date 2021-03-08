/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#include "build.h"
#include "uiCollectionView.h"

#include "core/input/include/inputStructures.h"
#include "uiWindowPopup.h"

#pragma optimize("", off)

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

RTTI_BEGIN_TYPE_CLASS(ICollectionItem);
RTTI_END_TYPE();

static std::atomic<uint32_t> GUniqueListItemIndex(1);

ICollectionItem::ICollectionItem()
{
    m_uniqueIndex = GUniqueListItemIndex++;
}

bool ICollectionItem::handleItemFilter(const SearchPattern& filter) const
{
    return true;
}

bool ICollectionItem::handleItemSort(const ICollectionItem* other, int colIndex) const
{
    auto otherIndex = other ? other->uniqueIndex() : 0;
    return uniqueIndex() < otherIndex;
}

bool ICollectionItem::handleItemContextMenu(const CollectionItems& items, PopupPtr& outPopup) const
{
    return false;
}

bool ICollectionItem::handleItemSelected()
{
    return true;
}

void ICollectionItem::handleItemUnselected()
{

}

void ICollectionItem::handleItemFocused()
{

}

void ICollectionItem::handleItemUnfocused()
{

}

bool ICollectionItem::handleItemActivate()
{
    return false;
}

void ICollectionItem::invalidateDisplayList()
{
    if (auto* view = m_view.unsafe())
        view->invalidateDisplayList();
}

void ICollectionItem::internalCreateChildViewItem(ICollectionItem* child)
{
    if (auto* view = m_view.unsafe())
        view->internalAddItem(child);
}

void ICollectionItem::internalDestroyViewItem()
{
    if (auto* view = m_view.unsafe())
        if (m_viewItem)
            view->internalRemoveItem((ICollectionView::ViewItem*)m_viewItem);
}

//---

CollectionItems::CollectionItems() {};
CollectionItems::CollectionItems(const CollectionItems& other) = default;
CollectionItems::CollectionItems(CollectionItems && other) = default;
CollectionItems& CollectionItems::operator=(const CollectionItems & other) = default;
CollectionItems& CollectionItems::operator=(CollectionItems && other) = default;

ICollectionItem* CollectionItems::head() const
{
    if (!m_items.keys().empty())
        return m_items.keys().front();
    else
        return nullptr;
}

ICollectionItem* CollectionItems::tail() const
{
    if (!m_items.keys().empty())
        return m_items.keys().back();
    else
        return nullptr;
}

bool CollectionItems::contains(const ICollectionItem* item) const
{
    return m_items.contains(item);
}

void CollectionItems::clear()
{
    m_items.clear();
}

bool CollectionItems::add(ICollectionItem* item)
{
    DEBUG_CHECK_RETURN_EX_V(item, "Cannot insert invalid item into the item collection", false);
    return m_items.insert(AddRef(item));
}

bool CollectionItems::remove(ICollectionItem* item)
{
    return m_items.remove(item);
}

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICollectionView);
RTTI_END_TYPE();

ICollectionView::ICollectionView()
{
    layoutMode(LayoutMode::Vertical);
    verticalScrollMode(ScrollMode::Auto);
}

void ICollectionView::internalAddItem(ICollectionItem* item)
{
    DEBUG_CHECK_RETURN_EX(item, "Invalid item");
    DEBUG_CHECK_RETURN_EX(item->m_view.unsafe() == nullptr, "Item already in a list");

    auto* viewItem = m_viewItemsPool.create();
    viewItem->item = AddRef(item);
    viewItem->unadjustedCachedArea = ElementArea();

    item->m_view = this;
    item->m_viewItem = viewItem;
    m_viewItems.insert(viewItem);

    invalidateDisplayList();

    internalAttachItem(viewItem);
}

/*void ICollectionView::internalAttachItem(ViewItem* item)
{
    item->displayItem = item->item;
    attachChild(item->displayItem);
}*/

void ICollectionView::internalRemoveItem(ViewItem* viewItem)
{
    bool currentChanged = false;
    if (m_current == viewItem)
    {
        auto old = m_current;
        m_current = nullptr;
        currentChanged = true;

        old->unfocus();
    }

    auto displayIndex = m_displayList.find(viewItem);
    if (displayIndex != INDEX_NONE)
        m_displayList.erase(displayIndex);

    m_selection.remove(viewItem);
    m_selectionItems.remove(viewItem->item);

    viewItem->item->m_view = nullptr;
    viewItem->item->m_viewItem = nullptr;
    internalDetachItem(viewItem);

    m_viewItems.remove(viewItem);
    m_viewItemsPool.free(viewItem);

    if (displayIndex != INDEX_NONE && currentChanged && !m_current)
    {
        auto index = std::min<int>(m_displayList.lastValidIndex(), displayIndex);
        if (index >= 0 && index <= m_displayList.lastValidIndex())
        {
            m_current = m_displayList[index];
            m_current->focus();
        }
    }
}

void ICollectionView::select(ICollectionItem* item, ItemSelectionMode mode /*= ItemSelectionModeBit::Default*/, bool postEvent /*= true*/)
{
    ViewItem* viewItem = nullptr;

    if (item)
    {
        DEBUG_CHECK_RETURN_EX(item->m_view.unsafe() == this, "Item not in this view");
        DEBUG_CHECK_RETURN_EX(item->m_viewItem != nullptr, "Item not in this view");
        viewItem = (ViewItem*)item->m_viewItem;
    }

    select(viewItem, mode, postEvent);
}

void ICollectionView::select(const CollectionItems& items, ItemSelectionMode mode /*= ItemSelectionModeBit::Default*/, bool postEvent /*= true*/)
{
    InplaceArray<ViewItem*, 128> viewItems;

    for (const auto& item : items.items())
    {
        DEBUG_CHECK_RETURN_EX(item, "Invalid item");
        DEBUG_CHECK_RETURN_EX(item->m_view.unsafe() == this, "Item not in this view");
        DEBUG_CHECK_RETURN_EX(item->m_viewItem != nullptr, "Item not in this view");

        auto* viewItem = (ViewItem*)item->m_viewItem;
        viewItems.pushBack(viewItem);
    }

    select(viewItems, mode, postEvent);
}

void ICollectionView::ensureVisible(ViewItem* viewItem)
{
    DEBUG_CHECK_RETURN_EX(viewItem, "Invalid item");

    if (m_displayList.contains(viewItem))
        scrollToMakeElementVisible(viewItem->item);
}

void ICollectionView::ensureVisible(ICollectionItem* item)
{
    DEBUG_CHECK_RETURN_EX(item, "Invalid item");
    DEBUG_CHECK_RETURN_EX(item->m_view == this, "Item not in view");
    DEBUG_CHECK_RETURN_EX(item->m_viewItem != nullptr, "Item not attached");

    auto* viewItem = (ViewItem*)item->m_viewItem;
    ensureVisible(viewItem);
}

void ICollectionView::filter(const SearchPattern& filter)
{
    m_filter = filter;
    invalidateDisplayList();
}

void ICollectionView::sort(int colIndex /*= INDEX_NONE*/, bool asc /*= true*/)
{
    if (m_sortingAsc != asc || m_sortingColumnIndex != colIndex)
    {
        m_sortingAsc = asc;
        m_sortingColumnIndex = colIndex;
        invalidateDisplayList();
    }
}

void ICollectionView::sortViewItems(Array<ViewItem*>& items) const
{
    if (m_sortingColumnIndex != INDEX_NONE)
    {
        if (m_sortingAsc)
        {
            std::sort(items.begin(), items.end(), [this](const auto* a, const auto* b)
                {
                    if (m_sortingAsc)
                        std::swap(a, b);

                    return a->item->handleItemSort(b->item, m_sortingColumnIndex);
                });
        }
    }
}

void ICollectionView::attachStaticElement(IElement* element)
{
    DEBUG_CHECK_RETURN_EX(element, "Invalid element");
    DEBUG_CHECK_RETURN_EX(!element->parentElement(), "Element already has parent");
    attachChild(element);

    auto* item = m_viewItemsPool.create();
    item->staticElement = AddRef(element);
    
    m_staticElements.pushBack(item);
}

void ICollectionView::detachStaticElement(IElement* element)
{
    DEBUG_CHECK_RETURN_EX(element, "Invalid element");
    DEBUG_CHECK_RETURN_EX(element->parentElement() == this, "Element not attached here");

    for (auto* viewItem : m_staticElements)
    {
        if (viewItem->staticElement == element)
        {
            detachChild(viewItem->staticElement);
            viewItem->staticElement.reset();

            m_displayList.remove(viewItem);
            m_staticElements.remove(viewItem);

            m_viewItemsPool.free(viewItem);
            break;
        }
    }
}

void ICollectionView::bindInputPropagationElement(IElement* element)
{
    m_inputPropagationElement = element;
}

void ICollectionView::itemsAtArea(const ElementArea& area, Array<ViewItem*>& outItems) const
{
    // find first element in range
    auto first = std::lower_bound(m_displayList.begin(), m_displayList.end(), area.absolutePosition().y, [](const ViewItem* a, float pos)
        {
            return a->item->cachedDrawArea().absolutePosition().y < pos;
        });

    // find last in range
    auto last = std::upper_bound(m_displayList.begin(), m_displayList.end(), area.absolutePosition().y, [](float pos, const ViewItem* a)
        {
            return pos < a->item->cachedDrawArea().absolutePosition().y;
        });

    // filter the ones really intersecting
    for (auto it = first; it < last; ++it)
        if ((*it)->item->cachedDrawArea().touches(area))
            outItems.pushBack(*it);
}

ICollectionView::ViewItem* ICollectionView::itemAtPoint(const Position& pos) const
{
    auto scrolledPos = pos + Position(horizontalScrollOffset(), verticalScrollOffset());

    for (auto* item : m_displayList)
    {
        const auto& area = item->unadjustedCachedArea;
        if (!area.empty() && area.contains(scrolledPos))
            return item;
    }

    return nullptr;
}

void ICollectionView::invalidateDisplayList()
{
    m_displayList.reset();
    m_displayListInvalid = true;
}

//--

void ICollectionView::validateDisplayList()
{
    if (m_displayListInvalid)
    {
        rebuildDisplayList();
        m_displayListInvalid = false;

        if (m_current)
        {
            auto index = m_displayList.find(m_current);
            if (index != INDEX_NONE)
            {
                ensureVisible(m_current);
            }
            else
            {
                m_current->unfocus();
                m_current = nullptr;
            }
        }
    }
}

bool ICollectionView::iterateDrawChildren(ElementDrawListToken& token) const
{
    if (token.start())
        const_cast<ICollectionView*>(this)->validateDisplayList();

    while (token.index < m_displayList.lastValidIndex())
    {
        token.index += 1;

        auto* viewItem = m_displayList[token.index];
        if (viewItem->item)
        {
            token.elem = viewItem->item;
            token.m_unadjustedArea = &viewItem->unadjustedCachedArea;
        }
        return true;
    }

    return false;
}

//--

int ICollectionView::findItemFromPrevRow(int displayIndex, float topY, float centerX) const
{
    const auto& itemArea = m_displayList[displayIndex]->unadjustedCachedArea;
    if (itemArea.empty())
        return displayIndex;

    // find start of next row
    bool hasPrevRow = false;
    auto testIndex = displayIndex;
    while (testIndex > 0)
    {
        const auto& testArea = m_displayList[testIndex]->unadjustedCachedArea;
        if (!testArea.empty() && testArea.bottom() <= topY)
        {
            hasPrevRow = true;
            break;
        }
        --testIndex;
    }

    // no previous row, use the first element
    if (!hasPrevRow)
        return 0;

    // find element closest to the reference area
    auto bestDistance = VERY_LARGE_FLOAT;
    auto bestIndex = testIndex;
    while (testIndex > 0)
    {
        const auto& testArea = m_displayList[testIndex]->unadjustedCachedArea;
        auto dist = std::fabs(testArea.center().x - centerX);
        if (!testArea.empty() && dist < bestDistance)
        {
            bestDistance = dist;
            bestIndex = testIndex;
        }
        // if we start going up it won't get better, use current best
        else if (dist > bestDistance)
            break;

        --testIndex;
    }

    return bestIndex;
}

int ICollectionView::findItemFromNextRow(int displayIndex, float bottomY, float centerX) const
{
    auto lastItem = m_displayList.lastValidIndex();
    const auto& itemArea = m_displayList[displayIndex]->unadjustedCachedArea;
    if (itemArea.empty())
        return displayIndex;

    // find start of next row
    bool hasRow = false;
    auto testIndex = displayIndex;
    while (testIndex <= lastItem)
    {
        const auto& testArea = m_displayList[testIndex]->unadjustedCachedArea;
        if (!testArea.empty() && testArea.top() >= bottomY)
        {
            hasRow = true;
            break;
        }
        ++testIndex;
    }

    // no previous row, use the last element
    if (!hasRow)
        return lastItem;

    // find element closest to the reference area
    auto bestDistance = VERY_LARGE_FLOAT;
    auto bestIndex = testIndex;
    while (testIndex <= lastItem)
    {
        const auto& testArea = m_displayList[testIndex]->unadjustedCachedArea;
        auto dist = std::fabs(testArea.center().x - centerX);
        if (!testArea.empty() && dist < bestDistance)
        {
            bestDistance = dist;
            bestIndex = testIndex;
        }
        // if we start going up it won't get better, use current best
        else if (dist > bestDistance)
            break;

        ++testIndex;
    }

    return bestIndex;
}

int ICollectionView::resolveIndexNavigation(int current, NavigationDirection mode) const
{
    // invalid item, go to first
    if (current == INDEX_NONE)
    {
        if (!m_displayList.empty())
        {
            if (mode == NavigationDirection::End)
                return m_displayList.lastValidIndex();
            else
                return 0;
        }

        return -1;
    }

    // item should be in range
    DEBUG_CHECK_RETURN_EX_V(current >= 0 && current <= m_displayList.lastValidIndex(), "Invalid current item", -1);

    // find the item
    auto* item = m_displayList[current];

    // common modes
    if (NavigationDirection::Home == mode)
        return 0;
    else if (NavigationDirection::End == mode)
        return m_displayList.lastValidIndex();

    // get area of the item
    const auto& displayArea = item->item->cachedDrawArea();

    // get the prev/next keys for current model
    if (layoutMode() == LayoutMode::Icons)
    {
        if (NavigationDirection::Up == mode)
            return findItemFromPrevRow(current, displayArea.top() + verticalScrollOffset(), displayArea.center().x + horizontalScrollOffset());
        else if (NavigationDirection::Down == mode)
            return findItemFromNextRow(current, displayArea.bottom() + verticalScrollOffset(), displayArea.center().x + horizontalScrollOffset());
        else if (NavigationDirection::Left == mode)
            return std::max<int>(current - 1, 0);
        else if (NavigationDirection::Right == mode)
            return std::min<int>(current + 1, m_displayList.lastValidIndex());
    }
    else
    {
        if (NavigationDirection::Up == mode)
            return std::max<int>(current - 1, 0);
        else if (NavigationDirection::Down == mode)
            return std::min<int>(current + 1, m_displayList.lastValidIndex());
        /*else if (NavigationDirection::Left == mode)
        {
            // go to parent item
            if (nullptr != item->m_parent)
                return item->m_parent->m_index;
            return ModelIndex();
        }
        else if (ItemNavigationDirection::Right == mode)
        {
            // go to first child item
            if (!item->m_children.m_orderedChildren.empty())
                return item->m_children.m_orderedChildren.front()->m_index;
            return ModelIndex();
        }*/
    }

    // page up/page.down
    if (NavigationDirection::PageUp == mode)
    {
        auto pageSize = cachedDrawArea().size().y;
        return findItemFromPrevRow(current, displayArea.top() - pageSize + verticalScrollOffset(), displayArea.center().x + horizontalScrollOffset());
    }
    else if (NavigationDirection::PageDown == mode)
    {
        auto pageSize = cachedDrawArea().size().y;
        return findItemFromNextRow(current, displayArea.bottom() + pageSize + verticalScrollOffset(), displayArea.center().x + horizontalScrollOffset());
    }

    // nothing changed, return the same item
    return current;
}

//--

void ICollectionView::handleFocusGained()
{

}

InputActionPtr ICollectionView::handleMouseClick(const ElementArea& area, const input::MouseClickEvent& evt)
{
    if (evt.leftDoubleClicked())
    {
        processActivation();
        return InputActionPtr();
    }

    return TBaseClass::handleMouseClick(area, evt);
}

bool ICollectionView::ViewItem::select()
{
    if (!item->m_selected)
    {
        if (!item->handleItemSelected())
            return false;
        item->m_selected = true;
    }

    item->addStyleClass("selected"_id);
    return true;
}

void ICollectionView::ViewItem::unselect()
{
    if (item->m_selected)
    {
        item->handleItemUnselected();
        item->m_selected = false;
    }

    item->removeStyleClass("selected"_id);
}

void ICollectionView::ViewItem::focus()
{
    item->addStyleClass("current"_id);
    item->handleItemFocused();
}

void ICollectionView::ViewItem::unfocus()
{
    item->removeStyleClass("current"_id);
    item->handleItemUnfocused();
}

void ICollectionView::focusElement(ViewItem* item)
{
    IElement* focusElement = this;

    if (item)
    {
        focusElement = item->item;
        if (auto customFocusElement = item->item->focusFindFirst())
            focusElement = customFocusElement;
    }

    if (focusElement)
        focusElement->focus();
}

void ICollectionView::select(const Array<ViewItem*>& items, ItemSelectionMode mode, bool postEvent)
{
    bool somethingChanged = false;

    // selection change
    if (mode.test(ItemSelectionModeBit::Clear))
    {
        // add objects that we want to directly select
        // O(N) in the number of items
        HashSet<ViewItem*> tempSelection;
        if (mode.test(ItemSelectionModeBit::Select))
        {
            for (const auto& item : items)
                tempSelection.insert(item);
        }

        // update visualization based on set difference
        // O(N log N) in the number of items
        HashSet<ViewItem*> newSelection;
        for (const auto& it : tempSelection.keys())
        {
            if (!m_selection.contains(it))
            {
                if (it->select())
                {
                    newSelection.insert(it);
                    somethingChanged = true;
                }
            }
            else
            {
                newSelection.insert(it);
            }
        }

        // remove visualization from deselected items - O(N log N) in the number of items
        for (const auto& it : m_selection.keys())
        {
            if (!tempSelection.contains(it))
            {
                it->unselect();
                somethingChanged = true;
            }
        }

        // set new selection
        m_selection = std::move(newSelection);
    }
    // select mode
    else if (mode.test(ItemSelectionModeBit::Select) && !mode.test(ItemSelectionModeBit::Deselect))
    {
        // items in the list, if not already selected will be selected
        for (const auto& item : items)
        {
            if (item && !m_selection.contains(item))
            {
                if (item->select())
                {
                    m_selection.insert(item);
                    somethingChanged = true;
                }
            }
        }
    }
    // deselect mode
    else if (mode.test(ItemSelectionModeBit::Deselect) && !mode.test(ItemSelectionModeBit::Select))
    {
        // items in the list, if not already selected will be selected
        for (const auto& item : items)
        {
            if (item && m_selection.removeOrdered(item))
            {
                item->unselect();
                somethingChanged = true;
            }
        }
    }
    // toggle mode
    else if (mode.test(ItemSelectionModeBit::Toggle))
    {
        // items in the list, if not already selected will be selected
        for (const auto& item : items)
        {
            if (m_selection.remove(item))
            {
                item->unselect();
                somethingChanged = true;
            }
            else if (!m_selection.contains(item))
            {
                if (item->select())
                {
                    m_selection.insert(item);
                    somethingChanged = true;
                }
            }
        }
    }

    // update current item
    if (mode.test(ItemSelectionModeBit::UpdateCurrent))
    {
        auto* newCurrent = items.empty() ? nullptr : items.back();
        if (newCurrent != m_current)
        {
            if (m_current)
                m_current->unfocus();

            m_current = newCurrent;

            if (m_current)
                m_current->focus();
        }
    }

    // rebuild the shadow list
    if (somethingChanged)
    {
        // TODO: optimize
        m_selectionItems.clear();
        for (const auto* item : m_selection.keys())
            m_selectionItems.add(item->item);
    }

    // focus inner widget if required
    if (mode.test(ItemSelectionModeBit::FocusCurrent) && m_current)
        focusElement(m_current);

    // notify interested parties
    if (somethingChanged && postEvent)
        call(EVENT_ITEM_SELECTION_CHANGED);
}

void ICollectionView::select(ViewItem* item, ItemSelectionMode mode, bool postEvent)
{
    InplaceArray<ViewItem*, 1> items;
    if (item)
        items.pushBack(item);
    select(items, mode, postEvent);
}

InputActionPtr ICollectionView::handleOverlayMouseClick(const ElementArea& area, const input::MouseClickEvent& evt)
{
    if (evt.leftClicked() || evt.rightClicked())
    {
        auto* clickedItem = itemAtPoint(evt.absolutePosition().toVector());

        if (evt.leftClicked() || !m_selection.contains(clickedItem))
        {
            if (evt.keyMask().isShiftDown())
            {
                if (!m_selection.empty())
                {
                    auto tailIndex = m_displayList.find(clickedItem);
                    auto headIndex = m_displayList.find(m_selection.keys().front());
                    if (tailIndex != INDEX_NONE && headIndex != INDEX_NONE)
                    {
                        Array<ViewItem*> newItems;
                        newItems.reserve(tailIndex - headIndex + 1);

                        if (headIndex < tailIndex)
                        {
                            for (auto i = headIndex; i <= tailIndex; ++i)
                                newItems.pushBack(m_displayList[i]);
                        }
                        else
                        {
                            for (auto i = headIndex; i >= tailIndex; --i)
                                newItems.pushBack(m_displayList[i]);
                        }

                        select(newItems, ItemSelectionModeBit::Default, true);
                    }
                }
            }
            else if (evt.keyMask().isCtrlDown())
            {
                if (clickedItem)
                    select(clickedItem, { ItemSelectionModeBit::Toggle, ItemSelectionModeBit::UpdateCurrent }, true);
            }
            else
            {
                select(clickedItem, ItemSelectionModeBit::Default, true);
            }
        }
    }

    return TBaseClass::handleOverlayMouseClick(area, evt);
}

bool ICollectionView::handleContextMenu(const ElementArea& area, const Position& absolutePosition, input::KeyMask controlKeys)
{
    if (auto item = itemAtPoint(absolutePosition))
    {
        PopupPtr popupToShow;
        item->item->handleItemContextMenu(m_selectionItems, popupToShow);

        if (popupToShow)
            popupToShow->show(this, PopupWindowSetup().relativeToCursor().bottomLeft().interactive().autoClose());

        return true;
    }

    return TBaseClass::handleContextMenu(area, absolutePosition, controlKeys);
}

ElementPtr ICollectionView::queryTooltipElement(const Position& absolutePosition, ElementArea& outArea) const
{
    return nullptr;
}

DragDropDataPtr ICollectionView::queryDragDropData(const input::BaseKeyFlags& keys, const Position& position) const
{
    return nullptr;
}

DragDropHandlerPtr ICollectionView::handleDragDrop(const DragDropDataPtr& data, const Position& entryPosition)
{
    return nullptr;
}

void ICollectionView::handleDragDropGenericCompletion(const DragDropDataPtr& data, const Position& entryPosition)
{

}

bool ICollectionView::processActivation()
{
    if (m_current)
    {
        if (m_current->item->handleItemActivate())
            return true;

        if (call(EVENT_ITEM_ACTIVATED, m_current->item))
            return true;
    }

    return false;
}

bool ICollectionView::processNavigation(bool shift, NavigationDirection mode)
{
    auto oldIndex = m_displayList.find(m_current);
    auto newIndex = resolveIndexNavigation(oldIndex, mode);

    if (newIndex != oldIndex && newIndex != INDEX_NONE)
    {
        auto newItem = m_displayList[newIndex];
        if (shift && oldIndex != INDEX_NONE)
        {
            int tailIndex = newIndex;
            int headIndex = oldIndex;
            if (!m_selection.empty())
                headIndex = m_displayList.find(m_selection.keys().front());

            Array<ViewItem*> newItems;
            newItems.reserve(tailIndex - headIndex + 1);

            if (headIndex < tailIndex)
            {
                for (auto i = headIndex; i <= tailIndex; ++i)
                    newItems.pushBack(m_displayList[i]);
            }
            else
            {
                for (auto i = headIndex; i >= tailIndex; --i)
                    newItems.pushBack(m_displayList[i]);
            }
            
            select(newItems, ItemSelectionModeBit::Default, true);
        }
        else
        {
            select(newItem, ItemSelectionModeBit::Default, true);
        }

        ensureVisible(newItem);
    }

    return true;
}

bool ICollectionView::handleKeyEvent(const input::KeyEvent& evt)
{
    if (evt.pressedOrRepeated())
    {
        const auto shift = evt.keyMask().isShiftDown();

        switch (evt.keyCode())
        {
        case input::KeyCode::KEY_UP:
            return processNavigation(shift, NavigationDirection::Up);

        case input::KeyCode::KEY_DOWN:
            return processNavigation(shift, NavigationDirection::Down);

        case input::KeyCode::KEY_PRIOR:
            return processNavigation(shift, NavigationDirection::PageUp);

        case input::KeyCode::KEY_NEXT:
            return processNavigation(shift, NavigationDirection::PageDown);

        case input::KeyCode::KEY_LEFT:
            return processNavigation(shift, NavigationDirection::Left);

        case input::KeyCode::KEY_RIGHT:
            return processNavigation(shift, NavigationDirection::Right);

        case input::KeyCode::KEY_HOME:
            return processNavigation(shift, NavigationDirection::Home);

        case input::KeyCode::KEY_END:
            return processNavigation(shift, NavigationDirection::End);

        case input::KeyCode::KEY_RETURN:
            return processActivation();
        }

        if (evt.deviceType() == input::DeviceType::Keyboard)
        {
            if (!evt.keyMask().isCtrlDown())
            {
                if (auto propagator = m_inputPropagationElement.lock())
                    if (propagator->handleExternalKeyEvent(evt))
                        return true;
            }
        }
    }

    return TBaseClass::handleKeyEvent(evt);
}

bool ICollectionView::handleCharEvent(const input::CharEvent& evt)
{
    if (!evt.keyMask().isCtrlDown())
    {
        if (auto propagator = m_inputPropagationElement.lock())
            if (propagator->handleExternalCharEvent(evt))
                return true;
    }

    return TBaseClass::handleCharEvent(evt);
}

//--

END_BOOMER_NAMESPACE_EX(ui)
