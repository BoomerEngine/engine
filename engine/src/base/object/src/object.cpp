/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#include "build.h"
#include "object.h"
#include "objectGlobalRegistry.h"
#include "objectObserverEventDispatcher.h"

#include "rttiTypeSystem.h"
#include "rttiNativeClassType.h"
#include "rttiTypeSystem.h"
#include "rttiNativeClassType.h"
#include "rttiMetadata.h"
#include "rttiDataView.h"

#include "base/object/include/serializationSaver.h"
#include "base/object/include/serializationLoader.h"
#include "base/object/include/memoryReader.h"
#include "base/object/include/memoryWriter.h"
#include "base/object/include/rttiProperty.h"

#include "dataView.h"

namespace base
{
    //--

    std::atomic<uint32_t> GObjectIDAllocator = 0;

    ObjectID IObject::AllocUniqueObjectID()
    {
        return ++GObjectIDAllocator;
    }

    //--

    static std::function<ObjectPtr(const IObject*, const IObject*, res::IResourceLoader * loader, SpecificClassType<IObject>)> GCloneFunction;
    static std::function<Buffer(const IObject*)> GSerializeFunction;
    static std::function<ObjectPtr(const void* data, uint32_t size, res::IResourceLoader * loader, SpecificClassType<IObject> mutatedClass)> GDeserializeFunction;

    ObjectPtr IObject::clone(const IObject* newParent, res::IResourceLoader* loader, SpecificClassType<IObject> mutatedObjectClass) const
    {
        return GCloneFunction(this, newParent, loader, mutatedObjectClass);
    }

    Buffer IObject::toBuffer() const
    {
        return GSerializeFunction(this);
    }

    ObjectPtr IObject::FromBuffer(const void* data, uint32_t size, res::IResourceLoader* loader /*= nullptr*/, SpecificClassType<IObject> mutatedClass /*= nullptr*/)
    {
        return GDeserializeFunction(data, size, loader, mutatedClass);
    }

    //--

    void IObject::RegisterCloneFunction(const std::function<ObjectPtr(const IObject*, const IObject*, res::IResourceLoader * loader, SpecificClassType<IObject>)>& func)
    {
        GCloneFunction = func;
    }

    void IObject::RegisterSerializeFunction(const std::function<Buffer(const IObject*)>& func)
    {
        GSerializeFunction = func;
    }

    void IObject::RegisterDeserializeFunction(const std::function<ObjectPtr(const void* data, uint32_t size, res::IResourceLoader * loader, SpecificClassType<IObject> mutatedClass)>& func)
    {
        GDeserializeFunction = func;
    }

    //--

    IObject::IObject()
        : IReferencable(1)
    {
        if (!base::IsDefaultObjectCreation())
        {
            m_id = AllocUniqueObjectID();
            ObjectGlobalRegistry::GetInstance().registerObject(m_id, this);
        }
    }

    IObject::~IObject()
    {
        if (m_id)
        {
            ObjectGlobalRegistry::GetInstance().unregisterObject(m_id, this);
            m_id = 0;
        }
    }

    void IObject::parent(const IObject* parentObject)
    {
        m_parent = nullptr;

        if (parentObject)
        {
            DEBUG_CHECK_EX(!parentObject->hasParent(this), "Recursive parenting");
            if (!parentObject->hasParent(this))
                m_parent = const_cast<IObject*>(parentObject);
        }
    }

    bool IObject::hasParent(const IObject* parentObject) const
    {
        auto parent = m_parent;
        while (parent)
        {
            if (parent == parentObject)
                return true;
            parent = parent->m_parent;
        }

        return false;
    }

    IObject* IObject::findParent(ClassType parentObjectClass) const
    {
        auto* parent = m_parent;
        while (parent)
        {
            if (parent->is(parentObjectClass))
                return parent;
            parent = parent->m_parent;
        }

        return nullptr;
    }

    bool IObject::is(ClassType objectClass) const
    {
        return cls().is(objectClass);
    }

    void IObject::onPreSave() const
    {
    }

    void IObject::onPostLoad()
    {
    }

    const void* IObject::defaultObject() const
    {
        return cls()->defaultObject();
    }

    bool IObject::onReadBinary(stream::IBinaryReader& reader)
    {
        rtti::TypeSerializationContext typeContext;
        return cls()->readBinary(typeContext, reader, this);
    }

    bool IObject::onWriteBinary(stream::IBinaryWriter& writer) const
    {
        rtti::TypeSerializationContext typeContext;
        return cls()->writeBinary(typeContext, writer, this, defaultObject());
    }

