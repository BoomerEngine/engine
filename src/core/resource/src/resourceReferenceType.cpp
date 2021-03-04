/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#include "build.h"

#include "resource.h"
#include "resourceReference.h"
#include "resourceReferenceType.h"

#include "core/object/include/streamOpcodeReader.h"
#include "core/object/include/streamOpcodeWriter.h"

#include "core/containers/include/stringBuilder.h"
#include "core/containers/include/stringParser.h"
#include "core/object/include/rttiProperty.h"
#include "core/object/include/rttiType.h"
#include "resourceLoader.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

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
    m_traits.alignment = alignof(res::BaseReference);
    m_traits.size = sizeof(res::BaseReference);
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

/*void ResourceRefType::referenceReadResource(const void* data, RefPtr<res::IResource>& outRef) const
{
    const auto& ptr1 = *(const res::BaseReference*) data;
    outRef = ptr1.acquire();
}

void ResourceRefType::referenceWriteResource(void* data, res::IResource* resource) const
{
    auto& ptr1 = *(res::BaseReference*) data;
    ptr1 = ResourcePtr(AddRef(resource));
}*/

bool ResourceRefType::referencePatchResource(void* data, res::IResource* currentResource, res::IResource* newResources) const
{
    auto& ptr1 = *(res::BaseReference*) data;
    if (ptr1.load() != currentResource)
        return false;

    ptr1 = ResourcePtr(AddRef(newResources));
    return true;
}

void ResourceRefType::readResourceRef(const void* data, res::BaseReference& outResRef) const
{
    outResRef = *(const res::BaseReference*)data;
}

void ResourceRefType::writeResourceRef(void* data, const res::BaseReference& resRef) const
{
    *(res::BaseReference*)data = resRef;
}

bool ResourceRefType::compare(const void* data1, const void* data2) const
{
    auto& ptr1 = *(const res::BaseReference*) data1;
    auto& ptr2 = *(const res::BaseReference*) data2;
    return ptr1 == ptr2;
}

void ResourceRefType::copy(void* dest, const void* src) const
{
    auto& ptrSrc = *(const res::BaseReference*) src;
    auto& ptrDest = *(res::BaseReference*) dest;
    ptrDest = ptrSrc;
}

void ResourceRefType::construct(void* object) const
{
    new (object) res::BaseReference();
}

void ResourceRefType::destruct(void* object) const
{
    ((res::BaseReference*)object)->~BaseReference();
}

namespace helper
{

    static ObjectPtr GetLoadedObjectPtrForSaving(const res::BaseReference& ptr, const TypeSerializationContext& typeContext)
    {
        return ptr.load();
    }

} // helper

enum ResourceRefBinaryFlag
{
    External = 1,
    Inlined = 2,
};

void ResourceRefType::writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const
{
    auto& ptr = *(const res::BaseReference*) data;

    uint8_t flag = 0;
    if (!ptr.empty())
    {
        /*if (ptr.inlined()) 
            flag = ResourceRefBinaryFlag::Inlined;
        else*/
            flag = ResourceRefBinaryFlag::External;
    }

    file.writeTypedData(flag);

    /*if (flag == ResourceRefBinaryFlag::Inlined)
        file.writePointer(ptr.acquire());
    else if (flag == ResourceRefBinaryFlag::External)*/
    file.writeResourceReference(ptr.path().view(), m_resourceClass, false);
}

void ResourceRefType::readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const
{
    res::BaseReference loadedRef;

    uint8_t flags = 0;
    file.readTypedData(flags);

    if (flags == ResourceRefBinaryFlag::Inlined)
    {
        auto pointer = rtti_cast<IResource>(file.readPointer());
        if (pointer && pointer->is(m_resourceClass))
            loadedRef = ResourcePtr(AddRef(pointer));
    }
    else if (flags == ResourceRefBinaryFlag::External)
    {
        const auto* resData = file.readResource();
        if (resData && resData->path)
        {
            if (resData->loaded && resData->loaded->is(m_resourceClass))
                loadedRef = rtti_cast<IResource>(resData->loaded);
            else
                loadedRef = res::BaseReference(resData->path.view());
        }
    }
            
    *(res::BaseReference*)data = loadedRef;
}
       
//--

void ResourceRefType::writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const
{
    auto& ptr = *(const res::BaseReference*) data;

    if (!ptr.empty())
    {
        /*if (ptr.inlined())
        {
            if (auto object = ptr.acquire())
            {
                node.writeAttribute("class", object->cls()->name().view());
                object->writeXML(node);
            }
        }
        else */if (ptr.path())
        {
            node.writeAttribute("path", ptr.path().view());
        }
    }
}

void ResourceRefType::readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const
{
    res::BaseReference loadedRef;

    const auto path = node.attribute("path");
    if (path)
    {
        if (typeContext.resourceLoader)
        {
            auto loadedResource = typeContext.resourceLoader->loadResource(path);

            if (loadedResource && !loadedResource->is(m_resourceClass))
                loadedResource = nullptr;

            if (loadedResource)
                loadedRef = loadedResource;
            else
                loadedRef = res::BaseReference(path);
        }
    }

    *(res::BaseReference*)data = loadedRef;
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

    /*auto resourceClass = classType.toSpecificClass<IResource>();
    if (!resourceClass)
    {
        TRACE_ERROR("Unable to build a resource reference type from '{}' that is not a resource", innerTypeName);
        return nullptr;
    }*/

    return new ResourceRefType((const SpecificClassType<IResource>&) classType);
}

//---

END_BOOMER_NAMESPACE_EX(res)
