/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#pragma once

#include "uiElement.h"

namespace ui
{
    ///---

    /// helper widget that provides a search bar, usually integrated with a ItemView
    /// NOTE: the search list can be automatically loaded/stored in config
    class BASE_UI_API SearchBar : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SearchBar, IElement);

    public:
        SearchBar(bool extendedSearchParams = false);
        virtual ~SearchBar();

        //--

        ElementEventProxy OnSearchPatternChanged;

        //--

        // get current search pattern
        const SearchPattern& searchPattern() const { return m_currentSearchPattern; }

        //--

        // load search history
        void loadHistory(const base::Array<SearchPattern>& history, bool setCurrent = true);

        // store search history, current pattern is stored at the top (might be empty)
        void saveHistory(base::Array<SearchPattern>& outHistory) const;

        // change search pattern
        void searchPattern(const SearchPattern& pattern);

        //--

        // attach to item view (to pass keyboard events and filter settings)
        void bindItemView(ItemView* ptr);

        //--

    private:
        TextEditorPtr m_text;
        ButtonPtr m_flagWholeWords;
        ButtonPtr m_flagCaseSensitive;
        ButtonPtr m_flagRegEx;
        ButtonPtr m_showHistory;
        Timer m_timer;

        //--

        base::Array<SearchPattern> m_history;
        SearchPattern m_currentSearchPattern;

        //--

        base::RefWeakPtr<ItemView> m_itemView;

        //--

        void updateSearchPattern();

        //--

        bool m_blockExternalKeyPropagation = false;
        bool m_externalEventReceived = false;

        virtual bool handleExternalKeyEvent(const base::input::KeyEvent& evt) override;
        virtual bool handleExternalCharEvent(const base::input::CharEvent& evt) override;
        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
    };

    ///---

} // ui