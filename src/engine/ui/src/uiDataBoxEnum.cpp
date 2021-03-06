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
#include "core/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

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

        m_box->bind(EVENT_COMBO_SELECTED) = [this]()
        {
            if (m_box->isEnabled())
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
        HashSet<StringID> options;

        StringID currentValue;

        const auto ret = readValue(currentValue);
        if (ret.code == DataViewResultCode::OK || ret.code == DataViewResultCode::ErrorManyValues)
        {
            DataViewInfo info;
            info.requestFlags |= DataViewRequestFlagBit::OptionsList;
            const auto ret = data()->describeDataView(path(), info);

            if (ret.code == DataViewResultCode::OK)
            {
                for (const auto option : info.options)
                    options.insert(option.name);

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

                m_box->visibility(true);
                m_box->enable(!readOnly());
            }
            else
            {
                // TODO: error message
                m_box->visibility(false);
            }
        }
        else
        {
            // TODO: error message
            m_box->visibility(false);
        }
    }

protected:
    RefPtr<ComboBox> m_box;
    Array<StringID> m_options;
      
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
    RTTI_METADATA(ElementClassNameMetadata).name("DataBoxEnum");
RTTI_END_TYPE();

//--

class DataBoxEnumFactory : public IDataBoxFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxEnumFactory, IDataBoxFactory);

public:
    virtual DataBoxPtr tryCreate(const DataViewInfo& info) const override
    {
        if (info.flags.test(DataViewInfoFlagBit::Constrainded))
            return RefNew<DataBoxEnum>();

        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxEnumFactory);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(ui)
