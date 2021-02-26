/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#pragma once

#include "uiElement.h"
#include "core/object/include/dataView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///---

/// data boxes allows values to be edited
class ENGINE_UI_API IDataBox : public IElement, public IDataViewObserver
{
    RTTI_DECLARE_VIRTUAL_CLASS(IDataBox, IElement);

public:
    ///---

    /// get data we are bound to 
    INLINE const DataViewPtr& data() const { return m_data; }

    /// get the action history we will use to post changes to the data
    INLINE const ActionHistoryPtr& actionHistory() const { return m_actionHistory; }

    /// get path to data
    INLINE const StringBuf& path() const { return m_path; }

    /// is data read only ?
    INLINE bool readOnly() const { return m_readOnly; }

    ///---

    IDataBox();
    virtual ~IDataBox();

    // bind to data
    virtual void bindData(const DataViewPtr& data, const StringBuf& path, bool readOnly=false);

    // bind action history for optional undo of operations we are doing
    virtual void bindActionHistory(ActionHistory* ah);

    // enter edit mode
    virtual void enterEdit();

    // cancel edit
    virtual void cancelEdit();

    // handle change of data value outside of the data box, re-read the value
    virtual void handleValueChange();

    // should we expand children (in case the value box was created for compound object)
    virtual bool canExpandChildren() const;

    ///----

    // read value
    DataViewResult readValue(void* data, const Type dataType);

    // write new value, will create undo action if action history is provided
    DataViewResult writeValue(const void* data, const Type dataType);

    ///----

    // read value
    template< typename T >
    INLINE DataViewResult readValue(T& data) { return readValue(&data, reflection::GetTypeObject<T>()); }

    // write new value, will create undo action if action history is provided
    template< typename T >
    INLINE DataViewResult writeValue(const T& data) { return writeValue(&data, reflection::GetTypeObject<T>()); }

    ///----

    /// create data box to edit/visualize given type
    static DataBoxPtr CreateForType(const rtti::DataViewInfo& info);

private:
    DataViewPtr m_data;
    StringBuf m_path;
    bool m_readOnly;
    void* m_observerToken = nullptr;

    ActionHistoryPtr m_actionHistory;

protected:
    virtual void handlePropertyChanged(StringView fullPath, bool parentNotification) override;

    DataViewResult executeAction(const ActionPtr& action);
    DataViewResult executeAction(const DataViewActionResult& action);
};

///---

/// data box "factory"
class ENGINE_UI_API IDataBoxFactory : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_UI_OBJECTS);
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDataBoxFactory);

public:
    virtual ~IDataBoxFactory();

    /// create a data box
    virtual DataBoxPtr tryCreate(const rtti::DataViewInfo& info) const = 0;
};

///---

END_BOOMER_NAMESPACE_EX(ui)


