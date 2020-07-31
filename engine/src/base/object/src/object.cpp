/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#include "build.h"
#include "object.h"
#include "objectTemplate.h"
#include "objectGlobalRegistry.h"

#include "rttiTypeSystem.h"
#include "rttiNativeClassType.h"
#include "rttiTypeSystem.h"
#include "rttiNativeClassType.h"
#include "rttiMetadata.h"
#include "rttiDataView.h"
#include "rttiProperty.h"

#include "globalEventDispatch.h"

#include "dataView.h"
#include "dataViewNative.h"

#include "base/xml/include/xmlUtils.h"
#include "base/xml/include/xmlWrappers.h"

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
            m_eventKey = MakeUniqueEventKey();
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
        auto parent = m_parent.unsafe();
        while (parent)
        {
            if (parent == parentObject)
                return true;
            parent = parent->m_parent.unsafe();
        }

        return false;
    }

    IObject* IObject::findParent(ClassType parentObjectClass) const
    {
        auto* parent = m_parent.unsafe();
        while (parent)
        {
            if (parent->is(parentObjectClass))
                return parent;
            parent = parent->m_parent.unsafe();
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
    
    void IObject::print(IFormatStream& f) const
    {
        f.appendf("{} ID:{} 0x{}", cls()->name(), id(), Hex(this));
    }

    //--

    void IObject::onReadBinary(stream::OpcodeReader& reader)
    {
        rtti::TypeSerializationContext typeContext;
        typeContext.directObjectContext = this;
        typeContext.parentObjectContext = this;
        cls()->readBinary(typeContext, reader, this);
    }

    void IObject::onWriteBinary(stream::OpcodeWriter& writer) const
    {
        rtti::TypeSerializationContext typeContext;
        typeContext.directObjectContext = (IObject*)this;
        typeContext.parentObjectContext = (IObject*)this;
        cls()->writeBinary(typeContext, writer, this, cls()->defaultObject());
    }

    //--

    void IObject::writeXML(xml::Node& node) const
    {
        rtti::TypeSerializationContext typeContext;
        typeContext.directObjectContext = (IObject*)this;
        typeContext.parentObjectContext = (IObject*)this;
        cls()->writeXML(typeContext, node, this, cls()->defaultObject());
    }

    void IObject::readXML(const xml::Node& node)
    {
        rtti::TypeSerializationContext typeContext;
        typeContext.directObjectContext = this;
        typeContext.parentObjectContext = this;
        cls()->readXML(typeContext, node, this);
    }

    //--

    bool IObject::onPropertyShouldSave(const rtti::Property* prop) const
    {
        // skip transient properties
        if (prop->flags().test(rtti::PropertyFlagBit::Transient))
            return false;

        // compare the property value with default, do not save if the same
        auto propData = prop->offsetPtr(this);
        auto propDefaultData = prop->offsetPtr(cls()->defaultObject());
        if (prop->type()->compare(propData, propDefaultData))
            return false;

        // TODO: additional checks ?

        // we can save this property
        return true;
    }

    bool IObject::onPropertyShouldLoad(const rtti::Property* prop)
    {
        return true;
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

    DataViewResult IObject::describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const
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

    DataViewResult IObject::readDataView(StringView<char> viewPath, void* targetData, Type targetType) const
    {
        if (viewPath == "__cls")
        {
            // TODO
        }

        return cls()->readDataView(viewPath, this, targetData, targetType.ptr());
    }

    DataViewResult IObject::writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType)
    {
        if (!onPropertyChanging(viewPath, sourceData, sourceType))
            return DataViewResultCode::ErrorIllegalOperation;

        if (auto ret = HasError(cls()->writeDataView(viewPath, this, sourceData, sourceType.ptr())))
            return ret;

        onPropertyChanged(viewPath);
        return DataViewResultCode::OK;
    }

    //--

    DataViewPtr IObject::createDataView() const
    {
        return CreateSharedPtr<DataViewNative>(const_cast<IObject*>(this));
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

        static const auto stringType = RTTI::GetInstance().findType("StringBuf"_id);
        StringBuf stringPath(path);

        postEvent(EVENT_OBJECT_PROPERTY_CHANGED, &stringPath, stringType);
    }

    bool IObject::onPropertyFilter(StringID propertyName) const
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

    void IObject::postEvent(StringID eventID, const void* data, Type dataType)
    {
        DispatchGlobalEvent(m_eventKey, eventID, this, data, dataType);
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
            IObjectTemplate::RegisterType(typeSystem);
            rtti::IMetadata::RegisterType(typeSystem);
            rtti::ShortTypeNameMetadata::RegisterType(typeSystem);
        }

    } // rtti

    //--

    xml::DocumentPtr SaveObjectToXML(const IObject* object, StringView<char> rootNodeName /*= "object"*/)
    {
        auto ret = xml::CreateDocument(rootNodeName);
        if (ret && object)
        {
            auto rootNode = xml::Node(ret);
            rootNode.writeAttribute("class", object->cls().name().view());
            object->writeXML(rootNode);
        }

        return ret;
    }

    ObjectPtr LoadObjectFromXML(const xml::IDocument* doc, SpecificClassType<IObject> expectedClass)
    {
        if (doc)
        {
            if (auto rootNode = xml::Node(doc))
                return LoadObjectFromXML(rootNode, expectedClass);
        }

        return nullptr;
    }

    ObjectPtr LoadObjectFromXML(const xml::Node& node, SpecificClassType<IObject> expectedClass)
    {
        if (const auto className = node.attribute("class"))
        {
            const auto classType = RTTI::GetInstance().findClass(StringID::Find(className));
            if (classType && !classType->isAbstract())
            {
                if (!expectedClass || classType->is(expectedClass))
                {
                    if (auto obj = classType.create<IObject>())
                    {
                        obj->readXML(node);
                        return obj;
                    }
                }
            }
        }

        return nullptr;
    }

    //--

} // base