/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: view #]
***/

#pragma once

#include "core/containers/include/array.h"
#include "core/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// observer of data view
class CORE_OBJECT_API IDataViewObserver : public NoCopy
{
public:
    virtual ~IDataViewObserver();

    /// full object change
    virtual void handleFullObjectChange() {};

    /// observed value (or parent or child) changed, passes full path to changed
    virtual void handlePropertyChanged(StringView fullPath, bool parentNotification) {};
};

//---

// result from data view action
struct CORE_OBJECT_API DataViewActionResult
{
    ActionPtr action;
    DataViewResult result;

    INLINE DataViewActionResult() {};
    INLINE DataViewActionResult(const DataViewActionResult& other) = default;
    INLINE DataViewActionResult& operator=(const DataViewActionResult& other) = default;

    INLINE DataViewActionResult(const ActionPtr& action_) : action(action_) {};
    INLINE DataViewActionResult(const DataViewResult& result_) : result(result_) {};
    INLINE DataViewActionResult(const DataViewErrorResult& result_) : result(result_.result) {};
    INLINE DataViewActionResult(DataViewResult&& result_) : result(std::move(result_)) {};
    INLINE DataViewActionResult(DataViewErrorResult&& result_) : result(std::move(result_.result)) {};

    INLINE operator bool() const { return action != nullptr; }

    void print(IFormatStream& f) const;
};

//---

/// abstract wrapper for a view of object's data
class CORE_OBJECT_API IDataView : public IReferencable
{
public:
    IDataView();
    virtual ~IDataView();

    //---

    /// Get metadata for view - describe what we will find here: flags, list of members, size of array, etc
    virtual DataViewResult describeDataView(StringView viewPath, DataViewInfo& outInfo) const = 0;

    /// Read data from memory
    virtual DataViewResult readDataView(StringView viewPath, void* targetData, Type targetType) const = 0;

    /// Write data to memory
    virtual DataViewResult writeDataView(StringView viewPath, const void* sourceData, Type sourceType) const = 0;

    //--

    /// Read data from memory - easier version using DataHolder (NOTE: it must be initialized :P)
    INLINE DataViewResult readDataViewSimple(StringView viewPath, DataHolder& outData) const { return readDataView(viewPath, outData.data(), outData.type()); }

    /// Write data to memory
    INLINE DataViewResult writeDataViewSimple(StringView viewPath, const DataHolder& newData) const { return writeDataView(viewPath, newData.data(), newData.type()); }

    //--

    /// Attach observer of particular place (property) in this view
    virtual void attachObserver(StringView path, IDataViewObserver* observer);

    /// Detach observer previously attached with attachObserver()
    virtual void detachObserver(StringView path, IDataViewObserver* observer);

    //---

    // create action that writes new value to this view
    virtual DataViewActionResult actionValueWrite(StringView viewPath, const void* sourceData, Type sourceType) const = 0;

    // create action that resets current value to base value
    virtual DataViewActionResult actionValueReset(StringView viewPath) const = 0;

    // create action that clear array
    virtual DataViewActionResult actionArrayClear(StringView viewPath) const = 0;

    // create action that adds one element to array
    virtual DataViewActionResult actionArrayInsertElement(StringView viewPath, uint32_t index) const = 0;

    // create action that removes one element from array
    virtual DataViewActionResult actionArrayRemoveElement(StringView viewPath, uint32_t index) const = 0;

    // create action that adds one new element to the array at the end of the array
    virtual DataViewActionResult actionArrayNewElement(StringView viewPath) const = 0;

    // create action that clears inlined object
    virtual DataViewActionResult actionObjectClear(StringView viewPath) const = 0;

    // create action that creates/replaces inlined object with an object of different class
    virtual DataViewActionResult actionObjectNew(StringView viewPath, ClassType objectClass) const = 0;

    //--

protected:
    struct Observer;

    struct Path : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_RTTI)

    public:
        Path* parent = nullptr;
        Array<IDataViewObserver*> observers;
    };

    HashMap<uint64_t, Path*> m_paths; // NOTE: owns the Path entry
    Path* m_rootPath = nullptr;

    uint32_t m_callbackDepth = 0;

    //--

    void dispatchPropertyChanged(StringView eventPath);
    void dispatchFullStructureChanged();

    Path* findPathEntry(StringView path);

    Path* createPathEntry(StringView path);
    Path* createPathEntryInternal(Path* parent, StringView fullPath);
};
    
///---

END_BOOMER_NAMESPACE()
