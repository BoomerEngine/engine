/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: view #]
***/

#include "build.h"
#include "dataView.h"
#include "base/object/include/rttiDataView.h"
#include "rttiClassType.h"

namespace base
{
    //----

    DataProxy::DataProxy(IDataView* initialView)
    {
        // create the always existing "root path" that points to the root object in the data view itself, this path always exists
        m_rootPath = m_pathPool.create();
        m_paths[StringBuf::EMPTY()] = m_rootPath;

        // add the initial view
        if (initialView)
            add(initialView);
    }

    DataProxy::~DataProxy()
    {
        clear();

        for (auto* path : m_paths.values())
        {
            auto* obs = path->observers;
            while (obs)
            {
                auto* next = obs->next;
                m_observerPool.free(obs);
                obs = next;
            }

            m_pathPool.free(path);
        }
    }

    void DataProxy::clear()
    {
        for (const auto& source : m_sources)
            source->detachObjectObserver(this);

        if (m_base)
            m_base->detachObjectObserver(this);

        m_sources.reset();
        m_base.reset();
    }

    void DataProxy::add(IDataView* view)
    {
        if (view && !m_sources.contains(view))
        {
            m_sources.pushBackUnique(AddRef(view));
            view->attachObjectObserver(this);
        }
    }

    void DataProxy::remove(IDataView* view)
    {
        if (view && m_sources.contains(view))
        {
            view->detachObjectObserver(this);
            m_sources.remove(view);
        }
    }

    bool DataProxy::read(int objectIndex, StringView<char> path, void* targetData, Type targetType) const
    {
        if (objectIndex >= 0 && objectIndex <= m_sources.lastValidIndex())
            return m_sources[objectIndex]->readDataView(path, targetData, targetType);
        return false;
    }

    bool DataProxy::write(int objectIndex, StringView<char> path, const void* sourceData, Type sourceType) const
    {
        if (objectIndex >= 0 && objectIndex <= m_sources.lastValidIndex())
            return m_sources[objectIndex]->writeDataView(path, sourceData, sourceType);
        return false;
    }

    bool DataProxy::describe(int objectIndex, StringView<char> path, rtti::DataViewInfo& outInfo) const
    {
        if (objectIndex >= 0 && objectIndex <= m_sources.lastValidIndex())
            return m_sources[objectIndex]->describeDataView(path, outInfo);
        return false;
    }

    //----

    void* DataProxy::registerObserver(StringView<char> path, IDataProxyObserver* observer)
    {
        if (auto* pathEntry = createPathEntry(path))
        {
            auto entry = m_observerPool.create();
            entry->path = pathEntry;
            entry->prev = nullptr;
            entry->next = pathEntry->observers;
            entry->observer = observer;
            pathEntry->observers = entry;

            return entry;
        }
        else
        {
            return nullptr;
        }
    }

    void DataProxy::unregisterObserver(void* token)
    {
        if (auto ob = (Observer*)token)
        {
            ob->observer = nullptr;

            if (m_callbackDepth == 0)
                unlinkAndReleaseObserver(ob);
            else
                m_observerersReleasedDuringCallback.pushBack(ob);
        }
    }

    void DataProxy::unlinkAndReleaseObserver(Observer * ob)
    {
        DEBUG_CHECK(ob->observer == nullptr);
        DEBUG_CHECK(ob->path != nullptr);

        if (ob->next)
            ob->next->prev = ob->prev;
        if (ob->prev)
            ob->prev->next = ob->next;
        else
            ob->path->observers = ob->next;
        ob->next = nullptr;
        ob->prev = nullptr;
        ob->path = nullptr;

        m_observerPool.free(ob);
    }

    DataProxy::Path* DataProxy::createPathEntryMemberInternal(Path* parent, StringView<char> propertyName)
    {
        // assemble full path
        base::StringBuf fullPath;
        if (parent == m_rootPath)
            fullPath = base::StringBuf(propertyName);
        else 
            fullPath = base::TempString("{}.{}", parent->path, propertyName);

        // find existing
        Path* ret = nullptr;
        if (!m_paths.find(fullPath, ret))
        {
            ret = m_pathPool.create();
            ret->path = fullPath;
            ret->parent = parent;
            m_paths[fullPath] = ret;
        }

        return ret;
    }

