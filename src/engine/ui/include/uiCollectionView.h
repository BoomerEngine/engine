/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#pragma once

#include "uiScrollArea.h"

#include "core/memory/include/structurePool.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

/// abstract element of the list, directly added/removed from the list via "addItem/removeItem"
class ENGINE_UI_API ICollectionItem : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ICollectionItem, IElement);

public:
    ICollectionItem();

    struct SortingData
    {
        int type = 0;
        int index = 0;
        StringView caption;
    };

    // internal index, globally unique and monotonic, allows to compare items for general age
    INLINE uint32_t uniqueIndex() const { return m_uniqueIndex; }

    // get list we are part of, can be null for items never added to the list or if the list got removed
    INLINE ICollectionView* view() const { return m_view.unsafe(); }

    // is the element currently in the selection set ?
    INLINE bool selected() const { return m_selected; }

    // are we the current element (selection head)
    INLINE bool current() const { return m_current; }

    ///--

    /// check if item should be displayed under given filter
    virtual bool handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const;

    /// check sorting relation with other item type
    /// NOTE: the item may be of different type
    virtual void handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outInfo) const;

    /// handle context menu on this item
    virtual bool handleItemContextMenu(ui::ICollectionView* view, const ui::CollectionItems& items, const ui::Position& pos, input::KeyMask controlKeys);

    // called when this item gets selected
    virtual bool handleItemSelected(ui::ICollectionView* view);

    // called when this item get's unselected
    virtual void handleItemUnselected(ui::ICollectionView* view);

    // called when this item becomes the current one
    virtual void handleItemFocused(ui::ICollectionView* view);

    // called when this item stops being the current one
    virtual void handleItemUnfocused(ui::ICollectionView* view);

    // handle double click/enter
    virtual bool handleItemActivate(ui::ICollectionView* view);

    //--

private:
    CollectionViewWeakPtr m_view;
    void* m_viewItem = nullptr;

    bool m_selected = false;
    bool m_current = false;

    uint32_t m_uniqueIndex = 0;

    friend class ICollectionView;
    friend class ListViewEx;

protected:
    INLINE void* viewItem() const { return m_viewItem; }

    void invalidateDisplayList();
    void internalCreateChildViewItem(ICollectionItem* child);
    void internalDestroyViewItem();
};

//---

/// collection of items
class ENGINE_UI_API CollectionItems
{
public:
    CollectionItems();
    CollectionItems(const CollectionItems& other);
    CollectionItems(CollectionItems&& other);
    CollectionItems& operator=(const CollectionItems& other);
    CollectionItems& operator=(CollectionItems&& other);

    INLINE bool empty() const { return m_items.empty(); }

    ICollectionItem* head() const;

    ICollectionItem* tail() const;

    bool contains(const ICollectionItem* item) const;

    void clear();

    bool add(ICollectionItem* item);

    bool remove(ICollectionItem* item);

    ///---

    INLINE const HashSet<CollectionItemPtr>& items() const { return m_items; }

    template< typename T >
    INLINE RefPtr<T> first(const std::function<bool(T*)>& func = nullptr) const
    {
        for (const auto& item : m_items.keys())
            if (auto typedItem = rtti_cast<T>(item.get()))
                if (!func || func(typedItem))
                    return RefPtr<T>(AddRef(typedItem));

        return nullptr;
    }

    template< typename T >
    INLINE Array<RefPtr<T>> collect(const std::function<bool(T*)>& func = nullptr) const
    {
        Array<RefPtr<T>> ret;

        for (const auto& item : m_items.keys())
            if (auto typedItem = rtti_cast<T>(item.get()))
                if (!func || func(typedItem))
                    ret.pushBack(AddRef(typedItem));

        return ret;
    }

    template< typename T >
    INLINE void visit(const std::function<void(T*)>& func) const
    {
        for (const auto& item : m_items.keys())
            if (auto typedItem = rtti_cast<T>(item.get()))
                func(typedItem);
    }

    ///---

protected:
    HashSet<CollectionItemPtr> m_items;
};

//---

/// item view, a container for selectable items - both tree and list
class ENGINE_UI_API ICollectionView : public ScrollArea
{
    RTTI_DECLARE_VIRTUAL_CLASS(ICollectionView, ScrollArea);

public:
    ICollectionView();

    //--

    // get all selected items
    INLINE const CollectionItems& selection() const { return m_selectionItems; }

    // no items ?
    INLINE bool empty() const { return m_viewItems.empty(); }

    //--

