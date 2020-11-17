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
#include "uiClassPickerBox.h"
#include "base/object/include/rttiDataView.h"

namespace ui
{

    //--

    /// general class picker
    class DataBoxClassPicker : public IDataBox
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxClassPicker, IDataBox);

    public:
        DataBoxClassPicker()
        {
            layoutHorizontal();

            m_caption = createChild<TextLabel>();
            m_caption->customHorizontalAligment(ElementHorizontalLayout::Expand);
            m_caption->customVerticalAligment(ElementVerticalLayout::Middle);

            m_button = createChildWithType<Button>("DataPropertyButton"_id);
            m_button->customVerticalAligment(ElementVerticalLayout::Middle);
            m_button->createChild<TextLabel>("[img:class]");
            m_button->tooltip("Select from class list");
            m_button->bind(EVENT_CLICKED) = [this]() { showClassPicker(); };
        }

        virtual void handleValueChange() override
        {
            base::ClassType data;

            const auto ret = readValue(data);
            if (ret.code == base::DataViewResultCode::OK)
            {
                base::StringBuilder txt;
                txt << data;
                m_caption->text(txt.toString());
                m_button->visibility(!readOnly());
            }
            else if (ret.code == base::DataViewResultCode::ErrorManyValues)
            {
                m_caption->text("<multiple values>");
                m_button->visibility(!readOnly());
            }
            else
            {
                m_caption->text(base::TempString("[tag:#F00][img:error] {}[/tag]", ret));
                m_button->visibility(false);
            }            
        }

        void showClassPicker()
        {
            if (!m_picker)
            {
                base::ClassType data;
                const auto ret = readValue(data);
                if (ret.code == base::DataViewResultCode::OK || ret.code == base::DataViewResultCode::ErrorManyValues)
                {
                    bool allowNullType = false;
                    bool allowAbstractType = true;

                    m_picker = base::RefNew<ClassPickerBox>(nullptr, data, allowAbstractType, allowNullType);
                    m_picker->show(this, ui::PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));

                    auto safeRef = base::RefWeakPtr<DataBoxClassPicker>(this); // NOTE: picker may linger and thus close after the data box gets destroyed

                    m_picker->bind(EVENT_WINDOW_CLOSED) = [safeRef]() {
                        if (auto box = safeRef.lock())
                            box->m_picker.reset();
                    };

                    m_picker->bind(EVENT_CLASS_SELECTED) = [safeRef](base::ClassType data) {
                        if (auto box = safeRef.lock())
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

        base::RefPtr<ClassPickerBox> m_picker;
    };

    RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxClassPicker);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxClassPicker");
    RTTI_END_TYPE();

    //--

    /// specific class picker
    class DataBoxSpecificClassPicker : public IDataBox
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxSpecificClassPicker, IDataBox);

    public:
        DataBoxSpecificClassPicker(base::ClassType rootClass, base::Type classMetaType)
            : m_rootClass(rootClass)
            , m_metaType(classMetaType)
        {
            layoutHorizontal();

            m_caption = createChild<TextLabel>();
            m_caption->customHorizontalAligment(ElementHorizontalLayout::Expand);
            m_caption->customVerticalAligment(ElementVerticalLayout::Middle);

            m_button = createChildWithType<Button>("DataPropertyButton"_id);
            m_button->customVerticalAligment(ElementVerticalLayout::Middle);
            m_button->createChild<TextLabel>("[img:class]");
            m_button->tooltip("Select from class list");
            m_button->bind(EVENT_CLICKED) = [this]() { showClassPicker(); };
        }

        virtual void handleValueChange() override
        {
            base::ClassType data;

            auto ret = readValue(&data, m_metaType);
            if (ret.valid())
            {
                base::StringBuilder txt;
                txt << data;
                m_caption->text(txt.toString());
            }
            else if (ret.code == base::DataViewResultCode::ErrorManyValues)
            {
                m_caption->text("<many values>");
            }
            else if (ret.code == base::DataViewResultCode::ErrorManyValues)
            {
                m_caption->text(base::TempString("[tag:#F00][img:error] {}[/tag]", ret));
            }

            m_button->visibility(!readOnly());
        }

        void showClassPicker()
        {
            if (!m_picker)
            {
                base::ClassType data;
                auto ret = readValue(&data, m_metaType);
                if (ret.code == base::DataViewResultCode::OK || ret.code == base::DataViewResultCode::ErrorManyValues)
                {
                    bool allowNullType = false;
                    bool allowAbstractType = true;

                    m_picker = base::RefNew<ClassPickerBox>(m_rootClass, data, allowAbstractType, allowNullType);
                    m_picker->show(this, ui::PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));

                    auto safeRef = base::RefWeakPtr<DataBoxSpecificClassPicker>(this); // NOTE: picker may linger and thus close after the data box gets destroyed

                    m_picker->bind(EVENT_WINDOW_CLOSED) = [safeRef]() {
                        if (auto box = safeRef.lock())
                            box->m_picker.reset();
                    };

                    m_picker->bind(EVENT_CLASS_SELECTED) = [safeRef](base::ClassType data) {
                        if (auto box = safeRef.lock())
                            box->writeValue(&data, box->m_metaType);
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

        base::RefPtr<ClassPickerBox> m_picker;
        base::ClassType m_rootClass;
        base::Type m_metaType;
    };

    RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxSpecificClassPicker);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxSpecificClassPicker");
    RTTI_END_TYPE();

    //--

    class DataBoxClassFactory : public IDataBoxFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxClassFactory, IDataBoxFactory);

    public:
        virtual DataBoxPtr tryCreate(const base::rtti::DataViewInfo& info) const override
        {
            if (info.dataType == base::reflection::GetTypeObject<base::ClassType>())
            {
                return base::RefNew<DataBoxClassPicker>();
            }
            else if (info.dataType.metaType() == base::rtti::MetaType::ClassRef)
            {
                const auto rootClass = info.dataType.innerType().toClass();
                return base::RefNew<DataBoxSpecificClassPicker>(rootClass, info.dataType);
            }

            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(DataBoxClassFactory);
    RTTI_END_TYPE();

    //--

} // ui