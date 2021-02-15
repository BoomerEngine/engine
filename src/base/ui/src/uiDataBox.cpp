/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDataBox.h"

#include "base/object/include/action.h"
#include "base/object/include/actionHistory.h"

namespace ui
{
    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDataBox);
        RTTI_METADATA(ElementClassNameMetadata).name("DataBox");
    RTTI_END_TYPE();

    IDataBox::IDataBox()
        : m_readOnly(false)
    {
        hitTest(HitTestState::Enabled);
    }

    void IDataBox::bindData(const base::DataViewPtr& data, const base::StringBuf& path, bool readOnly)
    {
        if (m_data)
            m_data->detachObserver(m_path, this);

        m_data = data;
        m_path = path;
        m_readOnly = readOnly;

        if (m_data)
            m_data->attachObserver(m_path, this);

        handleValueChange();
    }

    void IDataBox::bindActionHistory(base::ActionHistory* ah)
    {
        m_actionHistory = AddRef(ah);
    }

    IDataBox::~IDataBox()
    {
        if (m_data)
            m_data->detachObserver(m_path, this);
    }

    void IDataBox::handlePropertyChanged(base::StringView fullPath, bool parentNotification)
    {
        if (m_path == fullPath)
            handleValueChange();
    }

    void IDataBox::enterEdit()
    {
        focus();
    }

    void IDataBox::cancelEdit()
    {
        handleValueChange();
    }

    void IDataBox::handleValueChange()
    {

    }

    bool IDataBox::canExpandChildren() const
    {
        return false;
    }

    //--

    base::DataViewResult IDataBox::executeAction(const base::ActionPtr& action)
    {
        if (!action)
            return base::DataViewResultCode::ErrorIllegalOperation;

        if (m_actionHistory)
        {
            if (!m_actionHistory->execute(action))
                return base::DataViewResultCode::ErrorIllegalOperation;
        }
        else
        {
            if (!action->execute())
                return base::DataViewResultCode::ErrorIllegalOperation;
        }

        return base::DataViewResultCode::OK;
    }

    base::DataViewResult IDataBox::executeAction(const base::DataViewActionResult& action)
    {
        if (!action)
            return action.result;

        return executeAction(action.action);
    }

    //---

    base::DataViewResult IDataBox::readValue(void* data, const base::Type dataType)
    {
        if (!m_data)
            return base::DataViewResultCode::ErrorIllegalOperation;

        if (auto err = HasError(m_data->readDataView(m_path, data, dataType)))
            return err;

        return base::DataViewResultCode::OK;
    }

    base::DataViewResult IDataBox::writeValue(const void* data, const base::Type dataType)
    {
        if (!m_data)
            return base::DataViewResultCode::ErrorIllegalOperation;

        auto actionResult = m_data->actionValueWrite(m_path, data, dataType);
        return executeAction(actionResult);
    }

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDataBoxFactory);
    RTTI_END_TYPE();

    IDataBoxFactory::~IDataBoxFactory()
    {}

    //---

    class DataBoxRegistry : public base::ISingleton
    {
        DECLARE_SINGLETON(DataBoxRegistry);

    public:
        DataBoxRegistry()
        {
            base::InplaceArray<base::SpecificClassType<IDataBoxFactory>, 64> dataBoxClasses;
            RTTI::GetInstance().enumClasses(dataBoxClasses);

            m_templates.reserve(dataBoxClasses.size());
            for (const auto classPtr : dataBoxClasses)
                m_templates.pushBack(classPtr->createPointer<IDataBoxFactory>());
        }

        virtual void deinit() override
        {
            m_templates.clearPtr();
        }

        DataBoxPtr create(const base::rtti::DataViewInfo& info) const
        {
            for (const auto* ptr : m_templates)
                if (auto ret = ptr->tryCreate(info))
                    return ret;

            return nullptr;
        }

    public:
        base::Array<IDataBoxFactory*> m_templates;
    };

    DataBoxPtr IDataBox::CreateForType(const base::rtti::DataViewInfo& info)
    {
        return DataBoxRegistry::GetInstance().create(info);
    }

    //---

} // ui