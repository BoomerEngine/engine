/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#pragma once

#include "uiItemView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

/// tree view, used for items that can have children
class ENGINE_UI_API TreeView : public ItemView
{
    RTTI_DECLARE_VIRTUAL_CLASS(TreeView, ItemView);

public:
    TreeView();

    /// is item expanded ?
    bool isExpanded(const ModelIndex& index) const;

    /// expand all items
    void expandAll(bool recrusive=true);

    /// expand item, will expand  the parents as well
    void expandItem(const ModelIndex& index);

    /// collapse the item
    void collapseItem(const ModelIndex& index);

    /// collect expanded nodes
    void collectExpandedItems(Array<ModelIndex>& outIndices) const;

    /// make sure item is visible, on top of normal scrolling to show it we will also expand all parents
    virtual void ensureVisible(const ModelIndex& index) override;

protected:
    virtual void modelReset() override;
    virtual void modelItemsAdded(const ModelIndex& parent, const Array<ModelIndex>& items) override;
    virtual void modelItemsRemoved(const ModelIndex& parent, const Array<ModelIndex>& items) override;

    virtual bool handleKeyEvent(const InputKeyEvent& evt) override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt) override;

    void updateExpandStyle(ViewItem* item);
    void changeExpandState(ViewItem* item, bool state, bool recursive = false);

    void collectExpandedItems(const ViewItem* item, Array<ModelIndex>& outIndices) const;

    virtual void visualizeViewElement(ViewItem* item) override;
    virtual void unvisualizeViewElement(ViewItem* item) override;

    virtual void updateItem(ViewItem* item) override;

    void destroyViewElement(ViewItem* item);
};

//---

END_BOOMER_NAMESPACE_EX(ui)