    bool IObject::onReadText(stream::ITextReader& reader)
    {
        rtti::TypeSerializationContext typeContext;
        return cls()->readText(typeContext, reader, this);
    }

    bool IObject::onWriteText(stream::ITextWriter& writer) const
    {
        rtti::TypeSerializationContext typeContext;
        return cls()->writeText(typeContext, writer, this, defaultObject());
    }

    bool IObject::onPropertyMissing(StringID propertyName, Type originalType, const void* originalData)
    {
        return false;
    }

    bool IObject::onPropertyTypeChanged(StringID propertyName, Type originalDataType, const void* originalData, Type currentType, void* currentData)
    {
        return false;
    }

    //--

    void IObject::markModified()
    {
        auto parent = this->parent();
        if (parent)
            parent->markModified();
    }

    //--

    SpecificClassType<IObject> IObject::GetStaticClass()
    {
        static ClassType objectType = RTTI::GetInstance().findClass("base::IObject"_id);
        return SpecificClassType<IObject>(*objectType.ptr());
    }

    ClassType IObject::cls() const
    {
        return nativeClass();
    }

    //--

    bool IObject::describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const
    {
        if (viewPath.empty())
        {
            outInfo.flags |= rtti::DataViewInfoFlagBit::Object;
            outInfo.flags |= rtti::DataViewInfoFlagBit::LikeStruct;

            if (outInfo.requestFlags.test(rtti::DataViewRequestFlagBit::ObjectInfo))
            {
                outInfo.objectClass = cls().ptr(); // get DYNAMIC class
                outInfo.objectPtr = this;
            }
        }

        return cls()->describeDataView(viewPath, this, outInfo);
    }

    bool IObject::readDataView(const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, void* targetData, Type targetType) const
    {
        return cls()->readDataView((IObject*)this, rootView, rootViewPath, viewPath, this, targetData, targetType.ptr());
    }

    bool IObject::writeDataView(const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, const void* sourceData, Type sourceType)
    {
        if (!onPropertyChanging(viewPath, sourceData, sourceType))
            return false;

        if (!cls()->writeDataView(this, rootView, rootViewPath, viewPath, this, sourceData, sourceType.ptr()))
            return false;

        onPropertyChanged(viewPath);
        return true;
    }

    //--

    DataViewPtr IObject::createDataView() const
    {
        return CreateSharedPtr<DataViewNative>(const_cast<IObject*>(this));
    }

    DataProxyPtr IObject::createDataProxy() const
    {
        if (auto view = createDataView())
            return view->makeProxy();
        return nullptr;
    }

    //--

    bool IObject::onPropertyChanging(StringView<char> path, const void* newData, Type newDataType) const
    {
        return true;
    }

    void IObject::onPropertyChanged(StringView<char> path)
    {
        markModified();
        TRACE_INFO("OnPropertyChanged prop '{}', this 0x{}", path, Hex((uint64_t)this));
        postEvent("OnPropertyChanged"_id, path);
    }

    bool IObject::onPropertyFilter(StringView<char> propertyName) const
    {
        return true;
    }

    //---

    bool IObject::onResourceReloading(res::IResource* currentResource, res::IResource* newResource)
    {
        base::InplaceArray<base::StringID, 16> affectedProperties;
        if (!cls()->patchResourceReferences(this, currentResource, newResource, &affectedProperties))
            return false;

        for (const auto propName : affectedProperties)
            onPropertyChanged(propName.view());

        return true;
    }

    void IObject::onResourceReloadFinished(res::IResource* currentResource, res::IResource* newResource)
    {

    }

    //---

    void IObject::postEvent(StringID eventID, StringView<char> eventPath /*= ""*/, rtti::DataHolder eventData /*= DataHolder*/, bool alwaysExecuteLater /*= false*/)
    {
        ObjectObserverEventDispatcher::GetInstance().postEvent(eventID, m_id, this, eventPath, eventData);
    }

    //--

    void IObject::RegisterType(rtti::TypeSystem& typeSystem)
    {
        base::Type ret = MemNew(rtti::NativeClass, "base::IObject", sizeof(IObject), alignof(IObject), typeid(IObject).hash_code());
        typeSystem.registerType(ret);
    }

    //--

    namespace rtti
    {

        void RegisterObjectTypes(rtti::TypeSystem &typeSystem)
        {
            IObject::RegisterType(typeSystem);
            rtti::IMetadata::RegisterType(typeSystem);
            rtti::ShortTypeNameMetadata::RegisterType(typeSystem);
            rtti::DataViewCommand::RegisterType(typeSystem);
            rtti::DataViewBaseValue::RegisterType(typeSystem);
        }

    } // rtti

    //--

} // base