    DataProxy::Path* DataProxy::createPathEntryIndexInternal(Path* parent, uint32_t arrayIndex)
    {
        // assemble full path
        base::StringBuf fullPath = base::TempString("{}[{}]", parent->path, arrayIndex);

        // find existing
        Path* ret = nullptr;
        if (!m_paths.find(fullPath, ret))
        {
            ret = m_pathPool.create();
            ret->path = fullPath;
            ret->parent = parent;
            m_paths[fullPath] = ret;
        }

        return ret;
    }
    
    DataProxy::Path* DataProxy::createPathEntry(StringView<char> path)
    {
        // for most of the time path entry will exist
        Path* ret = nullptr;
        if (m_paths.find(path, ret))
            return ret;

        // dissect path, start with root
        auto* curPath = m_rootPath;
        auto curPathString = path;
        while (!curPathString.empty())
        {
            uint32_t arrayIndex = 0;
            StringView<char> propName;
            if (rtti::ParsePropertyName(curPathString, propName))
            {
                curPath = createPathEntryMemberInternal(curPath, propName);
            }
            else if (rtti::ParseArrayIndex(curPathString, arrayIndex))
            {
                curPath = createPathEntryIndexInternal(curPath, arrayIndex);
            }
            else
            {
                TRACE_WARNING("Invalid data view path: '{}'", path);
                return nullptr;
            }
        }

        // return the current path entry after dissection - this is the place we have arrived
        DEBUG_CHECK(curPath->path == path);
        return curPath;
    }

    void DataProxy::dispatchEvent(StringView<char> eventPath)
    {
        ++m_callbackDepth;

        bool parentEvent = false;
        StringView<char> childPath;
        do
        {
            Path* ret = nullptr;
            if (m_paths.find(eventPath, ret))
            {
                auto* cur = ret->observers;
                while (cur)
                {
                    if (cur->observer)
                        cur->observer->dataProxyValueChanged(eventPath, parentEvent);
                    cur = cur->next;
                }
            }

            parentEvent = true;
        }
        while (rtti::ExtractParentPath(eventPath, childPath));

        if (0 == --m_callbackDepth)
        {
            for (auto* cur : m_observerersReleasedDuringCallback)
                unlinkAndReleaseObserver(cur);

            m_observerersReleasedDuringCallback.reset();
        }
    }

    void DataProxy::dispatchFullStructureChange()
    {
        ++m_callbackDepth;

        StringView<char> childPath;
        Path* ret = nullptr;
        if (m_paths.find("", ret))
        {
            auto* cur = ret->observers;
            while (cur)
            {
                if (cur->observer)
                    cur->observer->dataProxyFullObjectChange();
                cur = cur->next;
            }
        }

        if (0 == --m_callbackDepth)
        {
            for (auto* cur : m_observerersReleasedDuringCallback)
                unlinkAndReleaseObserver(cur);

            m_observerersReleasedDuringCallback.reset();
        }
    }

    void DataProxy::onObjectChangedEvent(StringID eventID, const IObject* eventObject, StringView<char> eventPath, const rtti::DataHolder& eventData)
    {
        if (eventID == "OnPropertyChanged"_id)
            dispatchEvent(eventPath);
        else if (eventID == "OnFullStructureChange"_id && eventPath == "")
            dispatchFullStructureChange();
    }

    //----

    IDataProxyObserver::~IDataProxyObserver()
    {}

    //----

    IDataView::~IDataView()
    {}

    void IDataView::attachObjectObserver(IObjectObserver* observer)
    {}

    void IDataView::detachObjectObserver(IObjectObserver* observer)
    {}

    IDataView* IDataView::base() const
    {
        return nullptr;
    }

    DataProxyPtr IDataView::makeProxy() const
    {
        return CreateSharedPtr<DataProxy>(const_cast<IDataView*>(this));
    }

    IObject* IDataView::parentObject() const
    {
        return nullptr;
    }

