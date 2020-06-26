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
#include "base/object/include/rttiDataView.h"

namespace ui
{

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
            m_button->bind("OnClick"_id) = [this]() { showColorPicker(); };
        }

        virtual void handleValueChange() override
        {
            base::Color data;

            if (readValue(data))
            {
                data.a = 255;
                m_colorBox->customBackgroundColor(data);
            }

            m_button->visibility(!readOnly());
        }

        virtual bool canExpandChildren() const override
        {
            return false;
        }

        void showColorPicker()
        {
            if (!m_picker)
            {
                base::Color data;
                readValue(data);

                m_picker = base::CreateSharedPtr<ColorPickerBox>(data, m_allowAlpha);
                m_picker->show(this, ui::PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));

                m_picker->bind("OnClosed"_id, this) = [](DataBoxColorPicker* box)
                {
                    box->m_picker.reset();
                };

                m_picker->bind("OnColorChanged"_id, this) = [](DataBoxColorPicker* box, base::Color data)
                {
                    box->writeValue(data);
                };
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

        base::RefPtr<ColorPickerBox> m_picker;
    };

    RTTI_BEGIN_TYPE_CLASS(DataBoxColorPicker);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxColorPicker");
    RTTI_END_TYPE();

    //--

    class DataBoxColorFactory : public IDataBoxFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxColorFactory, IDataBoxFactory);

    public:
        virtual DataBoxPtr tryCreate(const base::rtti::DataViewInfo& info) const override
        {
            if (info.dataType == base::reflection::GetTypeObject<base::Color>())
                return base::CreateSharedPtr<DataBoxColorPicker>(true);
            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(DataBoxColorFactory);
    RTTI_END_TYPE();

    //--

} // ui