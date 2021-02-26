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
#include "core/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

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
        ClassType data;

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
            m_caption->text("<multiple values>");
            m_button->visibility(!readOnly());
        }
        else
        {
            m_caption->text(TempString("[tag:#F00][img:error] {}[/tag]", ret));
            m_button->visibility(false);
        }            
    }

    void showClassPicker()
    {
        if (!m_picker)
        {
            ClassType data;
            const auto ret = readValue(data);
            if (ret.code == DataViewResultCode::OK || ret.code == DataViewResultCode::ErrorManyValues)
            {
                bool allowNullType = false;
                bool allowAbstractType = true;

                m_picker = RefNew<ClassPickerBox>(nullptr, data, allowAbstractType, allowNullType);
                m_picker->show(this, PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));

                auto safeRef = RefWeakPtr<DataBoxClassPicker>(this); // NOTE: picker may linger and thus close after the data box gets destroyed

                m_picker->bind(EVENT_WINDOW_CLOSED) = [safeRef]() {
                    if (auto box = safeRef.lock())
                        box->m_picker.reset();
                };

                m_picker->bind(EVENT_CLASS_SELECTED) = [safeRef](ClassType data) {
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

    RefPtr<ClassPickerBox> m_picker;
};

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxClassPicker);
    RTTI_METADATA(ElementClassNameMetadata).name("DataBoxClassPicker");
RTTI_END_TYPE();

//--

/// specific class picker
class DataBoxSpecificClassPicker : public IDataBox
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxSpecificClassPicker, IDataBox);

public:
    DataBoxSpecificClassPicker(ClassType rootClass, Type classMetaType)
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
        ClassType data;

        auto ret = readValue(&data, m_metaType);
        if (ret.valid())
        {
            StringBuilder txt;
            txt << data;
            m_caption->text(txt.toString());
        }
        else if (ret.code == DataViewResultCode::ErrorManyValues)
        {
            m_caption->text("<many values>");
        }
        else if (ret.code == DataViewResultCode::ErrorManyValues)
        {
            m_caption->text(TempString("[tag:#F00][img:error] {}[/tag]", ret));
        }

        m_button->visibility(!readOnly());
    }

    void showClassPicker()
    {
        if (!m_picker)
        {
            ClassType data;
            auto ret = readValue(&data, m_metaType);
            if (ret.code == DataViewResultCode::OK || ret.code == DataViewResultCode::ErrorManyValues)
            {
                bool allowNullType = false;
                bool allowAbstractType = true;

                m_picker = RefNew<ClassPickerBox>(m_rootClass, data, allowAbstractType, allowNullType);
                m_picker->show(this, PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));

                auto safeRef = RefWeakPtr<DataBoxSpecificClassPicker>(this); // NOTE: picker may linger and thus close after the data box gets destroyed

                m_picker->bind(EVENT_WINDOW_CLOSED) = [safeRef]() {
                    if (auto box = safeRef.lock())
                        box->m_picker.reset();
                };

                m_picker->bind(EVENT_CLASS_SELECTED) = [safeRef](ClassType data) {
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

    RefPtr<ClassPickerBox> m_picker;
    ClassType m_rootClass;
    Type m_metaType;
};

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxSpecificClassPicker);
    RTTI_METADATA(ElementClassNameMetadata).name("DataBoxSpecificClassPicker");
RTTI_END_TYPE();

//--

class DataBoxClassFactory : public IDataBoxFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxClassFactory, IDataBoxFactory);

public:
    virtual DataBoxPtr tryCreate(const rtti::DataViewInfo& info) const override
    {
        if (info.dataType == reflection::GetTypeObject<ClassType>())
        {
            return RefNew<DataBoxClassPicker>();
        }
        else if (info.dataType.metaType() == rtti::MetaType::ClassRef)
        {
            const auto rootClass = info.dataType.innerType().toClass();
            return RefNew<DataBoxSpecificClassPicker>(rootClass, info.dataType);
        }

        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxClassFactory);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(ui)
