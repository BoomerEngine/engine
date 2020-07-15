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

#include "base/object/include/streamOpcodeReader.h"
#include "base/object/include/streamOpcodeWriter.h"

#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"
#include "base/object/include/rttiProperty.h"
#include "base/object/include/rttiType.h"
#include "resourceLoader.h"

namespace base
{
    namespace res
    {

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
            m_traits.metaType = rtti::MetaType::ResourceRef;
            m_traits.convClass = rtti::TypeConversionClass::TypeSyncRef;
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

        void ResourceRefType::referenceReadResource(const void* data, RefPtr<res::IResource>& outRef) const
        {
            const auto& ptr1 = *(const res::BaseReference*) data;
            outRef = ptr1.acquire();
        }

        void ResourceRefType::referenceWriteResource(void* data, res::IResource* resource) const
        {
            auto& ptr1 = *(res::BaseReference*) data;
            ptr1 = ResourcePtr(AddRef(resource));
        }

        bool ResourceRefType::referencePatchResource(void* data, res::IResource* currentResource, res::IResource* newResources) const
        {
            auto& ptr1 = *(res::BaseReference*) data;
            if (ptr1.acquire() != currentResource)
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

            static ObjectPtr GetLoadedObjectPtrForSaving(const res::BaseReference& ptr, const rtti::TypeSerializationContext& typeContext)
            {
                return ptr.acquire();
            }

        } // helper

        enum ResourceRefBinaryFlag
        {
            External = 1,
            Inlined = 2,
        };

        void ResourceRefType::writeBinary(rtti::TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const
        {
            auto& ptr = *(res::BaseReference*) data;

            uint8_t flag = 0;
            if (!ptr.empty())
            {
                if (ptr.inlined()) 
                    flag = ResourceRefBinaryFlag::Inlined;
                else
                    flag = ResourceRefBinaryFlag::External;
            }

            file.writeTypedData(flag);

            if (flag == ResourceRefBinaryFlag::Inlined)
                file.writePointer(ptr.acquire());
            else if (flag == ResourceRefBinaryFlag::External)
                file.writeResourceReference(ptr.key().path().view(), ptr.key().cls(), false);
        }

        void ResourceRefType::readBinary(rtti::TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const
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
                    {
                        loadedRef = rtti_cast<IResource>(resData->loaded);
                    }
                    else
                    {
                        const auto resourceClass = resData->type.cast<res::IResource>();
                        if (resourceClass && resourceClass->is(m_resourceClass))
                            loadedRef = res::BaseReference(res::ResourceKey(res::ResourcePath(resData->path), resourceClass));
                    }
                }
            }
            
            *(res::BaseReference*)data = loadedRef;
        }
       
        Type ResourceRefType::ParseType(StringParser& typeNameString, rtti::TypeSystem& typeSystem)
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

            return Type(MemNew(ResourceRefType, (const SpecificClassType<IResource>&) classType));
        }

        //---

    } // rtti
} // base
