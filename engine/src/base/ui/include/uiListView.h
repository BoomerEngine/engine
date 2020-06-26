/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#pragma once

#include "uiItemView.h"

namespace ui
{
    //---

    /// list view, can be used for both list box and a grid view (asset browser style)
    class BASE_UI_API ListView : public ItemView
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ListView, ItemView);

    public:
        ListView();

    protected:
        virtual void modelReset() override;
        virtual void modelRowsAboutToBeAdded(const ModelIndex& parent, int first, int count) override;
        virtual void modelRowsAdded(const ModelIndex& parent, int first, int count) override;
        virtual void modelRowsAboutToBeRemoved(const ModelIndex& parent, int first, int count) override;
        virtual void modelRowsRemoved(const ModelIndex& parent, int first, int count) override;

        virtual void visualizeViewElement(ViewItem* item) override;
        virtual void unvisualizeViewElement(ViewItem* item) override;
        virtual void destroyViewElement(ViewItem* item) override;

        virtual void updateItem(ViewItem* item) override;
    };

    //---

} // ui
