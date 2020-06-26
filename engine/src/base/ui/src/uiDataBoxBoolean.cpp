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
#include "base/object/include/rttiDataView.h"

namespace ui
{

    //--

    /// bool editor
    class DataBoxBool : public IDataBox
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxBool, IDataBox);

    public:
        DataBoxBool()
        {
            m_box = createChild<CheckBox>();
            m_box->bind("OnClick"_id) = [this]()
            {
                write();
            };
        }

        virtual void handleValueChange() override
        {
            bool value = false;
            auto state = CheckBoxState::Undecided;
            if (readValue(value))
                state = value ? CheckBoxState::Checked : CheckBoxState::Unchecked;

            m_box->state(state);
            m_box->enable(!readOnly());
        }

        virtual ui::IElement* handleFocusForwarding() override
        {
            return m_box;
        }

    protected:
        base::RefPtr<CheckBox> m_box;

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
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxBool");
    RTTI_END_TYPE();

    //--

    class DataBoxBoolFactory : public IDataBoxFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxBoolFactory, IDataBoxFactory);

    public:
        virtual DataBoxPtr tryCreate(const base::rtti::DataViewInfo& info) const override
        {
            if (info.dataType == base::reflection::GetTypeObject<bool>())
                return base::CreateSharedPtr<DataBoxBool>();
            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(DataBoxBoolFactory);
    RTTI_END_TYPE();

    //--

} // ui