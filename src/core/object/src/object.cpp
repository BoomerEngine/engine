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

#include "core/xml/include/xmlUtils.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE()

//--

std::atomic<uint32_t> GObjectIDAllocator = 0;

ObjectID IObject::AllocUniqueObjectID()
{
    return ++GObjectIDAllocator;
}

ObjectPtr IObject::FindUniqueObjectById(ObjectID id)
{
    return ObjectGlobalRegistry::GetInstance().findObject(id);
}


//--

static std::function<ObjectPtr(const IObject*, const IObject*, res::ResourceLoader * loader, SpecificClassType<IObject>)> GCloneFunction;
static std::function<Buffer(const IObject*)> GSerializeFunction;
static std::function<ObjectPtr(const void* data, uint32_t size, res::ResourceLoader * loader, SpecificClassType<IObject> mutatedClass)> GDeserializeFunction;

ObjectPtr IObject::clone(const IObject* newParent, res::ResourceLoader* loader, SpecificClassType<IObject> mutatedObjectClass) const
{
    return GCloneFunction(this, newParent, loader, mutatedObjectClass);
}

Buffer IObject::toBuffer() const
{
    return GSerializeFunction(this);
}

ObjectPtr IObject::FromBuffer(const void* data, uint32_t size, res::ResourceLoader* loader /*= nullptr*/, SpecificClassType<IObject> mutatedClass /*= nullptr*/)
{
    return GDeserializeFunction(data, size, loader, mutatedClass);
}

//--

void IObject::RegisterCloneFunction(const std::function<ObjectPtr(const IObject*, const IObject*, res::ResourceLoader * loader, SpecificClassType<IObject>)>& func)
{
    GCloneFunction = func;
}

void IObject::RegisterSerializeFunction(const std::function<Buffer(const IObject*)>& func)
{
    GSerializeFunction = func;
}

void IObject::RegisterDeserializeFunction(const std::function<ObjectPtr(const void* data, uint32_t size, res::ResourceLoader * loader, SpecificClassType<IObject> mutatedClass)>& func)
{
    GDeserializeFunction = func;
}

//--

ITemplatePropertyBuilder::~ITemplatePropertyBuilder()
{}

//--

ITemplatePropertyValueContainer::~ITemplatePropertyValueContainer()
{}

//--

IObject::IObject()
    : IReferencable(1)
{
    if (!IsDefaultObjectCreation())
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
    static ClassType objectType = RTTI::GetInstance().findClass("IObject"_id);
    return SpecificClassType<IObject>(*objectType.ptr());
}

ClassType IObject::cls() const
{
    return nativeClass();
}

//--

DataViewResult IObject::describeDataView(StringView viewPath, rtti::DataViewInfo& outInfo) const
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

DataViewResult IObject::readDataView(StringView viewPath, void* targetData, Type targetType) const
{
    if (viewPath == "__cls")
    {
        // TODO
    }

    return cls()->readDataView(viewPath, this, targetData, targetType.ptr());
}

DataViewResult IObject::writeDataView(StringView viewPath, const void* sourceData, Type sourceType)
{
    if (!onPropertyChanging(viewPath, sourceData, sourceType))
        return DataViewResultCode::ErrorIllegalOperation;

    if (auto ret = HasError(cls()->writeDataView(viewPath, this, sourceData, sourceType.ptr())))
        return ret;

    onPropertyChanged(viewPath);
    return DataViewResultCode::OK;
}

//--

DataViewPtr IObject::createDataView(bool forceReadOnly) const
{
    return RefNew<DataViewNative>(const_cast<IObject*>(this), forceReadOnly);
}

bool IObject::initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties)
{
    for (const auto& prop : cls()->allTemplateProperties())
    {
        if (prop.nativeProperty)
        {
            auto* localData = prop.nativeProperty->offsetPtr(this);
            templateProperties.compileValue(prop.name, prop.nativeProperty->type(), localData);
        }
    }

    return true;
}

void IObject::queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const
{
    // nothing special here, all default overridable properties are filled by class type
}

//--

bool IObject::onPropertyChanging(StringView path, const void* newData, Type newDataType) const
{
    return true;
}

void IObject::onPropertyChanged(StringView path)
{
    markModified();

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
    InplaceArray<StringID, 16> affectedProperties;
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
    Type ret = new rtti::NativeClass("IObject", sizeof(IObject), alignof(IObject), typeid(IObject).hash_code(), ClassAllocationPool::TAG);
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
    }

} // rtti

//--

xml::DocumentPtr SaveObjectToXML(const IObject* object, StringView rootNodeName /*= "object"*/)
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

bool CopyPropertyValue(const IObject* srcObject, const rtti::Property* srcProperty, IObject* targetObject, const rtti::Property* targetProperty)
{
    const auto srcType = srcProperty->type();
    const auto targetType = targetProperty->type();

    const auto* srcData = srcProperty->offsetPtr(srcObject);
    auto* targetData = targetProperty->offsetPtr(targetObject);

    // TODO: support for inlined objects! and more complex types

    if (!rtti::ConvertData(srcData, srcType, targetData, targetType))
        return false;

    return true;
}

//--

END_BOOMER_NAMESPACE()
