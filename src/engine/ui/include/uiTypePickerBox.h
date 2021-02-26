/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#pragma once

#include "uiEventFunction.h"
#include "uiWindowPopup.h"
#include "uiSimpleListModel.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///----

DECLARE_UI_EVENT(EVENT_TYPE_SELECTED, Type)

///----

// data model for listing all engine types
class TypeListModel : public SimpleTypedListModel<Type>
{
public:
    TypeListModel();

private:
    virtual bool compare(const Type& a, const Type& b, int colIndex) const override final;
    virtual bool filter(const Type& data, const SearchPattern& filter, int colIndex = 0) const  override final;
    virtual StringBuf content(const Type& data, int colIndex = 0) const override final;
};

///----

// helper dialog that allows to select a type from type list
class TypePickerBox : public PopupWindow
{
    RTTI_DECLARE_VIRTUAL_CLASS(TypePickerBox, PopupWindow);

public:
    TypePickerBox(Type initialType, bool allowNullType, StringView caption="Select type");

    // generated OnTypeSelected when selected and general OnClosed when window itself is closed

private:
    ListView* m_list;
    RefPtr<TypeListModel> m_listModel;
    bool m_allowNullType;

    virtual bool handleKeyEvent(const input::KeyEvent& evt) override;
    void closeWithType(Type value);
    void closeIfValidTypeSelected();
};

///----

END_BOOMER_NAMESPACE_EX(ui)
