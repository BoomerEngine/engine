/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#pragma once

#include "uiAbstractItemView.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

template< typename T >
struct ItemDisplayList
{
public:
    INLINE T* head() const
    {
        return m_head;
    }

    INLINE T* tail() const
    {
        return m_tail;
    }

    INLINE bool empty() const
    {
        return (nullptr == m_head);
    }

    INLINE void clear()
    {
        auto* next = m_head;
        for (auto* cur = m_head; cur != nullptr; cur = next)
        {
            next = cur->m_displayNext;
            cur->m_displayNext = nullptr;
            cur->m_displayPrev = nullptr;
            cur->m_displayIndex = INDEX_NONE;
        }

        m_head = nullptr;
        m_tail = nullptr;
        m_linearized.reset();
    }

    void pushBack(T* elem)
    {
        DEBUG_CHECK(elem->m_displayNext == nullptr);
        DEBUG_CHECK(elem->m_displayPrev == nullptr);
        DEBUG_CHECK(elem->m_displayIndex == INDEX_NONE);

        elem->m_displayNext = nullptr;
        elem->m_displayPrev = m_tail;

        if (nullptr != m_tail)
        {
            m_tail->m_displayNext = elem;
            elem->m_displayIndex = m_tail->m_displayIndex + 1;
        }
        else
        {
            elem->m_displayIndex = 0;
            m_head = elem;
        }

        m_tail = elem;
    }

    void unlink(T* elem)
    {
        if (elem->m_displayIndex != -1)
        {
            if (nullptr != elem->m_displayNext)
            {
                DEBUG_CHECK(m_tail != elem);
                elem->m_displayNext->m_displayPrev = elem->m_displayPrev;
            }
            else
            {
                DEBUG_CHECK(m_tail == elem);
                m_tail = elem->m_displayPrev;
            }

            if (nullptr != elem->m_displayPrev)
            {
                DEBUG_CHECK(m_head != elem);
                elem->m_displayPrev->m_displayNext = elem->m_displayNext;
            }
            else
            {
                DEBUG_CHECK(m_head == elem);
                m_head = elem->m_displayNext;
            }

            elem->m_displayPrev = nullptr;
            elem->m_displayNext = nullptr;
            elem->m_displayIndex = -1;
        }
        else
        {
            DEBUG_CHECK(elem->m_displayNext == nullptr);
            DEBUG_CHECK(elem->m_displayPrev == nullptr);
            DEBUG_CHECK(elem != m_head);
            DEBUG_CHECK(elem != m_tail);
        }

        m_linearized.reset();
    }

    const base::Array<T*>& linearizedList() const
    {
        if (m_linearized.empty() && m_head)
            buildLinearizedList();
        return m_linearized;
    }

private:
    T* m_head = nullptr;
    T* m_tail = nullptr;

    mutable base::Array<T*> m_linearized;

    void buildLinearizedList() const
    {
        m_linearized.reset();

        for (auto* cur = m_head; nullptr != cur; cur = cur->m_displayNext)
        {
            m_linearized.pushBack(cur);
            cur->m_displayIndex = m_linearized.lastValidIndex();
        }
    }
};

//---

template< typename T >
struct ItemPool
{
    T* alloc()
    {
        return new T;
    }

    void free(T* ptr)
    {
        delete ptr;
    }
};

//---

/// item navigation
enum class ItemNavigationDirection : uint8_t
{
    Left,
    Up,
    Right,
    Down,
    PageUp,
    PageDown,
    Home,
    End,
};

//---

/// generalized item view, can render lists/trees
class BASE_UI_API ItemView : public AbstractItemView
{
    RTTI_DECLARE_VIRTUAL_CLASS(ItemView, AbstractItemView);

public:
    ItemView();
    virtual ~ItemView();

    //--

    // get column count for this view, used if we have column header
    INLINE const uint32_t columnCount() const { return m_columnCount; }

    //--

    // find element under given mouse position
    ModelIndex indexAtPoint(const Position& pos) const;

    // find elements intersecting given area
    void indicesTouchingArea(const ElementArea& area, base::Array<ModelIndex>& outItems) const;

    /// change number of columns displayed by the view, in general should match the column header/model
    /// NOTE: this will invalidate all visualizations
    void columnCount(uint32_t count);

    /// recreate visualization of all items
    void recreateVisualization();

    /// run function on each visual item of given type
    template< typename T >
    INLINE void forEachVisualization(const std::function<void(T*)>& func) const { forEachVisualization(m_mainRows, func); }

    //--

    // ensure item is visible
    virtual void ensureVisible(const ModelIndex& index);

    //--

    // attach input propagation control - usually a search bar - all unconsumed input will be propagated there
    void bindInputPropagationElement(IElement* element);

protected:
    //--

    struct ViewItem;

    struct ViewItemChildren
    {
        base::Array<ViewItem*> m_orderedChildren;
        base::Array<ViewItem*> m_displayOrder; // sorted, empty indicates no sorting needed

        ViewItem* findItem(const ModelIndex& index) const;
    };