    /// set new selection
    void select(ICollectionItem* item, ItemSelectionMode mode = ItemSelectionModeBit::Default, bool postEvent = true);

    /// set new selection for a set of items
    void select(const CollectionItems& items, ItemSelectionMode mode = ItemSelectionModeBit::Default, bool postEvent = true);

    // ensure item is visible
    void ensureVisible(ICollectionItem* item);

    //--

    // get currently applied filter pattern
    INLINE const SearchPattern& filter() const { return m_filter; }

    /// bind filter, changes the current display list
    void filter(const SearchPattern& filter);

    /// enable sorting by given column, set to INDEX_NONE to disable
    void sort(int colIndex = INDEX_NONE, bool asc = true);

    //--

    // attach static list element (non selectable, always displayed), usually used for "Add +" buttons
    void attachStaticElement(IElement* element);

    // attach static list element (non selectable, always displayed), usually used for "Add +" buttons
    void detachStaticElement(IElement* element);

    //--

    // attach input propagation control - usually a search bar - all unconsumed input will be propagated there
    void bindInputPropagationElement(IElement* element);

    //--

    /// current item of type
    template< typename T >
    INLINE T* current() const { return m_current ? rtti_cast<T>(m_current->item.get()) : nullptr; }

protected:
    struct ViewItem
    {
        CollectionItemPtr item;
        ElementPtr staticElement;
        ElementArea unadjustedCachedArea;

        bool select(ICollectionView* view);
        void unselect(ICollectionView* view);

        void focus(ICollectionView* view);
        void unfocus(ICollectionView* view);
    };

    StructurePool<ViewItem> m_viewItemsPool;
    HashSet<ViewItem*> m_viewItems; // all of them, unordered

    ViewItem* internalAddItem(ICollectionItem* item);
    void internalRemoveItem(ViewItem* item);

    virtual void internalAttachItem(ViewItem* item) = 0;
    virtual void internalDetachItem(ViewItem* item) = 0;

    //--

    SearchPattern m_filter;

    Array<ViewItem*> m_staticElements;

    RefWeakPtr<IElement> m_inputPropagationElement;

    int m_sortingColumnIndex = INDEX_NONE;
    bool m_sortingAsc = true;

    void sortViewItems(Array<ViewItem*>& items) const;

    //--

    mutable ViewItem* m_current = nullptr;
    HashSet<ViewItem*> m_selection;

    CollectionItems m_selectionItems;

    void select(const Array<ViewItem*>& items, ItemSelectionMode mode, bool postEvent);
    void select(ViewItem* item, ItemSelectionMode mode, bool postEvent);

    void focusElement(ViewItem* item);
    void ensureVisible(ViewItem* item);

    //--

    Array<ViewItem*> m_displayList;
    bool m_displayListInvalid = false;

    ViewItem* itemAtPoint(const Position& pos) const;
    void itemsAtArea(const ElementArea& area, Array<ViewItem*>& outItems) const;

    void validateDisplayList();
    void invalidateDisplayList();
    virtual void rebuildDisplayList() = 0;

    int findItemFromPrevRow(int displayIndex, float topY, float centerX) const;
    int findItemFromNextRow(int displayIndex, float bottomY, float centerX) const;

    enum class NavigationDirection : uint8_t
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

    int resolveIndexNavigation(int current, NavigationDirection mode) const;

    virtual bool processNavigation(bool shift, NavigationDirection mode);
    virtual bool processActivation();

    //--

    // IElement
    virtual void handleFocusGained() override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const input::MouseClickEvent& evt) override;
    virtual InputActionPtr handleOverlayMouseClick(const ElementArea& area, const input::MouseClickEvent& evt) override;
    virtual bool handleContextMenu(const ElementArea& area, const Position& absolutePosition, input::KeyMask controlKeys) override final;
    virtual ElementPtr queryTooltipElement(const Position& absolutePosition, ElementArea& outArea) const override;
    virtual DragDropDataPtr queryDragDropData(const input::BaseKeyFlags& keys, const Position& position) const override;
    virtual DragDropHandlerPtr handleDragDrop(const DragDropDataPtr& data, const Position& entryPosition) override;
    virtual void handleDragDropGenericCompletion(const DragDropDataPtr& data, const Position& entryPosition) override;
    virtual bool handleKeyEvent(const input::KeyEvent& evt) override;
    virtual bool handleCharEvent(const input::CharEvent& evt) override;

    virtual bool iterateDrawChildren(ElementDrawListToken& token) const override final;

    //--

    friend class ICollectionItem;
};

//---

END_BOOMER_NAMESPACE_EX(ui)
