/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#include "build.h"
#include "uiTypePickerBox.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiWindow.h"
#include "uiImage.h"
#include "uiRenderer.h"
#include "uiListView.h"

#include "base/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE(ui)

//---

TypeListModel::TypeListModel()
{
    base::Array<base::Type> allTypes;
    RTTI::GetInstance().enumBaseTypes(allTypes);

    for (const auto& type : allTypes)
        add(type);
}

bool TypeListModel::compare(const base::Type& a, const base::Type& b, int colIndex) const
{
    return a.name().view() < b.name().view();
}

bool TypeListModel::filter(const base::Type& data, const SearchPattern& filter, int colIndex) const
{
    return filter.testString(data.name().view());
}

base::StringBuf TypeListModel::content(const base::Type& typeInfo, int colIndex) const
{
    base::StringBuilder txt;

    switch (typeInfo.metaType())
    {
    case base::rtti::MetaType::Enum: txt << "[img:vs2012/object_enum] "; break;
        case base::rtti::MetaType::Bitfield: txt << "[img:vs2012/object_bitfield] "; break;
        case base::rtti::MetaType::Class: txt << "[img:vs2012/object_class] "; break;
        //case base::rtti::MetaType::ClassRef: txt << "[img:vs2012/object_class_ref] ";
        case base::rtti::MetaType::Simple: txt << "[img:vs2012/object_simple_type] "; break;
    }

    txt << typeInfo->name();

    txt << "  [i][color:#888]";
    switch (typeInfo.metaType())
    {
        case base::rtti::MetaType::Enum: txt << "(Enum)"; break;
        case base::rtti::MetaType::Bitfield: txt << "(Bitfield)"; break;
        case base::rtti::MetaType::Class: txt << "(Class)"; break;
        case base::rtti::MetaType::Simple: txt << "(Simple)"; break;
    }

    return txt.toString();
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(TypePickerBox);
RTTI_END_TYPE();

//---

TypePickerBox::TypePickerBox(base::Type initialType, bool allowNullType, base::StringView caption)
    : PopupWindow(ui::WindowFeatureFlagBit::DEFAULT_POPUP_DIALOG, caption)
    , m_allowNullType(allowNullType)
{
    // list of types
    m_list = createChild<ui::ListView>();
    m_list->customInitialSize(500, 400);
    m_list->expand();

    // buttons
    {
        auto buttons = createChild();
        buttons->customPadding(5);
        buttons->layoutHorizontal();

        {
            auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:accept] Select");
            button->addStyleClass("green"_id);
            button->bind(EVENT_CLICKED) = [this]() { closeIfValidTypeSelected(); };
        }

        {
            auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Cancel");
            button->bind(EVENT_CLICKED) = [this]() { requestClose(); };
        }
    }

    // attach data model
    m_listModel = base::RefNew<TypeListModel>();
    m_list->model(m_listModel);

    if (const auto id = m_listModel->index(initialType))
    {
        m_list->select(id);
        m_list->ensureVisible(id);
    }

    m_list->bind(EVENT_ITEM_ACTIVATED) = [this]()
    {
        closeIfValidTypeSelected();
    };
}

bool TypePickerBox::handleKeyEvent(const base::input::KeyEvent & evt)
{
    if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_ESCAPE)
    {
        requestClose();
        return true;
    }
    else if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_RETURN)
    {
        closeIfValidTypeSelected();
        return true;
    }

    return TBaseClass::handleKeyEvent(evt);
}

void TypePickerBox::closeIfValidTypeSelected()
{
    if (m_list)
    {
        auto selected = m_listModel->data(m_list->selectionRoot());
        if (selected || m_allowNullType)
            closeWithType(selected);
    }
}

void TypePickerBox::closeWithType(base::Type value)
{
    call(EVENT_TYPE_SELECTED, value);
    requestClose();
}

//---

END_BOOMER_NAMESPACE(ui)

