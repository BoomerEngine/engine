/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"
#include "uiCheckBox.h"
#include "uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

RTTI_BEGIN_TYPE_CLASS(CheckBox);
    RTTI_METADATA(ElementClassNameMetadata).name("CheckBox");
RTTI_END_TYPE();

CheckBox::CheckBox(bool initialState /*= false*/)
    : Button(ButtonModeBit::EventOnClick)
    , m_state(initialState ? CheckBoxState::Checked : CheckBoxState::Unchecked)
{
    allowFocusFromClick(true);
    allowFocusFromKeyboard(true);

    if (!IsDefaultObjectCreation())
        createInternalChild<TextLabel>();

    updateCheckStyles();
}

void CheckBox::state(bool flag)
{
    state(flag ? CheckBoxState::Checked : CheckBoxState::Unchecked);
}

void CheckBox::updateCheckStyles()
{
    switch (m_state)
    {
        case CheckBoxState::Checked:
        {
            removeStyleClass("undefined"_id);
            addStyleClass("checked"_id);
            break;
        }

        case CheckBoxState::Unchecked:
        {
            removeStyleClass("undefined"_id);
            removeStyleClass("checked"_id);
            break;
        }

        case CheckBoxState::Undecided:
        {
            addStyleClass("undefined"_id);
            removeStyleClass("checked"_id);
            break;
        }
    }
}

void CheckBox::state(CheckBoxState state)
{
    if (m_state != state)
    {
        m_state = state;
        updateCheckStyles();
    }   
}

CheckBoxState CheckBox::NextState(CheckBoxState state)
{
    return (state == CheckBoxState::Checked) ? CheckBoxState::Unchecked : CheckBoxState::Checked;
}

void CheckBox::clicked()
{
    state(NextState(state()));
    call(EVENT_CLICKED, stateBool());
}

//--

CheckBoxPtr MakeCheckbox(IElement* parent, StringView txt, bool initialState)
{
    auto container = parent->createChild();
    container->layoutHorizontal();

    auto box = container->createChild<CheckBox>(initialState);
    auto text = container->createChild<TextLabel>(txt);
    text->customHorizontalAligment(ElementHorizontalLayout::Expand);

    return box;
}

//--
    
END_BOOMER_NAMESPACE_EX(ui)