    struct ViewItem
    {
        ModelIndex m_index;
        ElementPtr m_content; // content element or the container
        ElementPtr m_innerContent;

        //int m_indexInParent = INDEX_NONE;
        //base::UntypedRefWeakPtr m_indexRefPtr;

        uint8_t m_depth = 0;
        ViewItem* m_parent = nullptr;
        ViewItem* m_displayNext = nullptr;
        ViewItem* m_displayPrev = nullptr;
        int m_displayIndex = INDEX_NONE;
        ElementArea m_unadjustedCachedArea;

        bool m_expanded = false;
        bool m_selectionFlag = false;
        bool m_currentFlag = false;

        ViewItemChildren m_children;
    };

    ItemPool<ViewItem> m_pool;
    ViewItemChildren m_mainRows;
    uint32_t m_columnCount = 1;

    ItemDisplayList<ViewItem> m_displayList;

    base::RefWeakPtr<IElement> m_inputPropagationElement;

    bool resolveItemFromModelIndex(const ModelIndex& index, ViewItem*& outItem) const;

    virtual void visualizeViewElement(ViewItem* item) = 0;
    virtual void unvisualizeViewElement(ViewItem* item) = 0;

    bool findViewElement(const ModelIndex& index, const ViewItemChildren& items, ViewItem*& outViewElement) const;
    bool findViewElement(const ModelIndex& index, ViewItem*& outViewElement) const;

    void discardElements(ViewItemChildren& items, ItemDisplayList<ViewItem>* displayList = nullptr);
    void discardElement(ViewItem* item, ItemDisplayList<ViewItem>* displayList = nullptr);
    void discardAllElements();

    void buildSortedList(const base::Array<ViewItem*>& rawItems, base::Array<ViewItem*>& outSortedItems) const;
    void buildSortedList(ViewItemChildren& items) const;

    bool buildDisplayList(const ViewItemChildren& items, ItemDisplayList<ViewItem>& outList) const;

    int findItemFromPrevRow(int displayIndex, float topY, float centerX) const;
    int findItemFromNextRow(int displayIndex, float bottomY, float centerX) const;

    bool buildIndicesFromDisplayRange(int firstDisplayIndex, int lastDisplayIndex, base::Array<ModelIndex>& outIndices) const;
    bool buildIndicesFromItemRange(const ViewItem* firstDisplayItem, const ViewItem* lastDisplayItem, base::Array<ModelIndex>& outIndices) const;
    bool buildIndicesFromIndexRange(const ModelIndex& firstIndex, const ModelIndex& lastIndex, base::Array<ModelIndex>& outIndices) const;

    ViewItem* itemAtPoint(const Position& pos) const;

    ModelIndex resolveIndexNavigation(ModelIndex current, ItemNavigationDirection mode) const;

    virtual void updateItem(ViewItem* item) = 0;
    void updateItemWithMode(ViewItem* item, ItemUpdateMode mode);

    void createVisualization(const ViewItemChildren& list);
    void removeVisualization(const ViewItemChildren& list);

    template< typename T >
    INLINE void forEachVisualization(const ViewItemChildren& items, const std::function<void(T*)>& func) const
    {
        for (const auto* child : items.m_orderedChildren)
        {
            if (auto elem = base::rtti_cast<T>(child->m_innerContent.get()))
                func(elem);
            else if (auto elem = base::rtti_cast<T>(child->m_content.get()))
                func(elem);

            forEachVisualization(child->m_children, func);
        }
    }

    virtual void rebuildSortedOrder() override final;
    virtual void rebuildDisplayList() override final;
    virtual void visualizeSelectionState(const ModelIndex& item, bool selected) override final;
    virtual void visualizeCurrentState(const ModelIndex& item, bool selected) override final;
    virtual bool iterateDrawChildren(ElementDrawListToken& token) const override final;
    virtual void focusElement(const ModelIndex& item) override final;

    // IElement
    virtual void handleFocusGained() override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
    virtual InputActionPtr handleOverlayMouseClick(const ElementArea &area, const base::input::MouseClickEvent &evt) override;
    virtual bool handleContextMenu(const ui::ElementArea &area, const ui::Position &absolutePosition, base::input::KeyMask controlKeys) override final;
    virtual ElementPtr queryTooltipElement(const Position& absolutePosition, ui::ElementArea& outArea) const override;
    virtual DragDropDataPtr queryDragDropData(const base::input::BaseKeyFlags& keys, const Position& position) const override;
    virtual DragDropHandlerPtr handleDragDrop(const DragDropDataPtr& data, const Position& entryPosition) override;
    virtual void handleDragDropGenericCompletion(const DragDropDataPtr& data, const Position& entryPosition) override;
    virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
    virtual bool handleCharEvent(const base::input::CharEvent& evt) override;
    virtual bool handleTemplateProperty(base::StringView name, base::StringView value) override;

    // IAbstractItemModelObserver
    virtual void modelItemUpdate(const ModelIndex& index, ItemUpdateMode mode) override;

private:
    mutable base::Array<ViewItem*> m_tempStorage;
};

//---

END_BOOMER_NAMESPACE(ui)

