/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: view #]
***/

#pragma once

#include "base/containers/include/array.h"
#include "base/containers/include/inplaceArray.h"
#include "base/memory/include/structurePool.h"

#include "object.h"
#include "objectObserver.h"
#include "dataView.h"

namespace base
{

    ///---

    /// native data source 
    class BASE_OBJECT_API DataViewNative : public IDataView, public IObjectObserver
    {
    public:
        DataViewNative(IObject* obj); // adds ref
        virtual ~DataViewNative();

        // get the object we edit
        INLINE const ObjectPtr& object() const { return m_object; }

        // IDataView
        virtual DataViewResult describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const override;
        virtual DataViewResult readDataView(StringView<char> viewPath, void* targetData, Type targetType) const override;
        virtual DataViewResult writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType) const override;

        // IDataView - actions
        virtual DataViewActionResult actionValueWrite(StringView<char> viewPath, const void* sourceData, Type sourceType) const override;
        virtual DataViewActionResult actionValueReset(StringView<char> viewPath) const override;
        virtual DataViewActionResult actionArrayClear(StringView<char> viewPath) const override;
        virtual DataViewActionResult actionArrayInsertElement(StringView<char> viewPath, uint32_t index) const override;
        virtual DataViewActionResult actionArrayRemoveElement(StringView<char> viewPath, uint32_t index) const override;
        virtual DataViewActionResult actionArrayNewElement(StringView<char> viewPath) const override;
        virtual DataViewActionResult actionObjectClear(StringView<char> viewPath) const  override;
        virtual DataViewActionResult actionObjectNew(StringView<char> viewPath, ClassType objectClass) const override;

        //--

        virtual DataViewResult readDefaultDataView(StringView<char> viewPath, void* targetData, Type targetType) const;
        virtual DataViewResult resetToDefaultValue(StringView<char> viewPath, void* targetData, Type targetType) const; // called instead of write value if the value is "default value"
        virtual bool checkIfCurrentlyADefaultValue(StringView<char> viewPath) const;

    private:
        ObjectPtr m_object;
        uint32_t m_objectObserverIndex = 0;

        virtual void onObjectChangedEvent(StringID eventID, const IObject* eventObject, StringView<char> eventPath, const rtti::DataHolder& eventData) override final;
    };

    //---

} // base