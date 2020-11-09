/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#pragma once
#include "base/object/include/dataView.h"

namespace ed
{
    //--

    struct SceneContentEditableObject
    {
        StringBuf name;
        SceneContentEntityNodePtr owningNode;
        base::ObjectTemplatePtr baseData;
        base::ObjectTemplatePtr editableData; // must be valid
        DataViewPtr view;
    };

    //--

    class ASSETS_SCENE_EDITOR_API SceneContentNodeDataView : public IDataView, public IDataViewObserver
    {
    public:
        SceneContentNodeDataView(Array<SceneContentEditableObject>&& objects);

        virtual DataViewResult describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const override;
        virtual DataViewResult readDataView(StringView<char> viewPath, void* targetData, Type targetType) const override;
        virtual DataViewResult writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType) const override;

        virtual void attachObserver(StringView<char> path, IDataViewObserver* observer) override;
        virtual void detachObserver(StringView<char> path, IDataViewObserver* observer) override;

        virtual DataViewActionResult actionValueWrite(StringView<char> viewPath, const void* sourceData, Type sourceType) const override;
        virtual DataViewActionResult actionValueReset(StringView<char> viewPath) const override;
        virtual DataViewActionResult actionArrayClear(StringView<char> viewPath) const override;
        virtual DataViewActionResult actionArrayInsertElement(StringView<char> viewPath, uint32_t index) const override;
        virtual DataViewActionResult actionArrayRemoveElement(StringView<char> viewPath, uint32_t index) const override;
        virtual DataViewActionResult actionArrayNewElement(StringView<char> viewPath) const override;
        virtual DataViewActionResult actionObjectClear(StringView<char> viewPath) const override;
        virtual DataViewActionResult actionObjectNew(StringView<char> viewPath, ClassType objectClass) const override;

    private:
        Array<SceneContentEditableObject> m_objects;

        DataViewPtr m_tempView;

        virtual void handleFullObjectChange() override final;
        virtual void handlePropertyChanged(StringView<char> fullPath, bool parentNotification) override final;
    };

    //--

} // ed
