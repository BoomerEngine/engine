/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#pragma once

#include "uiListView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

class SimpleListViewModel;

//---

/// element of the simple list view 
class ENGINE_UI_API ISimpleListViewElement : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ISimpleListViewElement, IElement);

public:
    ISimpleListViewElement();
    virtual ~ISimpleListViewElement();

    /// get the model index of this item
    INLINE ModelIndex index() const { return m_index; }

    /// check sort ordering
    virtual bool compare(const ISimpleListViewElement* other) const;

    /// check filtering
    virtual bool filter(const SearchPattern& filter, int colIndex = 0) const;

private:
    ModelIndex m_index;
    int m_listIndex = -1;

    friend class SimpleListViewModel;
};

//---

/// list view that explicitly hold elements
class ENGINE_UI_API SimpleListView : public ListView
{
    RTTI_DECLARE_VIRTUAL_CLASS(SimpleListView, ListView);

public:
    SimpleListView();

    //--

    // remove all elements
    void clear();

    // add element to the list
    void add(ISimpleListViewElement* element);

    // remove element from the list
    void remove(ISimpleListViewElement* element);

    // select element on the list
    void select(ISimpleListViewElement* element, bool postEvent = true);

    // get selected element from the list
    ISimpleListViewElement* selected() const;

    //--

    // all elements
    const Array<RefPtr<ISimpleListViewElement>>& elements() const;

    //--

protected:
    RefPtr<SimpleListViewModel> m_container;
};

//---

END_BOOMER_NAMESPACE_EX(ui)

