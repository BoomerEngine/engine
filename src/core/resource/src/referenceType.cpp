/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#include "build.h"

#include "resource.h"
#include "reference.h"
#include "referenceType.h"

#include "core/object/include/serializationReader.h"
#include "core/object/include/serializationWriter.h"

#include "core/containers/include/stringBuilder.h"
#include "core/containers/include/stringParser.h"
#include "core/object/include/rttiProperty.h"
#include "core/object/include/rttiType.h"
#include "loader.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE()

//--

const char* ResourceRefType::TypePrefix = "ref<";


StringID FormatRefTypeName(StringID className)
{
    StringBuilder builder;
    builder.append(ResourceRefType::TypePrefix);
    builder.append(className.c_str());
    builder.append(">");
    return StringID(builder.c_str());
}

//--

ResourceRefType::ResourceRefType(SpecificClassType<IResource> classType)
    : IResourceReferenceType(FormatRefTypeName(classType->name()))
    , m_resourceClass(classType)
{
    m_traits.metaType = MetaType::ResourceRef;
    m_traits.convClass = TypeConversionClass::TypeSyncRef;
    m_traits.alignment = alignof(BaseReference);
    m_traits.size = sizeof(BaseReference);
    m_traits.initializedFromZeroMem = true;
    m_traits.requiresConstructor = true;
    m_traits.requiresDestructor = true;
    m_traits.simpleCopyCompare = true;
    m_traits.hashable = true;
}

ResourceRefType::~ResourceRefType()
{
}

ClassType ResourceRefType::referenceResourceClass() const
{
    return m_resourceClass;
}

bool ResourceRefType::referencePatchResource(void* data, IResource* currentResource, IResource* newResource) const
{
    auto& ptr1 = *(BaseReference*) data;
    if (ptr1.resource() != currentResource) // most common case
        return false;

    ptr1 = BaseReference(ptr1.id(), newResource);
    return true;
}

void ResourceRefType::readResourceRef(const void* data, BaseReference& outResRef) const
{
    outResRef = *(const BaseReference*)data;
}

void ResourceRefType::writeResourceRef(void* data, const BaseReference& resRef) const
{
    *(BaseReference*)data = resRef;
}

bool ResourceRefType::compare(const void* data1, const void* data2) const
{
    auto& ptr1 = *(const BaseReference*) data1;
    auto& ptr2 = *(const BaseReference*) data2;
    return ptr1 == ptr2;
}

void ResourceRefType::copy(void* dest, const void* src) const
{
    auto& ptrSrc = *(const BaseReference*) src;
    auto& ptrDest = *(BaseReference*) dest;
    ptrDest = ptrSrc;
}

void ResourceRefType::construct(void* object) const
{
    new (object) BaseReference();
}

void ResourceRefType::destruct(void* object) const
{
    ((BaseReference*)object)->~BaseReference();
}

enum ResourceRefBinaryFlag
{
    External = 1,
    Inlined = 2,
};

void ResourceRefType::writeBinary(TypeSerializationContext& typeContext, SerializationWriter& file, const void* data, const void* defaultData) const
{
    auto& ptr = *(const BaseReference*) data;

    uint8_t flag = 0;
    if (ptr.resource() && !ptr.id())
        flag = ResourceRefBinaryFlag::Inlined;
    else if (ptr.id())
        flag = ResourceRefBinaryFlag::External;

    file.writeTypedData(flag);

    if (flag == ResourceRefBinaryFlag::Inlined)
        file.writePointer(ptr.resource());
    else if (flag == ResourceRefBinaryFlag::External)
        file.writeResourceReference(ptr.id().guid(), m_resourceClass, false);
}

void ResourceRefType::readBinary(TypeSerializationContext& typeContext, SerializationReader& file, void* data) const
{
    BaseReference loadedRef;

    uint8_t flags = 0;
    file.readTypedData(flags);

    if (flags == ResourceRefBinaryFlag::Inlined)
    {
        auto pointer = rtti_cast<IResource>(file.readPointer());
        if (pointer && pointer->is(m_resourceClass))
            loadedRef = BaseReference(ResourceID(), pointer);
    }
    else if (flags == ResourceRefBinaryFlag::External)
    {
        const auto* resData = file.readResource();
        if (resData && resData->id)
        {
            IResource* loaded = nullptr;
            if (resData->loaded && resData->loaded->is(m_resourceClass))
                loaded = rtti_cast<IResource>(resData->loaded);

            loadedRef = BaseReference(resData->id, loaded);
        }
    }
            
    *(BaseReference*)data = loadedRef;
}
       
//--

void ResourceRefType::writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const
{
    auto& ptr = *(const BaseReference*) data;

    if (ptr.resource() && !ptr.id())
    {
        node.writeAttribute("class", ptr.resource()->cls()->name().view());
        ptr.resource()->writeXML(node);
    }
    else if (ptr.id())
    {
        node.writeAttribute("id", TempString("{}", ptr.id()));
    }
}

void ResourceRefType::readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const
{
    BaseReference loadedRef;

    if (const auto idText = node.attribute("id"))
    {
        ResourceID id;
        if (ResourceID::Parse(idText, id))
        {
            ResourcePtr loadedResource;

            if (typeContext.loadImports)
            {
                loadedResource = LoadResource(id);

                if (loadedResource && !loadedResource->is(m_resourceClass))
                    loadedResource = nullptr;
            }

            loadedRef = BaseReference(id, loadedResource);
        }
    }
    else if (const auto classText = node.attribute("class"))
    {
        // TODO: support for inplace resources
    }

    *(BaseReference*)data = loadedRef;
}

//--

Type ResourceRefType::ParseType(StringParser& typeNameString, TypeSystem& typeSystem)
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

    return new ResourceRefType((const SpecificClassType<IResource>&) classType);
}

//---

END_BOOMER_NAMESPACE()
