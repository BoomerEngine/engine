/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"
#include "uiDynamicChoiceBox.h"
#include "uiScrollArea.h"
#include "uiWindowPopup.h"
#include "uiTextLabel.h"
#include "uiButton.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

class DynamicChoiceBoxOptions : public ScrollArea
{
    RTTI_DECLARE_VIRTUAL_CLASS(DynamicChoiceBoxOptions, IElement);

public:
    DynamicChoiceBoxOptions()
        : ScrollArea(ScrollMode::Auto)
    {
        layoutMode(LayoutMode::Vertical);
    }

    void addOption(const DynamicChoiceList::Element& elem, int index, bool selected)
    {
        auto item = createNamedChild<Button>("ComboBoxListItem"_id, ButtonModeBit::EventOnClick);
        item->customHorizontalAligment(ElementHorizontalLayout::Expand);
        item->customVerticalAligment(ElementVerticalLayout::Expand);

        auto text = item->createChild<TextLabel>(elem.text);
        text->customHorizontalAligment(ElementHorizontalLayout::Expand);
        text->customVerticalAligment(ElementVerticalLayout::Expand);

        if (selected)
            item->addStyleClass("selected"_id);

        auto selfRef = RefWeakPtr<DynamicChoiceBoxOptions>(this);

        DynamicChoiceOption selectedOption;
        selectedOption.index = index;
        selectedOption.text = elem.text;
        selectedOption.value = elem.value;

        item->bind(EVENT_CLICKED) = [selfRef, selectedOption]()
        {
            if (auto options = selfRef.lock())
                options->call(EVENT_DYNAMIC_CHOICE_SELECTED, selectedOption);
        };
    }

private:
    int m_index;
};

RTTI_BEGIN_TYPE_CLASS(DynamicChoiceBoxOptions);
    RTTI_METADATA(ElementClassNameMetadata).name("ComboBoxOptions");
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(DynamicChoiceList);
RTTI_END_TYPE();

DynamicChoiceList::DynamicChoiceList()
{}

int DynamicChoiceList::add(StringView text, StringView icon /*= ""*/, Variant value /*= Variant()*/)
{
    auto& elem = m_elements.emplaceBack();
    elem.text = StringBuf(text);
    elem.icon = StringBuf(icon);
    elem.value = std::move(value);
    return m_elements.lastValidIndex();
}

void DynamicChoiceList::select(int index)
{
    m_selected = index;
}

//--

RTTI_BEGIN_TYPE_CLASS(DynamicChoiceOption);
    RTTI_PROPERTY(text);
    RTTI_PROPERTY(index);
    RTTI_PROPERTY(value);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(DynamicChoiceBox);
    RTTI_METADATA(ElementClassNameMetadata).name("ComboBox");
RTTI_END_TYPE();
     
DynamicChoiceBox::DynamicChoiceBox()
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

DynamicChoiceBox::~DynamicChoiceBox()
{
    closePopupList();
}

void DynamicChoiceBox::closePopupList()
{
    if (m_popup)
    {
        m_popup->requestClose();
        m_popup.reset();
    }
}

void DynamicChoiceBox::text(StringView text)
{
    m_text->text(text);
}

StringBuf DynamicChoiceBox::text() const
{
    return m_text->text();
}

bool DynamicChoiceBox::handleKeyEvent(const input::KeyEvent& evt)
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
            /*if (m_selectedOption > 0)
                selectOption(m_selectedOption - 1);*/
            return true;
        }
        else if (evt.keyCode() == input::KeyCode::KEY_RIGHT)
        {
            /*if (m_selectedOption < m_options.lastValidIndex())
                selectOption(m_selectedOption + 1);*/
            return true;
        }
    }

    return TBaseClass::handleKeyEvent(evt);
}

void DynamicChoiceBox::handleEnableStateChange(bool isEnabled)
{
    m_area->enable(isEnabled);
}

#pragma optimize("" , off)
void DynamicChoiceBox::showPopupList()
{
    if (!isEnabled())
        return;

    closePopupList();

    auto optionList = RefNew<DynamicChoiceList>();
    call(EVENT_DYNAMIC_CHOICE_QUERY, optionList);

    if (optionList->elements().empty())
        return;

    auto options = RefNew<DynamicChoiceBoxOptions>();
    for (auto i : optionList->elements().indexRange())
    {
        const auto selected = (i == optionList->selected());
        options->addOption(optionList->elements()[i], i, selected);
    }

    auto selfRef = RefWeakPtr<DynamicChoiceBox>(this);
    options->bind(EVENT_DYNAMIC_CHOICE_SELECTED) = [selfRef](DynamicChoiceOption option)
    {
        if (auto box = selfRef.lock())
        {
            box->text(option.text);
            box->closePopupList();

            box->runSync<DynamicChoiceBox>([option](DynamicChoiceBox& box) // allow the window to close
                {
                    box.call(EVENT_DYNAMIC_CHOICE_SELECTED, option);
                });
        }
    };

    m_popup = RefNew<PopupWindow>();
    m_popup->attachChild(options);

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
