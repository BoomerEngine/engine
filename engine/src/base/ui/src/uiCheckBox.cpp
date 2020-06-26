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

namespace ui
{

    //--

    RTTI_BEGIN_TYPE_CLASS(CheckBox);
        RTTI_METADATA(ElementClassNameMetadata).name("CheckBox");
    RTTI_END_TYPE();

    CheckBox::CheckBox()
        : Button(ButtonModeBit::EventOnClick)
        , m_state(CheckBoxState::Unchecked)
    {
        if (!base::IsDefaultObjectCreation())
            createInternalChild<ui::TextLabel>();
    }

    void CheckBox::state(bool flag)
    {
        state(flag ? CheckBoxState::Checked : CheckBoxState::Unchecked);
    }

    void CheckBox::state(CheckBoxState state)
    {
        if (m_state != state)
        {
            m_state = state;

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
    }

    bool CheckBox::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        if (name == "checked" || name == "check")
        {
            if (value == "true" || value == "1")
            {
                state(CheckBoxState::Checked);
                return true;
            }
            else if (value == "false" || value == "0")
            {
                state(CheckBoxState::Unchecked);
                return true;
            }
            else if (value == "?")
            {
                state(CheckBoxState::Undecided);
                return true;
            }
        }

        return TBaseClass::handleTemplateProperty(name, value);
    }

    CheckBoxState CheckBox::NextState(CheckBoxState state)
    {
        return (state == CheckBoxState::Checked) ? CheckBoxState::Unchecked : CheckBoxState::Checked;
    }

    void CheckBox::clicked()
    {
        state(NextState(state()));
        TBaseClass::clicked();
    }

    //--
    
} // ui
