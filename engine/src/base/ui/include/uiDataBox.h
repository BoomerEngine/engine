/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#pragma once

#include "uiElement.h"
#include "base/object/include/dataView.h"

namespace ui
{

    ///---

    /// data boxes allows values to be edited
    class BASE_UI_API IDataBox : public IElement, public base::IDataViewObserver
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IDataBox, IElement);

    public:
        ///---

        /// get data we are bound to 
        INLINE const base::DataViewPtr& data() const { return m_data; }

        /// get the action history we will use to post changes to the data
        INLINE const base::ActionHistoryPtr& actionHistory() const { return m_actionHistory; }

        /// get path to data
        INLINE const base::StringBuf& path() const { return m_path; }

        /// is data read only ?
        INLINE bool readOnly() const { return m_readOnly; }

        ///---

        IDataBox();
        virtual ~IDataBox();

        // bind to data
        virtual void bindData(const base::DataViewPtr& data, const base::StringBuf& path, bool readOnly=false);

        // bind action history for optional undo of operations we are doing
        virtual void bindActionHistory(base::ActionHistory* ah);

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
        base::DataViewResult readValue(void* data, const base::Type dataType);

        // write new value, will create undo action if action history is provided
        base::DataViewResult writeValue(const void* data, const base::Type dataType);

        ///----

        // read value
        template< typename T >
        INLINE base::DataViewResult readValue(T& data) { return readValue(&data, base::reflection::GetTypeObject<T>()); }

        // write new value, will create undo action if action history is provided
        template< typename T >
        INLINE base::DataViewResult writeValue(const T& data) { return writeValue(&data, base::reflection::GetTypeObject<T>()); }

        ///----

        /// create data box to edit/visualize given type
        static DataBoxPtr CreateForType(const base::rtti::DataViewInfo& info);

    private:
        base::DataViewPtr m_data;
        base::StringBuf m_path;
        bool m_readOnly;
        void* m_observerToken = nullptr;

        base::ActionHistoryPtr m_actionHistory;

    protected:
        virtual void handlePropertyChanged(base::StringView<char> fullPath, bool parentNotification) override;

        base::DataViewResult executeAction(const base::ActionPtr& action);
        base::DataViewResult executeAction(const base::DataViewActionResult& action);
    };

    ///---

    /// data box "factory"
    class BASE_UI_API IDataBoxFactory : public base::NoCopy
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDataBoxFactory);

    public:
        virtual ~IDataBoxFactory();

        /// create a data box
        virtual DataBoxPtr tryCreate(const base::rtti::DataViewInfo& info) const = 0;
    };

    ///---

} // ui


