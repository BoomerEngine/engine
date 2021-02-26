/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDataBox.h"
#include "uiCheckBox.h"
#include "core/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// bool editor
class DataBoxBool : public IDataBox
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxBool, IDataBox);

public:
    DataBoxBool()
    {
        m_box = createChild<CheckBox>();
        m_box->bind(EVENT_CLICKED) = [this]()
        {
            if (m_box->isEnabled())
                write();
        };
    } 

    virtual void handleValueChange() override
    {
        bool value = false;

        const auto ret = readValue(value);
        if (ret.code == DataViewResultCode::OK)
        {
            m_box->state(value ? CheckBoxState::Checked : CheckBoxState::Unchecked);
            m_box->enable(!readOnly());
            m_box->visibility(true);
        }
        else if (ret.code == DataViewResultCode::ErrorManyValues)
        {
            m_box->state(CheckBoxState::Undecided);
            m_box->enable(!readOnly());
            m_box->visibility(true);
        }
        else
        {
            // TODO: error!
            m_box->state(CheckBoxState::Undecided);
            m_box->enable(false);
        }
    }

protected:
    RefPtr<CheckBox> m_box;

    void write()
    {
        auto state = m_box->state();
        if (state != CheckBoxState::Undecided)
        {
            bool value = (state == CheckBoxState::Checked);
            writeValue(value);
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxBool);
    RTTI_METADATA(ElementClassNameMetadata).name("DataBoxBool");
RTTI_END_TYPE();

//--

class DataBoxBoolFactory : public IDataBoxFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxBoolFactory, IDataBoxFactory);

public:
    virtual DataBoxPtr tryCreate(const rtti::DataViewInfo& info) const override
    {
        if (info.dataType == reflection::GetTypeObject<bool>())
            return RefNew<DataBoxBool>();
        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxBoolFactory);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(ui)
