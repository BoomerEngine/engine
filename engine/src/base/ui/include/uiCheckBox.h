/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#pragma once

#include "uiButton.h"

namespace ui
{
    class Button;

    /// check box state
    enum class CheckBoxState : char
    {
        Undecided=-1,
        Unchecked=0,
        Checked=1,
    };

    /// a check box - 3 state check mark
    class BASE_UI_API CheckBox : public Button
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CheckBox, Button);

    public:
        CheckBox(bool initialState=false);

        //---

        INLINE CheckBoxState state() const { return m_state; }
        INLINE bool stateBool() const { return m_state == CheckBoxState::Checked; }

        void state(CheckBoxState state);
        void state(bool state);

        //--

    private:
        CheckBoxState m_state;

        virtual bool handleTemplateProperty(base::StringView name, base::StringView value) override;

        static CheckBoxState NextState(CheckBoxState state);

        virtual void clicked();
    };

    // make a checkbox with a label
    extern BASE_UI_API CheckBoxPtr MakeCheckbox(IElement* parent, base::StringView txt, bool initialState = false);

} // ui