    bool IDataView::Compare(const IDataView& a, const IDataView& b, StringView<char> viewPath)
    {
        StringBuilder txt;

        rtti::DataViewInfo infoA, infoB;
        if (!a.describeDataView(viewPath, infoA))
            return true;
        if (!b.describeDataView(viewPath, infoB))
            return false;

        if (infoA.dataType != infoB.dataType)
            return true;

        if (infoA.flags.test(rtti::DataViewInfoFlagBit::LikeArray) || infoB.flags.test(rtti::DataViewInfoFlagBit::LikeArray))
        {
            if (!infoA.flags.test(rtti::DataViewInfoFlagBit::LikeArray) || !infoB.flags.test(rtti::DataViewInfoFlagBit::LikeArray))
                return false;

            if (infoA.arraySize != infoB.arraySize)
                return false;

            for (uint32_t i = 0; i < infoB.arraySize; ++i)
            {
                txt.clear();
                txt << viewPath;
                txt.appendf("[{}]", i);

                if (!Compare(a, b, txt.view()))
                    return false;
            }
        }
        else if (infoA.flags.test(rtti::DataViewInfoFlagBit::LikeStruct) || infoB.flags.test(rtti::DataViewInfoFlagBit::LikeStruct))
        {
            if (!infoA.flags.test(rtti::DataViewInfoFlagBit::LikeStruct) || !infoB.flags.test(rtti::DataViewInfoFlagBit::LikeStruct))
                return false;

            infoA.requestFlags |= rtti::DataViewRequestFlagBit::MemberList;
            a.describeDataView(viewPath, infoA);

            for (const auto& member : infoA.members)
            {
                txt.clear();
                txt << viewPath;
                if (!viewPath.empty())
                    txt << ".";
                txt << member.name;

                if (!Compare(a, b, txt.view()))
                    return false;
            }
        }
        /*else if (infoA.dataType && infoA.dataType.metaType() == rtti::MetaType::StrongHandle)
        {

        }
        else if (infoA.dataType && infoA.dataType.metaType() == rtti::MetaType::WeakHandle)
        {
            return false;
        }*/
        else if (infoA.dataType)
        {
            rtti::DataHolder valueA(infoA.dataType);
            if (!a.readDataView(viewPath, valueA.data(), valueA.type()))
                return false;

            rtti::DataHolder valueB(infoB.dataType);
            if (!b.readDataView(viewPath, valueB.data(), valueB.type()))
                return false;

            return infoA.dataType->compare(valueA.data(), valueB.data());
        }

        return true;
    }

    bool IDataView::Copy(const IDataView& src, const IDataView& dest, StringView<char> viewPath)
    {
        StringBuilder txt;

        rtti::DataViewInfo infoA, infoB;
        if (!src.describeDataView(viewPath, infoA))
            return false;
        if (!dest.describeDataView(viewPath, infoB))
            return false;

        if (infoA.flags.test(rtti::DataViewInfoFlagBit::LikeArray) || infoB.flags.test(rtti::DataViewInfoFlagBit::LikeArray))
        {
            if (!infoA.flags.test(rtti::DataViewInfoFlagBit::LikeArray) || !infoB.flags.test(rtti::DataViewInfoFlagBit::LikeArray))
                return false;

            if (infoA.arraySize != infoB.arraySize)
            {
                rtti::DataViewCommand cmd;
                cmd.command = "resize"_id;
                cmd.arg0 = infoA.arraySize;
                if (!dest.writeDataView(viewPath, &cmd, rtti::DataViewCommand::GetStaticClass()))
                    return false;
            }

            for (uint32_t i = 0; i < infoA.arraySize; ++i)
            {
                txt.clear();
                txt << viewPath;
                txt.appendf("[{}]", i);

                if (!Copy(src, dest, txt.view()))
                    return false;
            }
        }
        else if (infoA.dataType.metaType() == rtti::MetaType::StrongHandle)
        {
            ObjectPtr obj;
            if (!src.readDataView(viewPath, &obj, infoA.dataType))
                return false;

            if (infoB.dataType.metaType() != rtti::MetaType::StrongHandle)
                return false;

            ClassType targetClass = infoB.dataType.innerType().toClass();
            if (obj && !obj->is(targetClass))
                obj = nullptr;

            auto parent = dest.parentObject();
            if (!parent)
                return false;

            if (obj)
                obj = obj->clone(parent);

            return dest.writeDataView(viewPath, &obj, infoB.dataType);
        }
        else if (infoA.flags.test(rtti::DataViewInfoFlagBit::LikeStruct) || infoB.flags.test(rtti::DataViewInfoFlagBit::LikeStruct))
        {
            if (!infoA.flags.test(rtti::DataViewInfoFlagBit::LikeStruct) || !infoB.flags.test(rtti::DataViewInfoFlagBit::LikeStruct))
                return false;

            infoA.requestFlags |= rtti::DataViewRequestFlagBit::MemberList;
            src.describeDataView(viewPath, infoA);

            for (const auto& member : infoA.members)
            {
                txt.clear();
                txt << viewPath;
                if (!viewPath.empty())
                    txt << ".";
                txt << member.name;

                if (!Copy(src, dest, txt.view()))
                    return false;
            }
        }       
        else if (infoA.dataType.metaType() == rtti::MetaType::WeakHandle)
        {
            return false;
        }
        else if (infoA.dataType)
        {
            /*if (infoA.dataType != infoB.dataType)
                return false;*/

            rtti::DataHolder value(infoA.dataType);
            if (!src.readDataView(viewPath, value.data(), value.type()))
                return false;

            return dest.writeDataView(viewPath, value.data(), value.type());
        }

        return true;
    }

