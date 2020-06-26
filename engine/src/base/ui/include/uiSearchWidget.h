/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiElement.h"

namespace ui
{

    ///---

    /// helper widget for searching stuff within a control
    class BASE_UI_API SearchWidget : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SearchWidget, IElement);

    public:
        SearchWidget(const base::StringBuf& initialText = base::StringBuf::EMPTY());

        //--

        // get the search pattern
        base::StringBuf searchPattern() const;

        //--


    private:
        EditBoxPtr m_text;
        Timer m_timer;

        //--

        virtual bool previewKeyEvent(const base::input::KeyEvent &evt) override final;

        //--

        void cancelSearch();
    };

    ///---

} // ui