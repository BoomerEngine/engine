/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#pragma once

#include "uiCollectionView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

// element of the list view
class ENGINE_UI_API IListItem : public ICollectionItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(IListItem, ICollectionItem);

public:
    IListItem();
    virtual ~IListItem();

    INLINE ListViewEx* view() const { return (ListViewEx*)ICollectionItem::view(); }

    void removeSelfFromList();
};

//---

/// list view, a container for selectable items
class ENGINE_UI_API ListViewEx : public ICollectionView
{
    RTTI_DECLARE_VIRTUAL_CLASS(ListViewEx, ICollectionView);

public:
    ListViewEx();

    //--

    // clear list (remove all elements)
    void clear();

    // add list item
    void addItem(IListItem* item);

    // remove list item
    void removeItem(IListItem* item);

    //--

    // find element
    template< typename T >
    INLINE T* find(const std::function<bool(T*)>& func) const;

    // visit all elements
    template< typename T >
    INLINE void visit(const std::function<void(T*)>& func) const;

private:
    virtual void internalAttachItem(ViewItem* item) override;
    virtual void internalDetachItem(ViewItem* item) override;

    virtual void rebuildDisplayList() override;
};

//---

template< typename T >
INLINE T* ListViewEx::find(const std::function<bool(T*)>& func) const
{
    for (const auto* element : m_viewItems.keys())
        if (auto item = rtti_cast<T>(element->item.get()))
            if (func(item))
                return item;
    
    return nullptr;
}

template< typename T >
INLINE void ListViewEx::visit(const std::function<void(T*)>& func) const
{
    for (const auto* element : m_viewItems.keys())
        if (auto item = rtti_cast<T>(element->item.get()))
            func(item);
}

//---

END_BOOMER_NAMESPACE_EX(ui)

