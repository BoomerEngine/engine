/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: view #]
***/

#include "build.h"
#include "action.h"
#include "rttiDataView.h"
#include "dataViewMulti.h"
#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE()

//----

DataViewMulti::DataViewMulti(IDataView** views, uint32_t count)
{
    m_extraViews.reserve(count-1);

    for (uint32_t i = 0; i < count; ++i)
    {
        if (views[i])
        {
            if (!m_mainView)
                m_mainView = AddRef(views[i]);
            else
                m_extraViews.pushBack(AddRef(views[i]));
        }
    }
}

DataViewMulti::~DataViewMulti()
{}

static void MergeMembersList(Array<DataViewMemberInfo>& currentList, const Array<DataViewMemberInfo>& extraList)
{
    for (auto i : currentList.indexRange().reversed())
    {
        const auto& current = currentList[i];

        bool valid = false;
        for (const auto& other : extraList)
        {
            if (other.name == current.name)
            {
                if (other.type == current.type && other.category == current.category)
                    valid = true;
                break;
            }
        }

        if (!valid)
            currentList.erase(i);
    }
}

static void MergeOptionList(Array<DataViewOptionInfo>& currentList, const Array<DataViewOptionInfo>& extraList)
{
    for (auto i : currentList.indexRange().reversed())
    {
        const auto& current = currentList[i];

        bool valid = false;
        for (const auto& other : extraList)
        {
            if (other.name == current.name)
            {
                valid = true;
                break;
            }
        }

        if (!valid)
            currentList.erase(i);
    }
}

#define RUN_SAFE(x) { auto ret = x; if (!ret.valid()) return ret; }

DataViewResult DataViewMulti::describeDataView(StringView viewPath, DataViewInfo& outInfo) const
{
    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    RUN_SAFE(m_mainView->describeDataView(viewPath, outInfo));

    for (const auto& view : m_extraViews)
    {
        DataViewInfo localInfo;
        localInfo.requestFlags = outInfo.requestFlags;
        localInfo.categoryFilter = outInfo.categoryFilter;

        RUN_SAFE(view->describeDataView(viewPath, localInfo));

        if (outInfo.flags.test(DataViewInfoFlagBit::LikeArray) != localInfo.flags.test(DataViewInfoFlagBit::LikeArray))
            return DataViewResultCode::ErrorIncompatibleMultiView;

        if (outInfo.flags.test(DataViewInfoFlagBit::LikeStruct) != localInfo.flags.test(DataViewInfoFlagBit::LikeStruct))
            return DataViewResultCode::ErrorIncompatibleMultiView;

        if (outInfo.flags.test(DataViewInfoFlagBit::LikeValue) != localInfo.flags.test(DataViewInfoFlagBit::LikeValue))
            return DataViewResultCode::ErrorIncompatibleMultiView;

        if (outInfo.flags.test(DataViewInfoFlagBit::Inlined) != localInfo.flags.test(DataViewInfoFlagBit::Inlined))
            return DataViewResultCode::ErrorIncompatibleMultiView;

        if (outInfo.dataType != localInfo.dataType)
            return DataViewResultCode::ErrorIncompatibleMultiView;

        if (localInfo.flags.test(DataViewInfoFlagBit::ResetableToBaseValue))
            outInfo.flags |= DataViewInfoFlagBit::ResetableToBaseValue;

        if (localInfo.flags.test(DataViewInfoFlagBit::ReadOnly))
            outInfo.flags |= DataViewInfoFlagBit::ReadOnly;

        outInfo.arraySize = std::min<uint32_t>(outInfo.arraySize, localInfo.arraySize);

        if (outInfo.requestFlags.test(DataViewRequestFlagBit::MemberList))
            MergeMembersList(outInfo.members, localInfo.members);

        if (outInfo.requestFlags.test(DataViewRequestFlagBit::OptionsList))
            MergeOptionList(outInfo.options, localInfo.options);
    }

    return DataViewResultCode::OK;
}

DataViewResult DataViewMulti::readDataView(StringView viewPath, void* targetData, Type targetType) const
{
    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    RUN_SAFE(m_mainView->readDataView(viewPath, targetData, targetType));

    for (const auto& view : m_extraViews)
    {
        DataHolder localData(targetType);

        RUN_SAFE(view->readDataView(viewPath, localData.data(), targetType));

        if (!targetType->compare(localData.data(), targetData))
            return DataViewResultCode::ErrorManyValues;
    }

    return DataViewResultCode::OK;
}

DataViewResult DataViewMulti::writeDataView(StringView viewPath, const void* sourceData, Type sourceType) const
{
    // TODO: atomic!

    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    RUN_SAFE(m_mainView->writeDataView(viewPath, sourceData, sourceType));

    for (const auto& view : m_extraViews)
    {
        RUN_SAFE(view->writeDataView(viewPath, sourceData, sourceType));
    }

    return DataViewResultCode::OK;
}

