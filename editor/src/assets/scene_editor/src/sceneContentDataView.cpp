/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#include "build.h"
#include "sceneContentDataView.h"
#include "base/object/include/objectTemplate.h"

namespace ed
{

    //--

    SceneContentNodeDataView::SceneContentNodeDataView(Array<SceneContentEditableObject>&& objects)
        : m_objects(std::move(objects))
    {
        for (auto& info : m_objects)
        {
            if (info.editableData)
                info.view = info.editableData->createDataView();
            else if (info.baseData)
                info.view = info.baseData->createDataView(true);

            if (!m_tempView)
                m_tempView = info.view;
        }    
    }

    DataViewResult SceneContentNodeDataView::describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const
    {
        if (m_tempView)
            return m_tempView->describeDataView(viewPath, outInfo);

        return DataViewResultCode::ErrorNullObject;
    }

    DataViewResult SceneContentNodeDataView::readDataView(StringView<char> viewPath, void* targetData, Type targetType) const
    {
        if (m_tempView)
            return m_tempView->readDataView(viewPath, targetData, targetType);

        return DataViewResultCode::ErrorNullObject;
    }

    DataViewResult SceneContentNodeDataView::writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType) const
    {
        if (m_tempView)
            return m_tempView->writeDataView(viewPath, sourceData, sourceType);

        return DataViewResultCode::ErrorNullObject;
    }

    void SceneContentNodeDataView::handleFullObjectChange()
    {
        dispatchFullStructureChanged();
    }

    void SceneContentNodeDataView::handlePropertyChanged(StringView<char> fullPath, bool parentNotification)
    {
        if (!parentNotification)
            dispatchPropertyChanged(fullPath);
    }

    void SceneContentNodeDataView::attachObserver(StringView<char> path, IDataViewObserver* observer)
    {
        IDataView::attachObserver(path, observer);

        for (const auto& obj : m_objects)
            if (obj.view)
                obj.view->attachObserver(path, this);
    }

    void SceneContentNodeDataView::detachObserver(StringView<char> path, IDataViewObserver* observer)
    {
        IDataView::detachObserver(path, observer);

        for (const auto& obj : m_objects)
            if (obj.view)
                obj.view->detachObserver(path, this);
    }

    DataViewActionResult SceneContentNodeDataView::actionValueWrite(StringView<char> viewPath, const void* sourceData, Type sourceType) const
    {
        if (m_tempView)
            return m_tempView->actionValueWrite(viewPath, sourceData, sourceType);

        return DataViewResultCode::ErrorNullObject;
    }

    DataViewActionResult SceneContentNodeDataView::actionValueReset(StringView<char> viewPath) const
    {
        if (m_tempView)
            return m_tempView->actionValueReset(viewPath);

        return DataViewResultCode::ErrorNullObject;
    }

    DataViewActionResult SceneContentNodeDataView::actionArrayClear(StringView<char> viewPath) const
    {
        if (m_tempView)
            return m_tempView->actionArrayClear(viewPath);

        return DataViewResultCode::ErrorNullObject;
    }

    DataViewActionResult SceneContentNodeDataView::actionArrayInsertElement(StringView<char> viewPath, uint32_t index) const
    {
        if (m_tempView)
            return m_tempView->actionArrayInsertElement(viewPath, index);

        return DataViewResultCode::ErrorNullObject;
    }

    DataViewActionResult SceneContentNodeDataView::actionArrayRemoveElement(StringView<char> viewPath, uint32_t index) const
    {
        if (m_tempView)
            return m_tempView->actionArrayRemoveElement(viewPath, index);

        return DataViewResultCode::ErrorNullObject;
    }

    DataViewActionResult SceneContentNodeDataView::actionArrayNewElement(StringView<char> viewPath) const
    {
        if (m_tempView)
            return m_tempView->actionArrayNewElement(viewPath);

        return DataViewResultCode::ErrorNullObject;
    }

    DataViewActionResult SceneContentNodeDataView::actionObjectClear(StringView<char> viewPath) const
    {
        if (m_tempView)
            return m_tempView->actionObjectClear(viewPath);

        return DataViewResultCode::ErrorNullObject;
    }

    DataViewActionResult SceneContentNodeDataView::actionObjectNew(StringView<char> viewPath, ClassType objectClass) const
    {
        if (m_tempView)
            return m_tempView->actionObjectNew(viewPath, objectClass);

        return DataViewResultCode::ErrorNullObject;
    }

    //--
    
} // ed
