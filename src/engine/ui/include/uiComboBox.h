/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///---

DECLARE_UI_EVENT(EVENT_COMBO_SELECTED, int) // value in the combo box was selected (data: int)

///---
    
/// VERY simple text based combo box
class ENGINE_UI_API ComboBox : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ComboBox, IElement);

public:
    ComboBox();
    virtual ~ComboBox();

    //---

    int numOptions() const;

    void clearOptions();
    int addOption(const StringBuf& txt);

    bool removeOption(int option);
    bool removeOption(StringView txt);

    void selectOption(int option, bool postEvent=false);
    void selectOption(StringView text, bool postEvent = false);
    int selectedOption() const; // always index of valid option or -1 if no valid option is selected (a different text may still be present)

    StringBuf text() const; // note: explicit text set, can be different than any option

    //--

    void closePopupList();
    void showPopupList();

private:
    ButtonPtr m_area;
    TextLabelPtr m_text;
    PopupPtr m_popup;

    Array<StringBuf> m_options;
    int m_selectedOption;

    virtual void handleEnableStateChange(bool isEnabled) override;
    virtual bool handleKeyEvent(const input::KeyEvent& evt) override;
};

///--

END_BOOMER_NAMESPACE_EX(ui)
