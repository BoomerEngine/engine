/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDynamicChoiceBox.h"
#include "uiDataBoxCustomChoice.h"
#include "core/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

DataBoxCustomChoice::DataBoxCustomChoice()
{
    m_box = createChild<DynamicChoiceBox>();
    m_box->customHorizontalAligment(ElementHorizontalLayout::Expand);
    m_box->customVerticalAligment(ElementVerticalLayout::Expand);

    m_box->bind(EVENT_DYNAMIC_CHOICE_SELECTED) = [this](ui::DynamicChoiceOption option)
    {
        writeOptionValue(option.index, option.text);
    };

    m_box->bind(EVENT_DYNAMIC_CHOICE_QUERY) = [this](ui::DynamicChoiceListPtr list)
    {
        InplaceArray<StringBuf, 256> options;
        queryOptions(options);

        for (const auto& option : options)
            list->add(option);
    };
}

void DataBoxCustomChoice::enterEdit()
{
    m_box->focus();
}

void DataBoxCustomChoice::cancelEdit()
{
    m_box->closePopupList();
}

bool DataBoxCustomChoice::canExpandChildren() const
{
    return false;
}

void DataBoxCustomChoice::handleValueChange()
{
    StringBuf currentValue;
    const auto ret = readOptionValue(currentValue);

    if (ret.code == DataViewResultCode::OK)
    {
        m_box->text(currentValue);
        m_box->enable(!readOnly());
    }
    else if (ret.code == DataViewResultCode::ErrorManyValues)
    {
        m_box->text("(many values)");
        m_box->enable(!readOnly());
    }
    else
    {
        m_box->text("(disabled)");
        m_box->enable(false);
    }
}

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(DataBoxCustomChoice);
    RTTI_METADATA(ElementClassNameMetadata).name("DataBoxEnum");
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(ui)
