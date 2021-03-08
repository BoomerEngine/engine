/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#include "build.h"

#include "resource.h"
#include "asyncReference.h"
#include "asyncReferenceType.h"

#include "core/object/include/serializationReader.h"
#include "core/object/include/serializationWriter.h"

#include "core/containers/include/stringBuilder.h"
#include "core/containers/include/stringParser.h"
#include "core/object/include/rttiProperty.h"
#include "core/object/include/rttiType.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE()

//--

const char* ResourceAsyncRefType::TypePrefix = "async<";


StringID FormatAsyncRefTypeName(StringID className)
{
    StringBuilder builder;
    builder.append(ResourceAsyncRefType::TypePrefix);
    builder.append(className.c_str());
    builder.append(">");
    return StringID(builder.c_str());
}

//--

ResourceAsyncRefType::ResourceAsyncRefType(SpecificClassType<IResource> classType)
    : IResourceReferenceType(FormatAsyncRefTypeName(classType->name()))
    , m_resourceClass(classType)
{
    m_traits.metaType = MetaType::ResourceAsyncRef;
    m_traits.convClass = TypeConversionClass::TypeAsyncRef;
    m_traits.alignment = alignof(BaseAsyncReference);
    m_traits.size = sizeof(BaseAsyncReference);
    m_traits.initializedFromZeroMem = true;
    m_traits.requiresConstructor = true;
    m_traits.requiresDestructor = false;
    m_traits.simpleCopyCompare = true;
    m_traits.hashable = true;
}

ResourceAsyncRefType::~ResourceAsyncRefType()
{
}

ClassType ResourceAsyncRefType::referenceResourceClass() const
{
    return m_resourceClass;
}

bool ResourceAsyncRefType::referencePatchResource(void* data, IResource* currentResource, IResource* newResources) const
{
    return false; // we never patch async handles as they don't contain actual resources
}

void ResourceAsyncRefType::readResourceRef(const void* data, BaseAsyncReference& outResRef) const
{
    outResRef = *(const BaseAsyncReference*)data;
}

void ResourceAsyncRefType::writeResourceRef(void* data, const BaseAsyncReference& resRef) const
{
    *(BaseAsyncReference*)data = resRef;
}

bool ResourceAsyncRefType::compare(const void* data1, const void* data2) const
{
    auto& ptr1 = *(const BaseAsyncReference*) data1;
    auto& ptr2 = *(const BaseAsyncReference*) data2;
    return ptr1 == ptr2;
}

void ResourceAsyncRefType::copy(void* dest, const void* src) const
{
    auto& ptrSrc = *(const BaseAsyncReference*) src;
    auto& ptrDest = *(BaseAsyncReference*) dest;
    ptrDest = ptrSrc;
}

void ResourceAsyncRefType::construct(void* object) const
{
    new (object) BaseAsyncReference();
}

void ResourceAsyncRefType::destruct(void* object) const
{
    ((BaseAsyncReference*)object)->~BaseAsyncReference();
}

void ResourceAsyncRefType::writeBinary(TypeSerializationContext& typeContext, SerializationWriter& file, const void* data, const void* defaultData) const
{
    const auto& ptr = *(const BaseAsyncReference*) data;
    file.writeResourceReference(ptr.id().guid(), m_resourceClass, true);
}

void ResourceAsyncRefType::readBinary(TypeSerializationContext& typeContext, SerializationReader& file, void* data) const
{
    ResourceID loadedKey;

    const auto* resData = file.readResource();
    if (resData && resData->id)
    {
        const auto resourceClass = resData->type.cast<IResource>();
        if (resourceClass && resourceClass->is(m_resourceClass))
            loadedKey = resData->id;
    }

    *(BaseAsyncReference*)data = loadedKey;
}

void ResourceAsyncRefType::writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const
{
    auto& ptr = *(const BaseAsyncReference*) data;
    if (!ptr.empty())
        node.writeAttribute("id", TempString("{}", ptr.id()));
}

void ResourceAsyncRefType::readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const
{
    BaseAsyncReference ref;

    const auto path = node.attribute("id");
    if (!path.empty())
    {
        ResourceID id;
        if (ResourceID::Parse(path, id))
        {
            ref = BaseAsyncReference(id);
        }
        else
        {
            // TODO!
        }
    }

    *(BaseAsyncReference*)data = ref;
}

Type ResourceAsyncRefType::ParseType(StringParser& typeNameString, TypeSystem& typeSystem)
{
    StringID innerTypeName;
    if (!typeNameString.parseTypeName(innerTypeName))
        return nullptr;

    if (!typeNameString.parseKeyword(">"))
        return nullptr;

    auto classType = typeSystem.findType(innerTypeName);
    if (!classType)
    {
        TRACE_ERROR("Unable to parse a resource reference type from '{}'", innerTypeName);
        return nullptr;
    }

    return new ResourceAsyncRefType((const SpecificClassType<IResource>&) classType);
}

//---

const ResourceAsyncRefType* CreateAsyncRefType(SpecificClassType<IResource> resourceClass)
{
    DEBUG_CHECK(resourceClass && resourceClass->is<IResource>());
    if (!resourceClass || !resourceClass->is<IResource>())
        return nullptr;

    const auto typeName = FormatAsyncRefTypeName(resourceClass->name());
    const auto type = RTTI::GetInstance().findType(typeName);
    ASSERT(type && type->metaType() == MetaType::ResourceAsyncRef);

    return static_cast<const ResourceAsyncRefType*>(type.ptr());
}

//--

END_BOOMER_NAMESPACE()
