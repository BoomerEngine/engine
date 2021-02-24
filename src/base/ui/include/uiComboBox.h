/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE(ui)

///---

DECLARE_UI_EVENT(EVENT_COMBO_SELECTED, int) // value in the combo box was selected (data: int)

///---
    
/// VERY simple text based combo box
class BASE_UI_API ComboBox : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ComboBox, IElement);

public:
    ComboBox();
    virtual ~ComboBox();

    //---

    int numOptions() const;

    void clearOptions();
    void addOption(const base::StringBuf& txt);

    bool removeOption(int option);
    bool removeOption(base::StringView txt);

    void selectOption(int option);
    void selectOption(base::StringView text);

    int selectedOption() const;
    base::StringBuf selectedOptionText() const;


    //--

    void closePopupList();
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

///--

END_BOOMER_NAMESPACE(ui)