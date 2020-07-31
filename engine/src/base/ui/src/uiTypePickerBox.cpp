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

namespace ui
{
    //---

    TypeListModel::TypeListModel()
    {
        RTTI::GetInstance().enumBaseTypes(m_allTypes);
    }

    base::Type TypeListModel::typeForIndex(uint32_t index) const
    {
        if (index < m_allTypes.size())
            return m_allTypes[index];
        return base::Type();
    }

    ModelIndex TypeListModel::findIndexForType(base::Type data) const
    {
        const auto index = m_allTypes.find(data);
        if (index != -1)
            return ModelIndex(this, index, 0);
        return ModelIndex();
    }

    uint32_t TypeListModel::size() const
    {
        return m_allTypes.size();
    }

    base::StringBuf TypeListModel::content(const ModelIndex& id, int colIndex /*= 0*/) const
    {
        base::StringBuilder txt;

        if (id.row() >= 0 && id.row() <= m_allTypes.lastValidIndex())
        {
            const auto typeInfo = m_allTypes[id.row()];

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
        }

        return txt.toString();
    }

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(TypePickerBox);
    RTTI_END_TYPE();

    //---

    TypePickerBox::TypePickerBox(base::Type initialType, bool allowNullType, base::StringView<char> caption)
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
        m_listModel = base::CreateSharedPtr<TypeListModel>();
        m_list->model(m_listModel);

        if (const auto id = m_listModel->findIndexForType(initialType))
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
            auto selected = m_listModel->typeForIndex(m_list->selectionRoot().row());

            if (!selected && !m_allowNullType)
                return;

            closeWithType(selected);
        }
    }

    void TypePickerBox::closeWithType(base::Type value)
    {
        call(EVENT_TYPE_SELECTED, value);
        requestClose();
    }

    //---

} // ui

