/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: view #]
***/

#pragma once

#include "core/containers/include/array.h"
#include "core/containers/include/inplaceArray.h"
#include "core/memory/include/structurePool.h"

#include "object.h"
#include "dataView.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// native data source 
class CORE_OBJECT_API DataViewNative : public IDataView
{
public:
    DataViewNative(IObject* obj, bool readOnly=false); // adds ref
    virtual ~DataViewNative();

    // get the object we edit
    INLINE const ObjectPtr& object() const { return m_object; }

    // IDataView
    virtual DataViewResult describeDataView(StringView viewPath, DataViewInfo& outInfo) const override;
    virtual DataViewResult readDataView(StringView viewPath, void* targetData, Type targetType) const override;
    virtual DataViewResult writeDataView(StringView viewPath, const void* sourceData, Type sourceType) const override;

    // IDataView - actions
    virtual DataViewActionResult actionValueWrite(StringView viewPath, const void* sourceData, Type sourceType) const override;
    virtual DataViewActionResult actionValueReset(StringView viewPath) const override;
    virtual DataViewActionResult actionArrayClear(StringView viewPath) const override;
    virtual DataViewActionResult actionArrayInsertElement(StringView viewPath, uint32_t index) const override;
    virtual DataViewActionResult actionArrayRemoveElement(StringView viewPath, uint32_t index) const override;
    virtual DataViewActionResult actionArrayNewElement(StringView viewPath) const override;
    virtual DataViewActionResult actionObjectClear(StringView viewPath) const  override;
    virtual DataViewActionResult actionObjectNew(StringView viewPath, ClassType objectClass) const override;

    //--

    virtual DataViewResult readDefaultDataView(StringView viewPath, void* targetData, Type targetType) const;
    virtual DataViewResult resetToDefaultValue(StringView viewPath, void* targetData, Type targetType) const; // called instead of write value if the value is "default value"
    virtual bool checkIfCurrentlyADefaultValue(StringView viewPath) const;

private:
    ObjectPtr m_object;
    GlobalEventTable m_events;
    bool m_readOnly = false;
};

//---

END_BOOMER_NAMESPACE()
