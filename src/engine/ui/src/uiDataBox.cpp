/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDataBox.h"

#include "core/object/include/action.h"
#include "core/object/include/actionHistory.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDataBox);
    RTTI_METADATA(ElementClassNameMetadata).name("DataBox");
RTTI_END_TYPE();

IDataBox::IDataBox()
    : m_readOnly(false)
{
    hitTest(HitTestState::Enabled);
}

void IDataBox::bindData(const DataViewPtr& data, const StringBuf& path, bool readOnly)
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

void IDataBox::bindActionHistory(ActionHistory* ah)
{
    m_actionHistory = AddRef(ah);
}

IDataBox::~IDataBox()
{
    if (m_data)
        m_data->detachObserver(m_path, this);
}

void IDataBox::handlePropertyChanged(StringView fullPath, bool parentNotification)
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

DataViewResult IDataBox::executeAction(const ActionPtr& action)
{
    if (!action)
        return DataViewResultCode::ErrorIllegalOperation;

    if (m_actionHistory)
    {
        if (!m_actionHistory->execute(action))
            return DataViewResultCode::ErrorIllegalOperation;
    }
    else
    {
        if (!action->execute())
            return DataViewResultCode::ErrorIllegalOperation;
    }

    return DataViewResultCode::OK;
}

DataViewResult IDataBox::executeAction(const DataViewActionResult& action)
{
    if (!action)
        return action.result;

    return executeAction(action.action);
}

//---

DataViewResult IDataBox::readValue(void* data, const Type dataType)
{
    if (!m_data)
        return DataViewResultCode::ErrorIllegalOperation;

    auto ret = m_data->readDataView(m_path, data, dataType);
    if (!ret.valid())
        return ret;

    return DataViewResultCode::OK;
}

DataViewResult IDataBox::writeValue(const void* data, const Type dataType)
{
    if (!m_data)
        return DataViewResultCode::ErrorIllegalOperation;

    auto actionResult = m_data->actionValueWrite(m_path, data, dataType);
    return executeAction(actionResult);
}

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDataBoxFactory);
RTTI_END_TYPE();

IDataBoxFactory::~IDataBoxFactory()
{}

//---

class DataBoxRegistry : public ISingleton
{
    DECLARE_SINGLETON(DataBoxRegistry);

public:
    DataBoxRegistry()
    {
        InplaceArray<SpecificClassType<IDataBoxFactory>, 64> dataBoxClasses;
        RTTI::GetInstance().enumClasses(dataBoxClasses);

        m_templates.reserve(dataBoxClasses.size());
        for (const auto classPtr : dataBoxClasses)
            m_templates.pushBack(classPtr->createPointer<IDataBoxFactory>());
    }

    virtual void deinit() override
    {
        m_templates.clearPtr();
    }

    DataBoxPtr create(const DataViewInfo& info) const
    {
        for (const auto* ptr : m_templates)
            if (auto ret = ptr->tryCreate(info))
                return ret;

        return nullptr;
    }

public:
    Array<IDataBoxFactory*> m_templates;
};

DataBoxPtr IDataBox::CreateForType(const DataViewInfo& info)
{
    return DataBoxRegistry::GetInstance().create(info);
}

//---

END_BOOMER_NAMESPACE_EX(ui)
