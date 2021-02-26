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
#include "uiColorPickerBox.h"
#include "core/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// color picker
class DataBoxColorPicker : public IDataBox
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxColorPicker, IDataBox);

public:
    DataBoxColorPicker(bool allowAlpha=false)
        : m_allowAlpha(allowAlpha) 
    {
        layoutHorizontal();

        m_colorBox = createChildWithType<IElement>("DataBoxColorPreview"_id);

        m_button = createChildWithType<Button>("DataPropertyButton"_id);
        m_button->customVerticalAligment(ElementVerticalLayout::Middle);
        m_button->createChild<TextLabel>("[img:color_wheel]");
        m_button->tooltip("Select color");
        m_button->bind(EVENT_CLICKED) = [this]() { showColorPicker(); };
    }

    virtual void handleValueChange() override
    {
        Color data;
        const auto ret = readValue(data);

        if (ret.code == DataViewResultCode::OK)
        {
            data.a = 255;
            m_colorBox->customBackgroundColor(data);
            m_button->visibility(!readOnly());
        }
        else if (ret.code == DataViewResultCode::ErrorManyValues)
        {
            // TODO: "many values"
            m_button->visibility(!readOnly());
        }
        else
        {
            // TODO: error
            m_button->visibility(false);
        }
    }

    virtual bool canExpandChildren() const override
    {
        return false;
    }

    void showColorPicker()
    {
        if (!m_picker)
        {
            Color data;
            const auto ret = readValue(data);

            if (ret.code == DataViewResultCode::OK || ret.code == DataViewResultCode::ErrorManyValues)
            {
                m_picker = RefNew<ColorPickerBox>(data, m_allowAlpha);
                m_picker->show(this, PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));

                auto selfRef = RefWeakPtr<DataBoxColorPicker>(this);

                m_picker->bind(EVENT_WINDOW_CLOSED) = [selfRef]()
                {
                    if (auto box = selfRef.lock())
                        box->m_picker.reset();
                };

                m_picker->bind(EVENT_COLOR_SELECTED) = [selfRef](Color data)
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
    ElementPtr m_colorBox;
    ButtonPtr m_button;
    bool m_allowAlpha;

    RefPtr<ColorPickerBox> m_picker;
};

RTTI_BEGIN_TYPE_CLASS(DataBoxColorPicker);
    RTTI_METADATA(ElementClassNameMetadata).name("DataBoxColorPicker");
RTTI_END_TYPE();

//--

class DataBoxColorFactory : public IDataBoxFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxColorFactory, IDataBoxFactory);

public:
    virtual DataBoxPtr tryCreate(const rtti::DataViewInfo& info) const override
    {
        if (info.dataType == reflection::GetTypeObject<Color>())
            return RefNew<DataBoxColorPicker>(true);
        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxColorFactory);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(ui)
