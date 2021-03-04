/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"
#include "uiComboBox.h"
#include "uiScrollArea.h"
#include "uiWindowPopup.h"
#include "uiTextLabel.h"
#include "uiButton.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

class ComboBoxOptions : public ScrollArea
{
    RTTI_DECLARE_VIRTUAL_CLASS(ComboBoxOptions, IElement);

public:
    ComboBoxOptions()
        : ScrollArea(ScrollMode::Auto)
    {
        layoutMode(LayoutMode::Vertical);
    }

    void addOption(const StringBuf& txt, int index, bool selected)
    {
        auto item = createNamedChild<Button>("ComboBoxListItem"_id, ButtonModeBit::EventOnClick);
        item->customHorizontalAligment(ElementHorizontalLayout::Expand);
        item->customVerticalAligment(ElementVerticalLayout::Expand);

        auto text = item->createChild<TextLabel>(txt);
        text->customHorizontalAligment(ElementHorizontalLayout::Expand);
        text->customVerticalAligment(ElementVerticalLayout::Expand);

        if (selected)
            item->addStyleClass("selected"_id);

        auto selfRef = RefWeakPtr<ComboBoxOptions>(this);

        item->bind(EVENT_CLICKED) = [selfRef, index]()
        {
            if (auto options = selfRef.lock())
                options->call(EVENT_COMBO_SELECTED, index);
        };
    }

private:
    int m_index;
};

RTTI_BEGIN_TYPE_CLASS(ComboBoxOptions);
    RTTI_METADATA(ElementClassNameMetadata).name("ComboBoxOptions");
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(ComboBox);
    RTTI_METADATA(ElementClassNameMetadata).name("ComboBox");
RTTI_END_TYPE();
     
ComboBox::ComboBox()
    : m_selectedOption(-1)
{
    m_area = createChild<Button>();
        
    m_text = m_area->createNamedChild<TextLabel>("ComboBoxCaption"_id);
    m_area->createNamedChild<TextLabel>("ComboBoxIcon"_id);

    m_area->bind(EVENT_CLICKED, this) = [this]()
    {
        if (m_popup)
            closePopupList();
        else
            showPopupList();
    };
}

ComboBox::~ComboBox()
{
    closePopupList();
}

void ComboBox::closePopupList()
{
    if (m_popup)
    {
        m_popup->requestClose();
        m_popup.reset();
    }
}

void ComboBox::clearOptions()
{
    m_options.clear();
    m_selectedOption = -1;
    m_text->text(StringBuf::EMPTY());
}

int ComboBox::addOption(const StringBuf& txt)
{
    m_options.pushBack(txt);
    return m_options.lastValidIndex();
}

void ComboBox::selectOption(int option, bool postEvent)
{
    m_selectedOption = std::clamp(option, -1, (int)m_options.size() - 1);
    if (m_selectedOption >= 0 && m_selectedOption < (int)m_options.size())
        m_text->text(m_options[m_selectedOption]);
    else
        m_text->text("");

    if (postEvent)
        call(EVENT_COMBO_SELECTED, m_selectedOption);
}

void ComboBox::selectOption(StringView text, bool postEvent)
{
    auto index = m_options.find(text);
    if (index != INDEX_NONE)
    {
        selectOption(index, false);
    }
    else
    {
        m_selectedOption = INDEX_NONE;
        m_text->text(text);
    }

    if (postEvent)
        call(EVENT_COMBO_SELECTED, m_selectedOption);
}

bool ComboBox::removeOption(int option)
{
    if (option >= 0 && option <= m_options.lastValidIndex())
    {
        m_options.erase(option);

        if (m_selectedOption == option)
        {
            m_text->text(StringBuf::EMPTY());
            m_selectedOption = -1;
        }

        return true;
    }

    return false;
}

bool ComboBox::removeOption(StringView text)
{
    auto index = m_options.find(text);
    if (index != INDEX_NONE)
        return removeOption(index);

    return false;
}

int ComboBox::selectedOption() const
{
    return m_selectedOption;
}

StringBuf ComboBox::text() const
{
    return m_text->text();
}

int ComboBox::numOptions() const
{
    return (int)m_options.size();
}

bool ComboBox::handleKeyEvent(const input::KeyEvent& evt)
{
    if (evt.pressed())
    {
        if (evt.keyCode() == input::KeyCode::KEY_ESCAPE)
        {
            closePopupList();
            return true;
        }
        else if (evt.keyCode() == input::KeyCode::KEY_RETURN)
        {
            if (!m_popup)
                showPopupList();
            return true;
        }
        else if (evt.keyCode() == input::KeyCode::KEY_LEFT)
        {
            if (m_selectedOption > 0)
                selectOption(m_selectedOption - 1);
            return true;
        }
        else if (evt.keyCode() == input::KeyCode::KEY_RIGHT)
        {
            if (m_selectedOption < m_options.lastValidIndex())
                selectOption(m_selectedOption + 1);
            return true;
        }
    }

    return TBaseClass::handleKeyEvent(evt);
}

void ComboBox::handleEnableStateChange(bool isEnabled)
{
    m_area->enable(isEnabled);
}

void ComboBox::showPopupList()
{
    if (!isEnabled() || m_options.empty())
        return;

    closePopupList();

    m_popup = RefNew<PopupWindow>();

    auto options = RefNew<ComboBoxOptions>();

    m_popup->attachChild(options);

    for (uint32_t i=0; i<m_options.size(); ++i)
    {
        const auto& option = m_options[i];
        bool selected = (i == m_selectedOption);
        options->addOption(option, i, selected);
    }

    auto selfRef = RefWeakPtr<ComboBox>(this);

    options->bind(EVENT_COMBO_SELECTED) = [selfRef](int index)
    {
        if (auto box = selfRef.lock())
        {
            box->selectOption(index);
            box->closePopupList();
            box->call(EVENT_COMBO_SELECTED, index);
        }
    };

    m_popup->bind(EVENT_WINDOW_CLOSED) = [selfRef]()
    {
        if (auto box = selfRef.lock())
            box->m_popup.reset();
    };

    auto totalSize = cachedLayoutParams().calcTotalAreaFromDrawArea(cachedDrawArea()).size();

    m_popup->customMinSize(Size(0.0f, totalSize.y * 1.0f));
    m_popup->customStyle<float>("min-absolute-width"_id, totalSize.x);
    m_popup->customStyle<float>("max-absolute-width"_id, totalSize.x);
    m_popup->show(this, PopupWindowSetup().bottomLeft());
}

//--

END_BOOMER_NAMESPACE_EX(ui)
