/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#include "build.h"
#include "uiAbstractItemModel.h"
#include "uiAbstractItemView.h"

namespace ui
{

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(AbstractItemView);
    RTTI_END_TYPE();

    AbstractItemView::AbstractItemView(bool allowItemSearch /*= false*/)
        : m_model(nullptr)
    {
        hitTest(HitTestState::Enabled);
        allowFocusFromKeyboard(true);
        layoutMode(LayoutMode::Vertical);
    }

    AbstractItemView::~AbstractItemView()
    {
        if (nullptr != m_model)
        {
            m_model->unregisterObserver(this);
            m_model = nullptr;
        }
    }

    void AbstractItemView::model(const base::RefPtr<IAbstractItemModel>& model)
    {
        if (m_model != model)
        {
            m_selection.reset();
            m_tempSelection.reset();
            m_selectionRoot = ModelIndex();
            m_current = ModelIndex();

            if (nullptr != m_model)
            {
                m_model->unregisterObserver(this);

                auto oldModel = m_model;
                m_model.reset();
                modelReset();
            }

            m_model = model;

            if (nullptr != m_model)
            {
                m_model->registerObserver(this);
                modelReset();
            }
        }
    }

    void AbstractItemView::filter(const SearchPattern& filter)
    {
        if (m_filter != filter)
        {
            m_filter = filter;
            rebuildDisplayList();
        }
    }

    void AbstractItemView::sort(int colIndex /*= INDEX_NONE*/, bool asc /*= true*/)
    {
        if (m_sortingColumnIndex != colIndex || m_sortingAsc != asc)
        {
            m_sortingColumnIndex = colIndex;
            m_sortingAsc = asc;
            rebuildSortedOrder();
            rebuildDisplayList();
        }
    }

    void AbstractItemView::select(const base::Array<ModelIndex>& items, ItemSelectionMode mode /*= ItemSelectionModeBit::Default*/, bool postEvent /*= true*/)
    {
        bool somethingChanged = false;

        // selection change
        if (mode.test(ItemSelectionModeBit::Clear))
        {
            // start with an empty set
            m_tempSelection.reset();

            // add objects that we want to directly select
            // O(N) in the number of items
            if (mode.test(ItemSelectionModeBit::Select))
            {
                for (const auto &item : items)
                    if (item)
                        m_tempSelection.insert(item);
            }

            // update visualization based on set difference
            // O(N log N) in the number of items
            for (const auto& it : m_tempSelection.keys())
            {
                if (!m_selection.contains(it))
                {
                    visualizeSelectionState(it, true);
                    somethingChanged = true;
                }
            }

            // remove visualization from deselected items - O(N log N) in the number of items
            for (const auto& it : m_selection.keys())
            {
                if (!m_tempSelection.contains(it))
                {
                    visualizeSelectionState(it, false);
                    somethingChanged = true;
                }
            }

            // set new selection
            std::swap(m_selection, m_tempSelection);
            m_tempSelection.reset();

            // selection root is the first item of the passed set
            m_selectionRoot = items.empty() ? ModelIndex() : items.front();
        }
        // select mode
        else if (mode.test(ItemSelectionModeBit::Select) && !mode.test(ItemSelectionModeBit::Deselect))
        {
            // items in the list, if not already selected will be selected
            for (const auto& item : items)
            {
                if (item && m_selection.insert(item))
                {
                    visualizeSelectionState(item, true);
                    somethingChanged = true;
                }
            }

            // if there was no selection root use the first item passed
            if (!m_selectionRoot && !items.empty())
                m_selectionRoot = items.front();
        }
        // deselect mode
        else if (mode.test(ItemSelectionModeBit::Deselect) && !mode.test(ItemSelectionModeBit::Select))
        {
            // items in the list, if not already selected will be selected
            for (const auto& item : items)
            {
                if (item && m_selection.remove(item))
                {
                    visualizeSelectionState(item, false);
                    somethingChanged = true;
                }
            }

            // if we deselected selection root than reset it
            if (!m_selection.contains(m_selectionRoot))
                m_selectionRoot = ModelIndex();
        }
        // toggle mode
        else if (mode.test(ItemSelectionModeBit::Toggle))
        {
            // items in the list, if not already selected will be selected
            for (const auto &item : items)
            {
                if (m_selection.remove(item))
                {
                    visualizeSelectionState(item, false);
                    somethingChanged = true;

                    if (m_selectionRoot == item)
                        m_selectionRoot = ModelIndex();
                }
                else if (m_selection.insert(item))
                {
                    visualizeSelectionState(item, true);
                    somethingChanged = true;

                    if (!m_selectionRoot)
                        m_selectionRoot = item;
                }
            }
        }

        // update current item
        if (mode.test(ItemSelectionModeBit::UpdateCurrent))
        {
            ModelIndex newCurrent;

            for (int i=items.lastValidIndex(); i >= 0; --i)
            {
                if (items[i])
                {
                    newCurrent = items[i];
                    break;
                }
            }

            if (newCurrent != m_current)
            {
                if (m_current)
                    visualizeCurrentState(m_current, false);

                m_current = newCurrent;

                if (m_current)
                    visualizeCurrentState(m_current, true);
            }
        }

        // focus inner widget if required
        if (m_current)
            focusElement(m_current);

        // notify interested parties
        if (somethingChanged && postEvent)
            call(EVENT_ITEM_SELECTION_CHANGED);
    }

    void AbstractItemView::select(const ModelIndex& item, ItemSelectionMode mode /*= ItemSelectionModeBit::Default*/, bool postEvent /*= true*/)
    {
        base::InplaceArray<ModelIndex, 1> items;
        if (item)
            items.pushBack(item);

        select(items, mode, postEvent);
    }

    //--

    void AbstractItemView::modelReset()
    {
        m_selection.reset();
        m_current = ModelIndex();
    }

    void AbstractItemView::modelRowsAboutToBeAdded(const ModelIndex& parent, int first, int count)
    {
        ModelIndexReindexerInsert reindexer(parent, first, count);

        // reindex the selection indices
        {
            m_tempSelection.reset();
            for (const auto &id : m_selection.keys())
            {
                ModelIndex newIndex = id;
                reindexer.reindex(id, newIndex);
                m_tempSelection.insert(newIndex);
            }

            std::swap(m_tempSelection, m_selection);
            m_tempSelection.reset();
        }

        // update the current
        if (m_current)
            reindexer.reindex(m_current, m_current);
    }

    void AbstractItemView::modelRowsAdded(const ModelIndex& parent, int first, int lacountst)
    {}

    void AbstractItemView::modelRowsAboutToBeRemoved(const ModelIndex& parent, int first, int count)
    {
        ModelIndexReindexerRemove reindexer(parent, first, count);

        // reindex the selection indices
        {
            m_tempSelection.reset();
            for (const auto &id : m_selection.keys())
            {
                ModelIndex newIndex = id;

                switch (reindexer.reindex(id, newIndex))
                {
                    case ReindexerResult::NotChanged:
                    case ReindexerResult::Changed:
                        m_tempSelection.insert(newIndex);
                }
            }

            std::swap(m_tempSelection, m_selection);
            m_tempSelection.reset();
        }

        // update the current
        if (m_current)
            reindexer.reindex(m_current, m_current);
    }

    void AbstractItemView::modelRowsRemoved(const ModelIndex& parent, int first, int count)
    {}

    //---

} // ui