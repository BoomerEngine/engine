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

/// list view, can be used for both list box and a grid view (asset browser style)
class ENGINE_UI_API ListView : public ItemView
{
    RTTI_DECLARE_VIRTUAL_CLASS(ListView, ItemView);

public:
    ListView();

protected:
    virtual void modelReset() override;
    virtual void modelItemsAdded(const ModelIndex& parent, const Array<ModelIndex>& items) override;
    virtual void modelItemsRemoved(const ModelIndex& parent, const Array<ModelIndex>& items) override;

    virtual void visualizeViewElement(ViewItem* item) override;
    virtual void unvisualizeViewElement(ViewItem* item) override;

    virtual void updateItem(ViewItem* item) override;

    void destroyViewElement(ViewItem* item);
};

//---

END_BOOMER_NAMESPACE_EX(ui)

