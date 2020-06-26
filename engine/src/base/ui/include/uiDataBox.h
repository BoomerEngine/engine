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
    class BASE_UI_API IDataBox : public IElement, public base::IDataProxyObserver
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IDataBox, IElement);

    public:
        ///---

        /// get parent data inspector
        INLINE const base::DataProxyPtr& data() const { return m_data; }

        /// get path to data
        INLINE const base::StringBuf& path() const { return m_path; }

        /// is data read only ?
        INLINE bool readOnly() const { return m_readOnly; }

        /// do we have a valid read (all values defined and same)
        INLINE bool readValid() const { return m_readValid; }

        ///---

        IDataBox();
        virtual ~IDataBox();

        // bind to data
        virtual void bind(const base::DataProxyPtr& data, const base::StringBuf& path, bool readOnly=false);

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
        bool readValue(void* data, const base::Type dataType);

        // write new value, will create undo action if action history is provided
        bool writeValue(const void* data, const base::Type dataType);

        ///----

        // read value
        template< typename T >
        INLINE bool readValue(T& data) { return readValue(&data, base::reflection::GetTypeObject<T>()); }

        // write new value, will create undo action if action history is provided
        template< typename T >
        INLINE bool writeValue(const T& data) { return writeValue(&data, base::reflection::GetTypeObject<T>()); }

        ///----

        /// create data box to edit/visualize given type
        static DataBoxPtr CreateForType(const base::rtti::DataViewInfo& info);

    private:
        base::DataProxyPtr m_data;
        base::StringBuf m_path;
        bool m_readOnly;
        bool m_readValid; // a valid value was read
        void* m_observerToken = nullptr;

    protected:
        /// observed value (or parent or child) changed, passes full path to changed
        virtual void dataProxyValueChanged(base::StringView<char> fullPath, bool parentNotification) override;
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


