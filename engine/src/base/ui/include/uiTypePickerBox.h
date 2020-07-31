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
    class TypeListModel : public SimpleListModel
    {
    public:
        TypeListModel();

        base::Type typeForIndex(uint32_t index) const;
        ModelIndex findIndexForType(base::Type) const;

    private:
        base::Array<base::Type> m_allTypes;

        virtual uint32_t size() const override final;
        virtual base::StringBuf content(const ModelIndex& id, int colIndex = 0) const override final;
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