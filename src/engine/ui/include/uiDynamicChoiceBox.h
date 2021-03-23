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

struct DynamicChoiceOption
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(DynamicChoiceOption);

public:
    StringBuf text;
    int index = -1;
    Variant value;
};

class DynamicChoiceList : public IReferencable
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(DynamicChoiceList);

public:
    DynamicChoiceList();

    struct Element
    {
        StringBuf text;
        StringBuf icon;
        Variant value;
    };

    INLINE int selected() const { return m_selected; }

    INLINE const Array<Element>& elements() const { return m_elements; }

    int add(StringView text, StringView icon="", Variant value = Variant());

    void select(int index);

protected:
    Array<Element> m_elements;
    int m_selected = -1;
};

//--

DECLARE_UI_EVENT(EVENT_DYNAMIC_CHOICE_SELECTED, DynamicChoiceOption) // value in the combo box was selected (data: int)
DECLARE_UI_EVENT(EVENT_DYNAMIC_CHOICE_QUERY, RefPtr<DynamicChoiceList>) // fill the list of choices

///--

/// a dynamic combo box-style control that gets the list of choices every time it's clicked
class ENGINE_UI_API DynamicChoiceBox : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(DynamicChoiceBox, IElement);

public:
    DynamicChoiceBox();
    virtual ~DynamicChoiceBox();

    //---

    void text(StringView text); // set display text

    StringBuf text() const; // note: explicit text set, can be different than any option

    //--

    void closePopupList();
    void showPopupList();

private:
    ButtonPtr m_area;
    TextLabelPtr m_text;
    PopupPtr m_popup;

    virtual void handleEnableStateChange(bool isEnabled) override;
    virtual bool handleKeyEvent(const InputKeyEvent& evt) override;
};

///---

END_BOOMER_NAMESPACE_EX(ui)
