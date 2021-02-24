/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"
#include "uiSearchBar.h"
#include "uiEditBox.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiInputAction.h"
#include "uiItemView.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

RTTI_BEGIN_TYPE_CLASS(SearchPattern);
RTTI_PROPERTY(pattern);
RTTI_PROPERTY(regex);
RTTI_PROPERTY(wholeWordsOnly);
RTTI_PROPERTY(caseSenitive);
RTTI_END_TYPE();

bool SearchPattern::operator==(const SearchPattern& other) const
{
    return (pattern == other.pattern) && (regex == other.regex) && (wholeWordsOnly == other.wholeWordsOnly) && (caseSenitive == other.caseSenitive);
}

bool SearchPattern::operator!=(const SearchPattern& other) const
{
    return !operator==(other);
}

void SearchPattern::print(base::IFormatStream& f) const
{
    if (!pattern.empty())
    {
        f << "Pattern: '";
        f << pattern;
        f << "'";

        if (caseSenitive)
            f << " (Case)";
        if (wholeWordsOnly)
            f << " (Words)";
        if (regex)
            f << " (RegEx)";
    }
    else
    {
        f << "<empty>";
    }
}

bool SearchPattern::testString(base::StringView txt) const
{
    if (pattern.empty())
        return true;

    if (caseSenitive)
    {
        if (hasWildcards)
            return txt.matchPattern(pattern);
        else
            return txt.matchString(pattern);
    }
    else
    {
        if (hasWildcards)
            return txt.matchPatternNoCase(pattern);
        else
            return txt.matchStringNoCase(pattern);
    }
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(SearchBar);
    RTTI_METADATA(ElementClassNameMetadata).name("SearchBar");
RTTI_END_TYPE();

//--

SearchBar::SearchBar(bool extendedSearchParams)
    : m_timer(this)
{
    layoutHorizontal();
    allowFocusFromKeyboard(false);
    enableAutoExpand(true, false);

    {
        m_showHistory = createChild<ui::Button>("[img:search_glyph]");
        m_showHistory->customVerticalAligment(ElementVerticalLayout::Middle);
        m_showHistory->bind(EVENT_CLICKED) = [this]()
        {

        };
    }

    m_text = createChild<ui::EditBox>();
    m_text->customHorizontalAligment(ElementHorizontalLayout::Expand);
    m_text->customVerticalAligment(ElementVerticalLayout::Middle);
    m_text->bind(EVENT_TEXT_MODIFIED) = [this]()
    {
        m_timer.startOneShot(0.1f);
    };

    {
        m_flagCaseSensitive = createChild<ui::Button>("Aa", ButtonMode{ ButtonModeBit::EventOnClickRelease, ButtonModeBit::Toggle, ButtonModeBit::AutoToggle });
        m_flagCaseSensitive->customVerticalAligment(ElementVerticalLayout::Middle);
        m_flagCaseSensitive->bind(EVENT_CLICKED) = [this]()
        {
            updateSearchPattern();
        };
    }

    if (extendedSearchParams)
    {
        {
            m_flagWholeWords = createChild<ui::Button>("Word", ButtonMode{ ButtonModeBit::EventOnClickRelease, ButtonModeBit::Toggle, ButtonModeBit::AutoToggle });
            m_flagWholeWords->customVerticalAligment(ElementVerticalLayout::Middle);
            m_flagWholeWords->bind(EVENT_CLICKED) = [this]()
            {

            };
        }

        {
            m_flagRegEx = createChild<ui::Button>("RegEx", ButtonMode{ ButtonModeBit::EventOnClickRelease, ButtonModeBit::Toggle, ButtonModeBit::AutoToggle });
            m_flagRegEx->customVerticalAligment(ElementVerticalLayout::Middle);
            m_flagRegEx->bind(EVENT_CLICKED) = [this]()
            {

            };
        }
    }

    m_timer = [this]()
    {
        updateSearchPattern();
    };
}

SearchBar::~SearchBar()
{
    if (auto view = m_itemView.lock())
        view->bindInputPropagationElement(nullptr);
}

IElement* SearchBar::focusFindFirst()
{
    return m_text;
}

void SearchBar::bindItemView(ItemView* ptr)
{
    if (auto view = m_itemView.lock())
        view->bindInputPropagationElement(nullptr);

    m_itemView = ptr;

    if (ptr)
        ptr->bindInputPropagationElement(this);

}

void SearchBar::loadHistory(const base::Array<SearchPattern>& history, bool setCurrent /*= true*/)
{
    if (!history.empty() && setCurrent)
        m_currentSearchPattern = history.back();
}

void SearchBar::saveHistory(base::Array<SearchPattern>& outHistory) const
{
    outHistory.clear();
    outHistory = m_history;
    outHistory.pushBack(m_currentSearchPattern);
}

void SearchBar::searchPattern(const SearchPattern& pattern)
{
    m_currentSearchPattern = pattern;
    //updateSearchPattern();
}

static bool IsWidcardPattern(base::StringView txt)
{
    for (const auto ch : txt)
        if (ch == '*' || ch == '?')
            return true;

    return false;
}

void SearchBar::updateSearchPattern()
{
    m_currentSearchPattern.pattern = m_text->text();
    m_currentSearchPattern.hasWildcards = IsWidcardPattern(m_currentSearchPattern.pattern);

    m_currentSearchPattern.caseSenitive = m_flagCaseSensitive->toggled();
    m_currentSearchPattern.regex = m_flagRegEx ? m_flagRegEx->toggled() : false;
    m_currentSearchPattern.wholeWordsOnly = m_flagWholeWords ? m_flagWholeWords->toggled() : false;

    if (auto view = m_itemView.lock())
        view->filter(m_currentSearchPattern);

    call(EVENT_SEARCH_CHANGED);
}

bool SearchBar::handleExternalKeyEvent(const base::input::KeyEvent& evt)
{
    if (m_blockExternalKeyPropagation)
    {
        m_externalEventReceived = true;
        return false;
    }

    if (evt.keyCode() == base::input::KeyCode::KEY_BACK || evt.keyCode() == base::input::KeyCode::KEY_DELETE || evt.keyCode() == base::input::KeyCode::KEY_TAB)
    {
        if (!m_text->isFocused())
        {
            m_text->focus();

            auto* elem = static_cast<IElement*>(m_text);
            return elem->handleKeyEvent(evt);
        }
    }
        
    return false;
}

static bool IsGoodCharToStartTyping(wchar_t ch)
{
    if (ch <= ' ')
        return false;

    return true;
}

bool SearchBar::handleExternalCharEvent(const base::input::CharEvent& evt)
{
    if (IsGoodCharToStartTyping(evt.scanCode()))
    {
        if (m_blockExternalKeyPropagation)
        {
            m_externalEventReceived = true;
            return false;
        }

        if (!m_text->isFocused())
        {
            m_text->focus();

            auto* elem = static_cast<IElement*>(m_text);
            return elem->handleCharEvent(evt);
        }
    }

    return false;
}

bool SearchBar::handleKeyEvent(const base::input::KeyEvent& evt)
{
    if (evt.deviceType() == base::input::DeviceType::Keyboard)
    {
        if (auto view = m_itemView.lock())
        {
            m_externalEventReceived = false;
            m_blockExternalKeyPropagation = true;

            auto* elem = static_cast<IElement*>(view);
            auto keyEventHandled = elem->handleKeyEvent(evt);

            m_blockExternalKeyPropagation = false;

            if (!m_externalEventReceived || keyEventHandled)
            {
                view->focus();
                return true;
            }
        }
    }

    return false;
}

//--

END_BOOMER_NAMESPACE(ui)