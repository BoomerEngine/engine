/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"
#include "uiSearchWidget.h"
#include "uiEditBox.h"

#include "uiTextLabel.h"
#include "uiInputAction.h"

namespace ui
{

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SearchWidget);
        RTTI_METADATA(ElementClassNameMetadata).name("SearchWidget");
    RTTI_END_TYPE();

    //--

    SearchWidget::SearchWidget(const base::StringBuf& initialText/* = base::StringBuf::EMPTY()*/)
        : m_timer(this)
    {
        layoutHorizontal();
        hitTest(HitTestState::Enabled);
        layoutMode(LayoutMode::Horizontal);

        actions().bindCommand("CloseSearch"_id) = [](SearchWidget* widget)
        {
            widget->cancelSearch();
        };

        m_text->bind("OnTextModified"_id, this) = [](SearchWidget* widget)
        {
            widget->m_timer.startOneShot(0.1f);
        };

        m_timer = [](SearchWidget* widget)
        {
            const auto text = widget->m_text->text();
            widget->call("OnSearchUpdated"_id, text);
        };
    }

    base::StringBuf SearchWidget::searchPattern() const
    {
        return m_text->text();
    }

    /*void SearchWidget::handleExternalCharEvent(const base::input::CharEvent &evt)
    {
        // show on first char
        auto shouldShow = (visibility() == VisibilityState::Hidden);
        if (shouldShow)
        {
            m_text->text(base::StringBuf::EMPTY());
            visibility(true);
        }

        m_text->focus();
        m_text->handleExternalCharEvent(evt);
    }*/

    bool SearchWidget::previewKeyEvent(const base::input::KeyEvent &evt)
    {
        // cancel the search box on escape
        if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_ESCAPE)
        {
            cancelSearch();
            return true;
        }

        return TBaseClass::previewKeyEvent(evt);
    }

    void SearchWidget::cancelSearch()
    {
        m_timer.stop();
        m_text->text(base::StringBuf::EMPTY());

        visibility(false);

        call("OnSearchUpdated"_id, base::StringBuf::EMPTY());
    }

    //--

} // ui