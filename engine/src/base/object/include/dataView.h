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

namespace base
{

    //---

    /// source of data for the data view
    class BASE_OBJECT_API IDataView : public IReferencable
    {
    public:
        virtual ~IDataView();

        /// Get metadata for view - describe what we will find here: flags, list of members, size of array, etc
        virtual bool describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const = 0;

        /// Read data from memory
        virtual bool readDataView(StringView<char> viewPath, void* targetData, Type targetType) const = 0;

        /// Write data to memory
        virtual bool writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType) const = 0;

        /// Attach custom object observer
        virtual void attachObjectObserver(IObjectObserver* observer);

        /// Attach custom object observer
        virtual void detachObjectObserver(IObjectObserver* observer);

        /// Get parent object
        virtual IObject* parentObject() const;

        /// Get base view
        virtual IDataView* base() const;

        //--

        // create data proxy with just this view
        DataProxyPtr makeProxy() const;

        //--

        // compare data views
        static bool Compare(const IDataView& a, const IDataView& b, StringView<char> viewPath);

        // copy data from one view to other view
        static bool Copy(const IDataView& src, const IDataView& dest, StringView<char> viewPath);
    };

    //---

    /// observer of data properties changes
    class BASE_OBJECT_API IDataProxyObserver : public NoCopy
    {
    public:
        virtual ~IDataProxyObserver();

        /// full object change
        virtual void dataProxyFullObjectChange() {};

        /// observed value (or parent or child) changed, passes full path to changed
        virtual void dataProxyValueChanged(StringView<char> fullPath, bool parentNotification) {};
    };

    //---

    /// context of operation for the data views
    class BASE_OBJECT_API DataProxy : public IReferencable, public IObjectObserver
    {
    public:
        DataProxy(IDataView* initialView = nullptr);
        ~DataProxy();

        //--

        // number of objects in the root view
        INLINE uint32_t size() const { return m_sources.size(); }

        // get n-th view
        INLINE const DataViewPtr& view(const uint32_t index) const { return m_sources[index]; }

        // empty ? no sources ?
        INLINE bool empty() const { return m_sources.empty(); }

        // remove all sources
        void clear();

        // add a data view to sources list
        void add(IDataView* view);

        // remove view from sources list
        void remove(IDataView* view);

        //--

        // read value for a single object
        bool read(int objectIndex, StringView<char> path, void* targetData, Type targetType) const;

        // write value for a single object
        bool write(int objectIndex, StringView<char> path, const void* sourceData, Type sourceType) const;

        // describe view of a single element
        bool describe(int objectIndex, StringView<char> path, rtti::DataViewInfo& outInfo) const;

        //--

        // register observer for given path
        void* registerObserver(StringView<char> path, IDataProxyObserver* observer); 

        // unregister previously registered observer
        void unregisterObserver(void* token);

        //--

    private:
        InplaceArray<DataViewPtr, 1> m_sources;
        DataViewPtr m_base = nullptr;

        struct Observer;

        struct Path
        {
            Path* parent = nullptr;
            Observer* observers = nullptr;
            StringBuf path;
        };

        struct Observer
        {      
            Path* path = nullptr;
            Observer* next = nullptr;
            Observer* prev = nullptr;
            IDataProxyObserver* observer = nullptr;
        };

        HashMap<StringBuf, Path*> m_paths;
        Path* m_rootPath = nullptr;

        mem::StructurePool<Path> m_pathPool;
        mem::StructurePool<Observer> m_observerPool;

        Array<Observer*> m_observerersReleasedDuringCallback;
        uint32_t m_callbackDepth;

        Path* createPathEntry(StringView<char> path);
        Path* createPathEntryMemberInternal(Path* parent, StringView<char> propertyName);
        Path* createPathEntryIndexInternal(Path* parent, uint32_t arrayIndex);

        void unlinkAndReleaseObserver(Observer* ob);
        void dispatchEvent(StringView<char> eventPath);
        void dispatchFullStructureChange();

        virtual void onObjectChangedEvent(StringID eventID, const IObject* eventObject, StringView<char> eventPath, const rtti::DataHolder& eventData) override;
    };

    ///---

    /// native data source 
    class BASE_OBJECT_API DataViewNative : public IDataView
    {
    public:
        DataViewNative(void* data, Type type, IReferencable* owner = nullptr);
        DataViewNative(IObject* obj);
        virtual ~DataViewNative();

        virtual bool describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const override;
        virtual bool readDataView(StringView<char> viewPath, void* targetData, Type targetType) const override;
        virtual bool writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType) const override;
        virtual void attachObjectObserver(IObjectObserver* observer) override;
        virtual void detachObjectObserver(IObjectObserver* observer) override;
        virtual IObject* parentObject() const override;
        virtual IDataView* base() const override;

    private:
        void* m_data = nullptr;
        Type m_type = nullptr;
        IReferencable* m_owner = nullptr;
        IObject* m_object = nullptr;

        HashMap<IObjectObserver*, uint32_t> m_observers;
        DataViewPtr m_base;
    };

    ///---

    /// base class data source 
    class BASE_OBJECT_API DataViewBaseClass : public IDataView
    {
    public:
        DataViewBaseClass(ClassType type);

        virtual bool describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const override;
        virtual bool readDataView(StringView<char> viewPath, void* targetData, Type targetType) const override;
        virtual bool writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType) const override;
        virtual void attachObjectObserver(IObjectObserver* observer) override;
        virtual void detachObjectObserver(IObjectObserver* observer) override;

    private:
        ClassType m_baseClass = nullptr;
        const void* m_baseObjectData = nullptr;
    };

    //---

} // base