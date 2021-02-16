/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#include "build.h"

#include "resource.h"
#include "resourceAsyncReference.h"
#include "resourceAsyncReferenceType.h"

#include "base/object/include/streamOpcodeReader.h"
#include "base/object/include/streamOpcodeWriter.h"

#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"
#include "base/object/include/rttiProperty.h"
#include "base/object/include/rttiType.h"
#include "base/xml/include/xmlWrappers.h"

namespace base
{
    namespace res
    {

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
            m_traits.metaType = rtti::MetaType::AsyncResourceRef;
            m_traits.convClass = rtti::TypeConversionClass::TypeAsyncRef;
            m_traits.alignment = alignof(res::BaseAsyncReference);
            m_traits.size = sizeof(res::BaseAsyncReference);
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

        bool ResourceAsyncRefType::referencePatchResource(void* data, res::IResource* currentResource, res::IResource* newResources) const
        {
            return false; // we never patch async handles as they don't contain actual resources
        }

        void ResourceAsyncRefType::readResourceRef(const void* data, res::BaseAsyncReference& outResRef) const
        {
            outResRef = *(const res::BaseAsyncReference*)data;
        }

        void ResourceAsyncRefType::writeResourceRef(void* data, const res::BaseAsyncReference& resRef) const
        {
            *(res::BaseAsyncReference*)data = resRef;
        }

        bool ResourceAsyncRefType::compare(const void* data1, const void* data2) const
        {
            auto& ptr1 = *(const res::BaseAsyncReference*) data1;
            auto& ptr2 = *(const res::BaseAsyncReference*) data2;
            return ptr1 == ptr2;
        }

        void ResourceAsyncRefType::copy(void* dest, const void* src) const
        {
            auto& ptrSrc = *(const res::BaseAsyncReference*) src;
            auto& ptrDest = *(res::BaseAsyncReference*) dest;
            ptrDest = ptrSrc;
        }

        void ResourceAsyncRefType::construct(void* object) const
        {
            new (object) res::BaseAsyncReference();
        }

        void ResourceAsyncRefType::destruct(void* object) const
        {
            ((res::BaseAsyncReference*)object)->~BaseAsyncReference();
        }

        void ResourceAsyncRefType::writeBinary(rtti::TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const
        {
            auto& ptr = *(res::BaseAsyncReference*) data;
            file.writeResourceReference(ptr.path().view(), m_resourceClass, true);
        }

        void ResourceAsyncRefType::readBinary(rtti::TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const
        {
            res::ResourcePath loadedKey;

            const auto* resData = file.readResource();
            if (resData && resData->path)
            {
                const auto resourceClass = resData->type.cast<res::IResource>();
                if (resourceClass && resourceClass->is(m_resourceClass))
                    loadedKey = res::ResourcePath(resData->path);
            }

            *(res::BaseAsyncReference*)data = loadedKey;
        }

        void ResourceAsyncRefType::writeXML(rtti::TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const
        {
            auto& ptr = *(const res::BaseAsyncReference*) data;
            if (!ptr.empty())
                node.writeAttribute("path", ptr.path().view());
        }

        void ResourceAsyncRefType::readXML(rtti::TypeSerializationContext& typeContext, const xml::Node& node, void* data) const
        {
            auto& ptr = *(res::BaseAsyncReference*) data;

            const auto path = node.attribute("path");
            if (path.empty())
                ptr = res::BaseAsyncReference();
            else
                ptr = res::BaseAsyncReference(path);
        }

        Type ResourceAsyncRefType::ParseType(StringParser& typeNameString, rtti::TypeSystem& typeSystem)
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
            ASSERT(type && type->metaType() == rtti::MetaType::AsyncResourceRef);

            return static_cast<const ResourceAsyncRefType*>(type.ptr());
        }

        //--

    } // res
} // base