class MultiAction : public IAction
{
public:
    MultiAction(IAction* base)
        : m_base(AddRef(base))
    {}

    void addExtra(IAction* action)
    {
        if (action)
            m_extra.pushBack(AddRef(action));
    }

    virtual StringID id() const override
    {
        return m_base->id();
    }

    virtual StringBuf description() const override
    {
        return m_base->description();
    }

    virtual bool execute() override
    {
        if (!m_base->execute())
            return false;

        for (const auto& extra : m_extra)
            extra->execute();

        return true;
    }

    virtual bool undo() override
    {
        if (!m_base->undo())
            return false;

        for (const auto& extra : m_extra)
            extra->undo();

        return true;
    }

private:
    ActionPtr m_base;
    Array<ActionPtr> m_extra;
};

DataViewActionResult DataViewMulti::actionValueWrite(StringView viewPath, const void* sourceData, Type sourceType) const
{
    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    auto action = m_mainView->actionValueWrite(viewPath, sourceData, sourceType);
    if (!action)
        return action;

    auto ret = RefNew<MultiAction>(action.action);

    for (const auto& extra : m_extraViews)
    {
        auto action = extra->actionValueWrite(viewPath, sourceData, sourceType);
        if (!action)
            return action;

        ret->addExtra(action.action);
    }

    return ret;
}

DataViewActionResult DataViewMulti::actionValueReset(StringView viewPath) const
{
    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    auto action = m_mainView->actionValueReset(viewPath);
    if (!action)
        return action;

    auto ret = RefNew<MultiAction>(action.action);

    for (const auto& extra : m_extraViews)
    {
        auto action = extra->actionValueReset(viewPath);
        if (!action)
            return action;

        ret->addExtra(action.action);
    }

    return ret;
}

DataViewActionResult DataViewMulti::actionArrayClear(StringView viewPath) const
{
    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    auto action = m_mainView->actionArrayClear(viewPath);
    if (!action)
        return action;

    auto ret = RefNew<MultiAction>(action.action);

    for (const auto& extra : m_extraViews)
    {
        auto action = extra->actionArrayClear(viewPath);
        if (!action)
            return action;

        ret->addExtra(action.action);
    }

    return ret;
}

DataViewActionResult DataViewMulti::actionArrayInsertElement(StringView viewPath, uint32_t index) const
{
    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    auto action = m_mainView->actionArrayInsertElement(viewPath, index);
    if (!action)
        return action;

    auto ret = RefNew<MultiAction>(action.action);

    for (const auto& extra : m_extraViews)
    {
        auto action = extra->actionArrayInsertElement(viewPath, index);
        if (!action)
            return action;

        ret->addExtra(action.action);
    }

    return ret;
}

DataViewActionResult DataViewMulti::actionArrayRemoveElement(StringView viewPath, uint32_t index) const
{
    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    auto action = m_mainView->actionArrayRemoveElement(viewPath, index);
    if (!action)
        return action;

    auto ret = RefNew<MultiAction>(action.action);

    for (const auto& extra : m_extraViews)
    {
        auto action = extra->actionArrayRemoveElement(viewPath, index);
        if (!action)
            return action;

        ret->addExtra(action.action);
    }

    return ret;
}

DataViewActionResult DataViewMulti::actionArrayNewElement(StringView viewPath) const
{
    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    auto action = m_mainView->actionArrayNewElement(viewPath);
    if (!action)
        return action;

    auto ret = RefNew<MultiAction>(action.action);

    for (const auto& extra : m_extraViews)
    {
        auto action = extra->actionArrayNewElement(viewPath);
        if (!action)
            return action;

        ret->addExtra(action.action);
    }

    return ret;
}

DataViewActionResult DataViewMulti::actionObjectClear(StringView viewPath) const
{
    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    auto action = m_mainView->actionObjectClear(viewPath);
    if (!action)
        return action;

    auto ret = RefNew<MultiAction>(action.action);

    for (const auto& extra : m_extraViews)
    {
        auto action = extra->actionObjectClear(viewPath);
        if (!action)
            return action;

        ret->addExtra(action.action);
    }

    return ret;
}

DataViewActionResult DataViewMulti::actionObjectNew(StringView viewPath, ClassType objectClass) const
{
    if (!m_mainView)
        return DataViewResultCode::ErrorNullObject;

    auto action = m_mainView->actionObjectNew(viewPath, objectClass);
    if (!action)
        return action;

    auto ret = RefNew<MultiAction>(action.action);

    for (const auto& extra : m_extraViews)
    {
        auto action = extra->actionObjectNew(viewPath, objectClass);
        if (!action)
            return action;

        ret->addExtra(action.action);
    }

    return ret;
}

//----

END_BOOMER_NAMESPACE()

