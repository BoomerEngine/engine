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

namespace ui
{
    ///----

    DECLARE_UI_EVENT(EVENT_TYPE_SELECTED, base::Type)

    ///----

    // data model for listing all engine types
    class TypeListModel : public SimpleTypedListModel<base::Type>
    {
    public:
        TypeListModel();

    private:
        virtual bool compare(const base::Type& a, const base::Type& b, int colIndex) const override final;
        virtual bool filter(const base::Type& data, const SearchPattern& filter, int colIndex = 0) const  override final;
        virtual base::StringBuf content(const base::Type& data, int colIndex = 0) const override final;
    };

    ///----

    // helper dialog that allows to select a type from type list
    class TypePickerBox : public PopupWindow
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TypePickerBox, PopupWindow);

    public:
        TypePickerBox(base::Type initialType, bool allowNullType, base::StringView<char> caption="Select type");

        // generated OnTypeSelected when selected and general OnClosed when window itself is closed

    private:
        ListView* m_list;
        base::RefPtr<TypeListModel> m_listModel;
        bool m_allowNullType;

        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
        void closeWithType(base::Type value);
        void closeIfValidTypeSelected();
    };

    ///----

} // ui