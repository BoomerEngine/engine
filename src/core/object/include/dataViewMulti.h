/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: view #]
***/

#pragma once

#include "dataView.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// data view that wraps multiple data views so multiple objects can be edited at the same time
class CORE_OBJECT_API DataViewMulti : public IDataView
{
public:
    DataViewMulti(IDataView** views, uint32_t count);
    virtual ~DataViewMulti();

    //---

    /// Get metadata for view - describe what we will find here: flags, list of members, size of array, etc
    virtual DataViewResult describeDataView(StringView viewPath, DataViewInfo& outInfo) const override final;

    /// Read data from memory
    virtual DataViewResult readDataView(StringView viewPath, void* targetData, Type targetType) const override final;

    /// Write data to memory
    virtual DataViewResult writeDataView(StringView viewPath, const void* sourceData, Type sourceType) const override final;

    //---

    // create action that writes new value to this view
    virtual DataViewActionResult actionValueWrite(StringView viewPath, const void* sourceData, Type sourceType) const override final;

    // create action that resets current value to base value
    virtual DataViewActionResult actionValueReset(StringView viewPath) const override final;

    // create action that clear array
    virtual DataViewActionResult actionArrayClear(StringView viewPath) const override final;

    // create action that adds one element to array
    virtual DataViewActionResult actionArrayInsertElement(StringView viewPath, uint32_t index) const override final;

    // create action that removes one element from array
    virtual DataViewActionResult actionArrayRemoveElement(StringView viewPath, uint32_t index) const override final;

    // create action that adds one new element to the array at the end of the array
    virtual DataViewActionResult actionArrayNewElement(StringView viewPath) const override final;

    // create action that clears inlined object
    virtual DataViewActionResult actionObjectClear(StringView viewPath) const override final;

    // create action that creates/replaces inlined object with an object of different class
    virtual DataViewActionResult actionObjectNew(StringView viewPath, ClassType objectClass) const override final;

    //--

protected:
    DataViewPtr m_mainView;
    Array<DataViewPtr> m_extraViews;
};
    
///---

END_BOOMER_NAMESPACE()
