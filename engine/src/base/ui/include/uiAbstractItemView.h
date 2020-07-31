/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: elements\view #]
***/

#pragma once

#include "uiAbstractItemModel.h"
#include "uiScrollArea.h"
#include "uiElement.h"

#include "base/containers/include/hashSet.h"
#include "base/containers/include/inplaceArray.h"

namespace ui
{
    //---

    DECLARE_UI_EVENT(EVENT_ITEM_SELECTION_CHANGED)
    DECLARE_UI_EVENT(EVENT_ITEM_ACTIVATED, ModelIndex)
    DECLARE_UI_EVENT(EVENT_ITEM_CONTEXT_MENU)

    //---

    /// base class for a widget that interacts with abstract model
    class BASE_UI_API AbstractItemView : public ScrollArea, public IAbstractItemModelObserver
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AbstractItemView, ScrollArea);

    public:
        AbstractItemView(bool allowItemSearch = false);
        virtual ~AbstractItemView();

        //--

        // get bound model
        INLINE IAbstractItemModel* model() const { return m_model.get(); }

        // is given item selected ?
        INLINE bool isSelected(const ModelIndex& item) const { return m_selection.contains(item); }

        // get current selection
        INLINE const base::HashSet<ModelIndex>& selection() const { return m_selection; }

        // get selection root - first item of selection
        INLINE const ModelIndex& selectionRoot() const { return m_selectionRoot; }

        // get current item (usually last item of selection, has the input focus)
        INLINE const ModelIndex& current() const { return m_current; }

        // get currently applied filter pattern
        INLINE const SearchPattern& filter() const { return m_filter; }

        //--

        /// bind model to view
        void model(const base::RefPtr<IAbstractItemModel>& model);

        //--

        /// set new selection for a set of items
        void select(const base::Array<ModelIndex>& items, ItemSelectionMode mode = ItemSelectionModeBit::Default, bool postEvent=true);

        /// set new selection
        void select(const ModelIndex& item, ItemSelectionMode mode = ItemSelectionModeBit::Default, bool postEvent = true);

        //--

        /// bind filter, changes the current display list
        void filter(const SearchPattern& filter);

        /// enable sorting by given column, set to INDEX_NONE to disable
        void sort(int colIndex = INDEX_NONE, bool asc = true);

        //--

    protected:
        virtual void modelReset() override;
        virtual void modelRowsAboutToBeAdded(const ModelIndex& parent, int first, int count) override;
        virtual void modelRowsAdded(const ModelIndex& parent, int first, int count) override;
        virtual void modelRowsAboutToBeRemoved(const ModelIndex& parent, int first, int count) override;
        virtual void modelRowsRemoved(const ModelIndex& parent, int first, int count) override;

        base::HashSet<ModelIndex> m_tempSelection;
        base::HashSet<ModelIndex> m_selection;
        ModelIndex m_selectionRoot;
        ModelIndex m_current;

        base::RefPtr<IAbstractItemModel> m_model;

        SearchPattern m_filter;

        int m_sortingColumnIndex = INDEX_NONE;
        bool m_sortingAsc = true;

        //--

        virtual void visualizeSelectionState(const ModelIndex& item, bool selected) = 0;
        virtual void visualizeCurrentState(const ModelIndex& item, bool selected) = 0;
        virtual void focusElement(const ModelIndex& item) = 0;
        virtual void rebuildSortedOrder() = 0;
        virtual void rebuildDisplayList() = 0;
    };

} // ui
