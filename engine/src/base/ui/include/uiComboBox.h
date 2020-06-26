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
    class Button;
    class TextLabel;
    
    /// simple text based combo box
    class BASE_UI_API ComboBox : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ComboBox, IElement);

    public:
        ComboBox();
        virtual ~ComboBox();

        //---

        // clear all options
        void clearOptions();

        // add option
        void addOption(const base::StringBuf& txt);

        // select option
        void selectOption(int option);

        // select option by text
        void selectOption(const base::StringBuf& text);

        // get selected option
        int selectedOption() const;

        // get text for selected option
        base::StringBuf selectedOptionText() const;

        // get number of options
        int numOptions() const;

        //--

        // hide the popup if displayed
        void closePopupList();

        // show popup window
        void showPopupList();

    private:
        ButtonPtr m_area;
        TextLabelPtr m_text;
        PopupPtr m_popup;

        base::Array<base::StringBuf> m_options;
        int m_selectedOption;

        virtual void handleEnableStateChange(bool isEnabled) override;
        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
    };

} // ui