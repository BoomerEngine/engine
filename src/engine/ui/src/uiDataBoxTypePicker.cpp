/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDataBox.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiTypePickerBox.h"
#include "core/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// general type picker
class DataBoxTypePicker : public IDataBox
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxTypePicker, IDataBox);

public:
    DataBoxTypePicker()
    {
        layoutHorizontal();

        m_caption = createChild<TextLabel>();
        m_caption->customHorizontalAligment(ElementHorizontalLayout::Expand);
        m_caption->customVerticalAligment(ElementVerticalLayout::Middle);

        m_button = createChildWithType<Button>("DataPropertyButton"_id);
        m_button->customVerticalAligment(ElementVerticalLayout::Middle);
        m_button->createChild<TextLabel>("[img:selection_list]");
        m_button->tooltip("Select from type list");
        m_button->bind(EVENT_CLICKED) = [this]() { showTypePicker(); };
    }

    virtual void handleValueChange() override
    {
        Type data;

        const auto ret = readValue(data);
        if (ret.code == DataViewResultCode::OK)
        {
            StringBuilder txt;
            txt << data;
            m_caption->text(txt.toString());
            m_button->visibility(!readOnly());
        }
        else if (ret.code == DataViewResultCode::ErrorManyValues)
        {
            m_caption->text("<many values>");
            m_button->visibility(!readOnly());
        }
        else
        {
            m_caption->text(TempString("[tag:#F00][img:error] {}[/tag]", ret));
            m_button->visibility(false);
        }
    }

    void showTypePicker()
    {
        if (!m_picker)
        {
            Type data;
            const auto ret = readValue(data);
            if (ret.code == DataViewResultCode::OK || ret.code == DataViewResultCode::ErrorManyValues)
            {
                bool allowNullType = false;

                m_picker = RefNew<TypePickerBox>(data, allowNullType);
                m_picker->show(this, PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));

                auto selfRef = RefWeakPtr<DataBoxTypePicker>(this);

                m_picker->bind(EVENT_WINDOW_CLOSED) = [selfRef]()
                {
                    if (auto box = selfRef.lock())
                        box->m_picker.reset();
                };

                m_picker->bind(EVENT_TYPE_SELECTED) = [selfRef](Type data)
                {
                    if (auto box = selfRef.lock())
                        box->writeValue(data);
                };
            }
        }
    }

    virtual void enterEdit() override
    {
        m_button->focus();
    }

    virtual void cancelEdit() override
    {
        if (m_picker)
            m_picker->requestClose();
        m_picker.reset();
    }

protected:
    TextLabelPtr m_caption;
    ButtonPtr m_button;

    RefPtr<TypePickerBox> m_picker;
};

RTTI_BEGIN_TYPE_CLASS(DataBoxTypePicker);
    RTTI_METADATA(ElementClassNameMetadata).name("DataBoxTypePicker");
RTTI_END_TYPE();

//--

class DataBoxTypeFactory : public IDataBoxFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxTypeFactory, IDataBoxFactory);

public:
    virtual DataBoxPtr tryCreate(const rtti::DataViewInfo& info) const override
    {
        if (info.dataType == reflection::GetTypeObject<Type>())
            return RefNew<DataBoxTypePicker>();
        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxTypeFactory);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(ui)
