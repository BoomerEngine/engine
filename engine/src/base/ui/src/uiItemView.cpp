/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#include "build.h"
#include "uiItemView.h"
#include "uiWindow.h"
#include "uiInputAction.h"
#include "uiDragDrop.h"

#include "base/input/include/inputStructures.h"

namespace ui
{

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ItemView);
        RTTI_METADATA(ElementClassNameMetadata).name("ItemView");
    RTTI_END_TYPE();

    ItemView::ItemView()
        : OnItemActivated(this, "OnItemActivated"_id)
    {
        hitTest(true);
    }

    ItemView::~ItemView()
    {
        discardAllElements();
    }

    void ItemView::columnCount(uint32_t count)
    {
        if (m_columnCount != count)
        {
            m_columnCount = count;
            layoutMode(count ? ui::LayoutMode::Vertical : ui::LayoutMode::Icons);
            recreateVisualization();
        }
    }

    void ItemView::createVisualization(const ViewItemChildren& list)
    {
        for (auto* item : list.m_displayOrder)
        {
            visualizeViewElement(item);

            if (item->m_content)
            {
                if (item->m_selectionFlag)
                    item->m_content->addStyleClass("selected"_id);
                else
                    item->m_content->removeStyleClass("selected"_id);

                if (item->m_currentFlag)
                    item->m_content->addStyleClass("current"_id);
                else
                    item->m_content->removeStyleClass("current"_id);
            }

            createVisualization(item->m_children);
        }
    }

    void ItemView::removeVisualization(const ViewItemChildren& list)
    {
        for (auto* item : list.m_displayOrder)
        {
            unvisualizeViewElement(item);
            removeVisualization(item->m_children);
        }
    }

    void ItemView::recreateVisualization()
    {
        removeVisualization(m_mainRows);
        createVisualization(m_mainRows);
        invalidateLayout();
    }

    void ItemView::ensureVisible(const ModelIndex& index)
    {
        ViewItem* item = nullptr;
        if (findViewElement(index, item))
        {
            if (!item->m_unadjustedCachedArea.empty())
            {
                auto scrollPos = Position(-horizontalScrollOffset(), -verticalScrollOffset());
                scrollToMakeAreaVisible(item->m_unadjustedCachedArea.offset(scrollPos));
            }
        }
    }

    void ItemView::bindInputPropagationElement(IElement* element)
    {
        m_inputPropagationElement = element;
    }

    void ItemView::discardElements(ViewItemChildren& items, ItemDisplayList<ViewItem>* displayList /*= nullptr*/)
    {
        for (auto* elem : items.m_orderedChildren)
            discardElement(elem, displayList);
        items.m_orderedChildren.reset();
        items.m_displayOrder.reset();
    }

    void ItemView::discardElement(ViewItem* item, ItemDisplayList<ViewItem>* displayList /*= nullptr*/)
    {
        if (item->m_content)
            detachChild(item->m_content);

        item->m_innerContent.reset();

        if (nullptr != displayList)
            displayList->unlink(item);

        discardElements(item->m_children, displayList);

        m_pool.free(item);
    }

    void ItemView::discardAllElements()
    {
        m_displayList.clear();
        discardElements(m_mainRows, nullptr);
    }

    void ItemView::rebuildDisplayList()
    {
        m_displayList.clear();

        if (nullptr != m_model)
            buildDisplayList(m_mainRows, m_displayList);

        invalidateLayout();
    }

    void ItemView::rebuildSortedOrder()
    {
        if (nullptr != m_model)
            buildSortedList(m_mainRows);
    }

    void ItemView::buildSortedList(const base::Array<ViewItem*>& rawItems, base::Array<ViewItem*>& outSortedItems) const
    {
        outSortedItems.reset();

        if (m_sortingColumnIndex != INDEX_NONE)
        {
            outSortedItems.reserve(rawItems.size());
            outSortedItems = rawItems;

            if (m_sortingAsc)
            {
                std::sort(outSortedItems.begin(), outSortedItems.end(), [this](const ViewItem *a, const ViewItem *b)
                {
                    return m_model->compare(a->m_index, b->m_index, m_sortingColumnIndex);
                });
            }
            else
            {
                std::sort(outSortedItems.begin(), outSortedItems.end(), [this](const ViewItem *a, const ViewItem *b)
                {
                    return m_model->compare(b->m_index, a->m_index, m_sortingColumnIndex);
                });
            }
        }
    }

    void ItemView::buildSortedList(ViewItemChildren& items) const
    {
        buildSortedList(items.m_orderedChildren, items.m_displayOrder);
    }

    bool ItemView::buildDisplayList(const ViewItemChildren& items, ItemDisplayList<ViewItem>& outList) const
    {
        // use the sorted elements if we have them, otherwise use the normal elements
        const auto& elements = items.m_displayOrder.empty() ? items.m_orderedChildren : items.m_displayOrder;

        // no filtering case, fast
        if (m_filter.empty())
        {
            for (auto *item : elements)
            {
                outList.pushBack(item);

                if (item->m_expanded)
                    buildDisplayList(item->m_children, outList);
            }

            return !elements.empty();
        }
        else
        {
            bool somethingAdded = false;
            for (auto *item : elements)
            {
                outList.pushBack(item); // always add, we may remove later

                auto filterThis = m_model->filter(item->m_index, m_filter);
                auto filterChild = buildDisplayList(item->m_children, outList); // ignore expand flag

                // ok, if the item is not needed than it can be removed
                if (!filterThis && !filterChild)
                    outList.unlink(item);
                else
                    somethingAdded = true;
            }

            return somethingAdded;
        }
    }

    bool ItemView::resolveItemFromModelIndex(const ModelIndex& index, ViewItem*& outItem) const
    {
        // the "null" root item that is always there
        if (!index)
        {
            outItem = nullptr;
            return true;
        }

        // if we have a parent than resolve it first
        auto parent = index.parent();
        if (!parent)
        {
            // this is a root item
            if (index.row() >= 0 && index.row() <= m_mainRows.m_orderedChildren.lastValidIndex())
            {
                outItem = m_mainRows.m_orderedChildren[index.row()];
                return true;
            }

            // invalid index
            return false;
        }

        // resolve parent first
        ViewItem* parentItem = nullptr;
        if (!resolveItemFromModelIndex(parent, parentItem))
            return false;

        // this is a root item
        if (index.row() >= 0 && index.row() <= parentItem->m_children.m_orderedChildren.lastValidIndex())
        {
            outItem = parentItem->m_children.m_orderedChildren[index.row()];
            return true;
        }

        // invalid index
        return false;
    }

    void ItemView::attachViewElement(ViewItem* parentItemContainer, const ModelIndex& index)
    {
        auto* item = m_pool.alloc();
        item->m_index = index;
        item->m_parent = parentItemContainer;

        if (nullptr != parentItemContainer)
            item->m_depth = parentItemContainer->m_depth + 1;

        auto& container = (nullptr != parentItemContainer) ? parentItemContainer->m_children.m_orderedChildren : m_mainRows.m_orderedChildren;

        container.prepareWith(index.row() + 1, nullptr);
        ASSERT_EX(container[index.row()] == nullptr, "Slot for given row is already occupied");
        container[index.row()] = item;

        visualizeViewElement(item);
    }

    void ItemView::attachViewElement(const ModelIndex& index)
    {
        // resolve the parent container
        ViewItem* parentItemContainer = nullptr;
        if (!resolveItemFromModelIndex(index.parent(), parentItemContainer))
        {
            TRACE_ERROR("Unable to attach view item {}, unable to resovle parent", index);
            return;
        }

        attachViewElement(parentItemContainer, index);
    }

    bool ItemView::findViewElement(const ModelIndex& index, const ViewItemChildren& items, ViewItem*& outViewElement) const
    {
        // index is out of range, probably old
        if (index.row() < 0 || index.row() > items.m_orderedChildren.lastValidIndex())
            return false;

        // still, check that we are pointing to the same data in case of index shift
        auto* item = items.m_orderedChildren[index.row()];
        if (item->m_index != index)
            return false;

        // item seems to match
        outViewElement = item;
        return true;
    }

    bool ItemView::findViewElement(const ModelIndex& index, ViewItem*& outViewElement) const
    {
        // no element/root  
        if (!index)
            return false;

        // elements without parent are the top element
        auto parent = index.parent();
        if (!parent)
            return findViewElement(index, m_mainRows, outViewElement);

        // resolve the parent item first
        ViewItem* parentItem = nullptr;
        if (!findViewElement(parent, parentItem))
            return false;

        // find us in the parent
        return findViewElement(index, parentItem->m_children, outViewElement);
    }

    void ItemView::visualizeSelectionState(const ModelIndex& index, bool selected)
    {
        ViewItem* item = nullptr;
        if (findViewElement(index, item))
        {
            if (item->m_selectionFlag != selected)
            {
                item->m_selectionFlag = selected;

                if (item->m_content)
                {
                    if (selected)
                        item->m_content->addStyleClass("selected"_id);
                    else
                        item->m_content->removeStyleClass("selected"_id);
                }
            }
        }
    }

    void ItemView::visualizeCurrentState(const ModelIndex& index, bool selected)
    {
        ViewItem* item = nullptr;
        if (findViewElement(index, item))
        {
            if (item->m_currentFlag != selected)
            {
                item->m_currentFlag = selected;

                if (item->m_content)
                {
                    if (selected)
                        item->m_content->addStyleClass("current"_id);
                    else
                        item->m_content->removeStyleClass("current"_id);
                }
            }
        }
    }

    bool ItemView::iterateDrawChildren(ElementDrawListToken& token) const
    {
        if (nullptr == token.curToken)
        {
            if (nullptr == m_displayList.head())
                return false;

            token.curToken = nullptr;
            token.nextToken = m_displayList.head();
        }

        while (token.nextToken != nullptr)
        {
            token.curToken = token.nextToken;

            auto* viewElem = (ViewItem*)token.curToken;
            token.nextToken = viewElem->m_displayNext;
            token.elem = viewElem->m_content.get();
            if (nullptr != token.elem)
            {
                token.m_unadjustedArea = &viewElem->m_unadjustedCachedArea;
                return true;
            }
        }

        return false;
    }

    //--

    ItemView::ViewItem* ItemView::itemAtPoint(const Position& pos) const
    {
        // build/get a linearized list of items in their display order
        const auto &linearizedList = m_displayList.linearizedList();

        /*// find first element in range
        auto item = std::lower_bound(linearizedList.begin(), linearizedList.end(), pos.y, [](const ViewItem *a, float pos)
        {
            return a->m_content->cachedDrawArea().absolutePosition().y < pos;
        });

        // make sure that the item we've found really intersects
        if (item != linearizedList.end())
            if ((*item)->m_content->cachedDrawArea().contains(pos))
                return *item;*/

        auto scrolledPos = pos + Position(horizontalScrollOffset(), verticalScrollOffset());

        for (auto* item : linearizedList)
        {
            const auto& area = item->m_unadjustedCachedArea;
            if (!area.empty() && area.contains(scrolledPos))
            {
                return item;
            }
        }

        // nothing
        return nullptr;
    }

    ModelIndex ItemView::indexAtPoint(const Position& pos) const
    {
        const auto* item = itemAtPoint(pos);
        return item ? item->m_index : ModelIndex();
    }

    void ItemView::indicesTouchingArea(const ElementArea& area, base::Array<ModelIndex>& outItems) const
    {
        // build/get a linearized list of items in their display order
        const auto &linearizedList = m_displayList.linearizedList();

        // find first element in range
        auto first = std::lower_bound(linearizedList.begin(), linearizedList.end(), area.absolutePosition().y, [](const ViewItem *a, float pos)
        {
            return a->m_content->cachedDrawArea().absolutePosition().y < pos;
        });

        // find last in range
        auto last = std::upper_bound(linearizedList.begin(), linearizedList.end(), area.absolutePosition().y, [](float pos, const ViewItem *a)
        {
            return pos < a->m_content->cachedDrawArea().absolutePosition().y;
        });

        // filter the ones really intersecting
        for (auto it=first; it < last; ++it)
        {
            if ((*it)->m_content->cachedDrawArea().touches(area))
                outItems.pushBack((*it)->m_index);
        }
    }

    int ItemView::findItemFromPrevRow(int displayIndex, float topY, float centerX) const
    {
        const auto& displayList = m_displayList.linearizedList();
        const auto& itemArea = displayList[displayIndex]->m_unadjustedCachedArea;
        if (itemArea.empty())
            return displayIndex;

        // find start of next row
        bool hasPrevRow = false;
        auto testIndex = displayIndex;
        while (testIndex > 0)
        {
            const auto& testArea = displayList[testIndex]->m_unadjustedCachedArea;
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
            const auto &testArea = displayList[testIndex]->m_unadjustedCachedArea;
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

    int ItemView::findItemFromNextRow(int displayIndex, float bottomY, float centerX) const
    {
        const auto& displayList = m_displayList.linearizedList();
        auto lastItem = displayList.lastValidIndex();
        const auto& itemArea = displayList[displayIndex]->m_unadjustedCachedArea;
        if (itemArea.empty())
            return displayIndex;

        // find start of next row
        bool hasRow = false;
        auto testIndex = displayIndex;
        while (testIndex <= lastItem)
        {
            const auto &testArea = displayList[testIndex]->m_unadjustedCachedArea;
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
            const auto &testArea = displayList[testIndex]->m_unadjustedCachedArea;
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

    ModelIndex ItemView::resolveIndexNavigation(ModelIndex current, ItemNavigationDirection mode) const
    {
        // invalid item, go to first
        if (!current)
        {
            if (!m_displayList.empty())
            {
                if (mode == ItemNavigationDirection::Home || mode == ItemNavigationDirection::Left || mode == ItemNavigationDirection::Up || mode == ItemNavigationDirection::PageUp)
                    return m_displayList.head()->m_index;
                else
                    return m_displayList.tail()->m_index;
            }

            return ModelIndex();
        }

        // find the item
        ViewItem* item = nullptr;
        if (!findViewElement(current, item))
            return ModelIndex(); // we don't view that element currently

        // get the linear list, we will use it a lot
        const auto& displayList = m_displayList.linearizedList();
        if (item->m_displayIndex < 0 || item->m_displayIndex > displayList.lastValidIndex())
        {
            if (!m_displayList.empty())
            {
                if (mode == ItemNavigationDirection::End)
                    return m_displayList.tail()->m_index;
                else
                    return m_displayList.head()->m_index;
            }

            return ModelIndex();
        }

        // check that we are indeed there
        if (displayList[item->m_displayIndex] != item)
            return ModelIndex(); // no proper display index

        // common modes
        if (ItemNavigationDirection::Home == mode)
            return displayList.front()->m_index;
        else if (ItemNavigationDirection::End == mode)
            return displayList.back()->m_index;

        // get area of the item
        const auto& displayArea = item->m_content->cachedDrawArea();

        // get the prev/next keys for current model
        if (layoutMode() == LayoutMode::Icons)
        {
            if (ItemNavigationDirection::Up == mode)
            {
                auto newDisplayIndex = findItemFromPrevRow(item->m_displayIndex, displayArea.top() + verticalScrollOffset(), displayArea.center().x + horizontalScrollOffset());
                return displayList[newDisplayIndex]->m_index;
            }
            else if (ItemNavigationDirection::Down == mode)
            {
                auto newDisplayIndex = findItemFromNextRow(item->m_displayIndex, displayArea.bottom() + verticalScrollOffset(), displayArea.center().x + horizontalScrollOffset());
                return displayList[newDisplayIndex]->m_index;
            }
            else if (ItemNavigationDirection::Left == mode)
            {
                auto prevIndex = std::max<int>(item->m_displayIndex - 1, 0);
                return displayList[prevIndex]->m_index;
            }
            else if (ItemNavigationDirection::Right == mode)
            {
                auto nextIndex = std::min<int>(item->m_displayIndex + 1, displayList.lastValidIndex());
                return displayList[nextIndex]->m_index;
            }
        }
        else
        {
            if (ItemNavigationDirection::Up == mode)
            {
                auto prevIndex = std::max<int>(item->m_displayIndex - 1, 0);
                return displayList[prevIndex]->m_index;
            }
            else if (ItemNavigationDirection::Down == mode)
            {
                auto nextIndex = std::min<int>(item->m_displayIndex + 1, displayList.lastValidIndex());
                return displayList[nextIndex]->m_index;
            }
            else if (ItemNavigationDirection::Left == mode)
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
            }
        }

        // page up/page.down
        if (ItemNavigationDirection::PageUp == mode)
        {
            auto pageSize = cachedDrawArea().size().y;
            auto newDisplayIndex = findItemFromPrevRow(item->m_displayIndex, displayArea.top() - pageSize + verticalScrollOffset(), displayArea.center().x + horizontalScrollOffset());
            return displayList[newDisplayIndex]->m_index;
        }
        else if (ItemNavigationDirection::PageDown == mode)
        {
            auto pageSize = cachedDrawArea().size().y;
            auto newDisplayIndex = findItemFromNextRow(item->m_displayIndex, displayArea.bottom() + pageSize + verticalScrollOffset(), displayArea.center().x + horizontalScrollOffset());
            return displayList[newDisplayIndex]->m_index;
        }

        // nothing changed, return the same item
        return current;
    }

    //--

    bool ItemView::buildIndicesFromDisplayRange(int firstDisplayIndex, int lastDisplayIndex, base::Array<ModelIndex>& outIndices) const
    {
        const auto& linearList = m_displayList.linearizedList();

        if (lastDisplayIndex >= firstDisplayIndex)
        {
            for (int i = firstDisplayIndex; i <= lastDisplayIndex; ++i)
                outIndices.pushBack(linearList[i]->m_index);
        }
        else
        {
            for (int i = firstDisplayIndex; i >= lastDisplayIndex; --i)
                outIndices.pushBack(linearList[i]->m_index);
        }

        return true;
    }

    bool ItemView::buildIndicesFromItemRange(const ViewItem* firstDisplayItem, const ViewItem* lastDisplayItem, base::Array<ModelIndex>& outIndices) const
    {
        if (firstDisplayItem && lastDisplayItem)
            if (firstDisplayItem->m_displayIndex != INDEX_NONE && lastDisplayItem->m_displayIndex != INDEX_NONE)
                return buildIndicesFromDisplayRange(firstDisplayItem->m_displayIndex, lastDisplayItem->m_displayIndex, outIndices);

        return false;
    }

    bool ItemView::buildIndicesFromIndexRange(const ModelIndex& firstIndex, const ModelIndex& lastIndex, base::Array<ModelIndex>& outIndices) const
    {
        ViewItem *firstItem = nullptr, *lastItem = nullptr;
        if (findViewElement(firstIndex, firstItem) && findViewElement(lastIndex, lastItem))
            return buildIndicesFromItemRange(firstItem, lastItem, outIndices);

        return false;
    }
      
    void ItemView::modelItemUpdate(const ModelIndex& index, ItemUpdateMode mode)
    {
        ViewItem* item = nullptr;
        if (resolveItemFromModelIndex(index, item))
        {
            updateItemWithMode(item, mode);
        }
    }

    void ItemView::updateItemWithMode(ViewItem* item, ItemUpdateMode mode)
    {
        if (item == nullptr)
            return;

        if (mode.test(ItemUpdateModeBit::Parents) && item->m_parent)
            updateItemWithMode(item->m_parent, (mode - ItemUpdateModeBit::Children) | ItemUpdateModeBit::Item); // update parent but don't recurse back

        if (mode.test(ItemUpdateModeBit::Item))
            updateItem(item);

        if (mode.test(ItemUpdateModeBit::Children))
            for (auto* child : item->m_children.m_orderedChildren)
                updateItemWithMode(item, (mode - ItemUpdateModeBit::Parents) | ItemUpdateModeBit::Item); // don't visit parents
    }

    //--

    void ItemView::handleFocusGained()
    {
        return TBaseClass::handleFocusGained();
    }

    InputActionPtr ItemView::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.rightClicked())
        {
            auto index = indexAtPoint(evt.absolutePosition().toVector());
            if (!m_selection.contains(index))
                select(index, ItemSelectionModeBit::Default);
            return InputActionPtr();
        }
        else if (evt.leftDoubleClicked())
        {
            auto index = indexAtPoint(evt.absolutePosition().toVector());
            if (index)
            {
                select(index, ItemSelectionModeBit::Default);
                OnItemActivated(index);
                return InputActionPtr();
            }
        }
        else if (evt.leftClicked())
        {
            auto clickedItem = indexAtPoint(evt.absolutePosition().toVector());
            if (evt.keyMask().isShiftDown())
            {
                if (selectionRoot() && clickedItem) // at lest one element should be selected as the root
                {
                    base::InplaceArray<ModelIndex, 64> selectedIndices;
                    if (buildIndicesFromIndexRange(selectionRoot(), clickedItem, selectedIndices))
                        select(selectedIndices, ItemSelectionMode(ItemSelectionModeBit::Clear) | ItemSelectionModeBit::Select | ItemSelectionModeBit::UpdateCurrent);
                }
            }
            else if (evt.keyMask().isCtrlDown())
            {
                if (clickedItem)
                    select(clickedItem, ItemSelectionMode(ItemSelectionModeBit::Toggle) | ItemSelectionModeBit::UpdateCurrent);
            }
            else
            {
                select(clickedItem, ItemSelectionModeBit::Default);
            }
            return InputActionPtr();
        }        

        return TBaseClass::handleMouseClick(area, evt);
    }

    InputActionPtr ItemView::handleOverlayMouseClick(const ElementArea &area, const base::input::MouseClickEvent &evt)
    {
        if (evt.leftClicked())
        {
            auto index = indexAtPoint(evt.absolutePosition().toVector());
            select(index, ItemSelectionModeBit::Default);
        }

        return TBaseClass::handleOverlayMouseClick(area, evt);
    }

    bool ItemView::handleContextMenu(const ui::ElementArea &area, const ui::Position &absolutePosition)
    {
        if (auto index = indexAtPoint(absolutePosition))
        {
            auto items = selection().keys();
            if (!selection().contains(index))
            {
                items.reset();
                items.pushBack(index);
            }

            if (auto menu = m_model->contextMenu(this, items))
            {
                menu->show(this, PopupWindowSetup().relativeToCursor().topLeft().interactive().autoClose());
                return true;
            }
        }

        return TBaseClass::handleContextMenu(area, absolutePosition);
    }

    ElementPtr ItemView::queryTooltipElement(const Position& absolutePosition) const
    {
        if (auto index = indexAtPoint(absolutePosition))
            if (auto tooltip = m_model->tooltip(const_cast<ItemView*>(this), index))
                return tooltip;

        return TBaseClass::queryTooltipElement(absolutePosition);
    }

    DragDropDataPtr ItemView::queryDragDropData(const base::input::BaseKeyFlags& keys, const Position& position) const
    {
        if (auto index = indexAtPoint(position))
        {
            if (auto ret = m_model->queryDragDropData(keys, index))
                return ret;
        }

        return TBaseClass::queryDragDropData(keys, position);
    }

    DragDropHandlerPtr ItemView::handleDragDrop(const DragDropDataPtr& data, const Position& entryPosition)
    {
        if (auto index = indexAtPoint(entryPosition))
        {
            if (auto ret = m_model->handleDragDropData(this, index, data, entryPosition))
                return ret;
        }

        return TBaseClass::handleDragDrop(data, entryPosition);
    }

    void ItemView::handleDragDropGenericCompletion(const DragDropDataPtr& data, const Position& entryPosition)
    {
        if (auto index = indexAtPoint(entryPosition))
        {
            if (m_model->handleDragDropCompletion(this, index, data))
                return;
        }

        return TBaseClass::handleDragDropGenericCompletion(data, entryPosition);
    }

    bool ItemView::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        if (name == "columns")
        {
            int val = 0;
            if (base::MatchResult::OK != value.match(val))
                return false;
            columnCount(val);
            return true;
        }

        return TBaseClass::handleTemplateProperty(name, value);
    }

    bool ItemView::handleCharEvent(const base::input::CharEvent& evt)
    {
        if (!evt.keyMask().isCtrlDown())
        {
            if (auto propagator = m_inputPropagationElement.lock())
                if (propagator->handleExternalCharEvent(evt))
                    return true;
        }

        return TBaseClass::handleCharEvent(evt);
    }

    bool ItemView::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (evt.pressedOrRepeated())
        {
            ModelIndex oldIndex = m_current;
            ModelIndex nextIndex = oldIndex;
            bool processed = false;

            switch (evt.keyCode())
            {
                case base::input::KeyCode::KEY_UP:
                {
                    nextIndex = resolveIndexNavigation(m_current, ItemNavigationDirection::Up);
                    processed = true;
                    break;
                }

                case base::input::KeyCode::KEY_DOWN:
                {
                    nextIndex = resolveIndexNavigation(m_current, ItemNavigationDirection::Down);
                    processed = true;
                    break;
                }

                case base::input::KeyCode::KEY_PRIOR:
                {
                    nextIndex = resolveIndexNavigation(m_current, ItemNavigationDirection::PageUp);
                    processed = true;
                    break;
                }

                case base::input::KeyCode::KEY_NEXT:
                {
                    nextIndex = resolveIndexNavigation(m_current, ItemNavigationDirection::PageDown);
                    processed = true;
                    break;
                }

                case base::input::KeyCode::KEY_LEFT:
                {
                    nextIndex = resolveIndexNavigation(m_current, ItemNavigationDirection::Left);
                    processed = true;
                    break;
                }

                case base::input::KeyCode::KEY_RIGHT:
                {
                    nextIndex = resolveIndexNavigation(m_current, ItemNavigationDirection::Right);
                    processed = true;
                    break;
                }

                case base::input::KeyCode::KEY_HOME:
                {
                    nextIndex = resolveIndexNavigation(m_current, ItemNavigationDirection::Home);
                    processed = true;
                    break;
                }

                case base::input::KeyCode::KEY_END:
                {
                    nextIndex = resolveIndexNavigation(m_current, ItemNavigationDirection::End);
                    processed = true;
                    break;
                }
            }

            if (processed)
            {
                if (evt.keyMask().isShiftDown())
                {

                }
                else
                {
                    if (nextIndex)
                        select(nextIndex, ItemSelectionModeBit::Default);
                }

                if (nextIndex != oldIndex)
                    ensureVisible(nextIndex);

                return true;
            }

            if (m_current)
            {
                if (evt.keyCode() == base::input::KeyCode::KEY_RETURN)
                {
                    return OnItemActivated(m_current);
                }                   
            }

            if (evt.deviceType() == base::input::DeviceType::Keyboard)
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
   
    //--

} // ui
