/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDataBox.h"
#include "uiComboBox.h"
#include "base/object/include/rttiDataView.h"

namespace ui
{

    //--

    /// Enum selector
    class DataBoxEnum : public IDataBox
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxEnum, IDataBox);

    public:
        DataBoxEnum()
        {
            m_box = createChild<ComboBox>();
            m_box->customHorizontalAligment(ElementHorizontalLayout::Expand);
            m_box->customVerticalAligment(ElementVerticalLayout::Expand);

            m_box->bind("OnChanged"_id) = [this]()
            {
                write();
            };
        }

        virtual void enterEdit() override
        {
            m_box->focus();
        }

        virtual void cancelEdit() override
        {
            m_box->closePopupList();
        }

        virtual void handleValueChange() override
        {
            base::HashSet<base::StringID> options;

            const auto count = data()->size();
            for (uint32_t i = 0; i < count; ++i)
            {
                base::rtti::DataViewInfo info;
                info.requestFlags |= base::rtti::DataViewRequestFlagBit::OptionsList;

                if (data()->describe(i, path(), info))
                {
                    for (const auto& option : info.options)
                        options.insert(option.name);
                }
            }

            base::StringID currentValue;
            readValue(currentValue);

            if (m_options != options.keys())
            {
                m_box->closePopupList();
                m_box->clearOptions();
                m_box->selectOption(-1);

                for (uint32_t i = 0; i < options.keys().size(); ++i)
                {
                    const auto name = options.keys()[i];
                    m_box->addOption(name.c_str());

                    if (name == currentValue)
                        m_box->selectOption(i);
                }

                m_options = options.keys();
            }
            else
            {
                const auto index = m_options.find(currentValue);
                m_box->selectOption(index);
            }

            m_box->enable(!readOnly());
        }

    protected:
        base::RefPtr<ComboBox> m_box;
        base::Array<base::StringID> m_options;
      
        void write()
        {
            if (m_box->numOptions())
            {
                auto optionIndex = m_box->selectedOption();
                if (optionIndex >= 0 && optionIndex <= m_options.lastValidIndex())
                {
                    const auto& optionValue = m_options[optionIndex];
                    writeValue(optionValue);
                }
            }
        }
    };

    RTTI_BEGIN_TYPE_CLASS(DataBoxEnum);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxEnum");
    RTTI_END_TYPE();

    //--

    class DataBoxEnumFactory : public IDataBoxFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxEnumFactory, IDataBoxFactory);

    public:
        virtual DataBoxPtr tryCreate(const base::rtti::DataViewInfo& info) const override
        {
            if (info.flags.test(base::rtti::DataViewInfoFlagBit::Constrainded))
                return base::CreateSharedPtr<DataBoxEnum>();

            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(DataBoxEnumFactory);
    RTTI_END_TYPE();

    //--

} // ui