    //----

    DataViewNative::DataViewNative(void* data, Type type, IReferencable* owner /*= nullptr*/)
        : m_data(data)
        , m_type(type)
        , m_owner(owner)
    {
        if (m_owner)
            m_owner->addRef();
    }

    DataViewNative::DataViewNative(IObject* obj)
        : m_data(obj)
        , m_type(obj->cls())
        , m_object(obj)
    {
        if (m_object)
            m_object->addRef();

        m_base = base::CreateSharedPtr<DataViewBaseClass>(obj->cls());
    }

    DataViewNative::~DataViewNative()
    {
        if (m_owner)
        {
            m_owner->releaseRef();
            m_owner = nullptr;
        }

        if (m_object)
        {
            m_object->releaseRef();
            m_object = nullptr;
        }
    }

    bool DataViewNative::describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const
    {
        if (m_object)
            return m_object->describeDataView(viewPath, outInfo);
        else
            return m_type->describeDataView(viewPath, m_data, outInfo);
    }

    IObject* DataViewNative::parentObject() const
    {
        return m_object;
    }

    IDataView* DataViewNative::base() const
    {
        return m_base;
    }

    bool DataViewNative::readDataView(StringView<char> viewPath, void* targetData, Type targetType) const
    {
        if (m_object)
            return m_object->readDataView(this, "", viewPath, targetData, targetType);
        else 
            return m_type->readDataView(nullptr, this, "", viewPath, m_data, targetData, targetType);
    }

    bool DataViewNative::writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType) const
    {
        if (m_object)
            return m_object->writeDataView(this, "", viewPath, sourceData, sourceType);
        else
            return m_type->writeDataView(nullptr, this, "", viewPath, m_data, sourceData, sourceType);
    }

    void DataViewNative::attachObjectObserver(IObjectObserver* observer)
    {
        if (!m_observers.contains(observer))
        {
            if (auto obj = m_object)
            {
                if (auto id = ObjectObserver::RegisterObserver(m_object, observer))
                {
                    m_observers[observer] = id;
                }
            }
        }
    }

    void DataViewNative::detachObjectObserver(IObjectObserver* observer)
    {
        uint32_t id = 0;
        if (m_observers.find(observer, id))
        {
            ObjectObserver::UnregisterObserver(id, observer);
            m_observers.remove(observer);
        }
    }

    //----

    DataViewBaseClass::DataViewBaseClass(ClassType type)
        : m_baseClass(type)
    {
        m_baseObjectData = m_baseClass->defaultObject();
    }

    bool DataViewBaseClass::describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const
    {
        return m_baseClass->describeDataView(viewPath, m_baseObjectData, outInfo);
    }

    bool DataViewBaseClass::readDataView(StringView<char> viewPath, void* targetData, Type targetType) const
    {
        return m_baseClass->readDataView(nullptr, this, "", viewPath, m_baseObjectData, targetData, targetType);
    }

    bool DataViewBaseClass::writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType) const
    {
        return nullptr;
    }

    void DataViewBaseClass::attachObjectObserver(IObjectObserver* observer)
    {
        // base classes are not changed
    }

    void DataViewBaseClass::detachObjectObserver(IObjectObserver* observer)
    {
        // base classes are not changed
    }

    //----

} // editor

