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

#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamTextReader.h"
#include "base/object/include/streamTextWriter.h"

#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"
#include "base/object/include/serializationMapper.h"
#include "base/object/include/serializationUnampper.h"
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

        bool ResourceRefType::writeText(const rtti::TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const
        {
            auto& ptr = *(const res::BaseReference*) data;
            auto loadedObject = helper::GetLoadedObjectPtrForSaving(ptr, typeContext);
            stream.writeValue(ptr.key().path().view(), ptr.key().cls(), loadedObject);
            return true;
        }


        bool ResourceRefType::readText(const rtti::TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const
        {
            auto& ptr = *(res::BaseReference*) data;

            auto policy = stream::ResourceLoadingPolicy::AlwaysLoad;

            StringBuf resourcePathStr;
            ClassType resourceClass;
            ObjectPtr resourceObject;

            if (!stream.readValue(policy, resourcePathStr, resourceClass, resourceObject))
                return false;

            ResourcePath resourcePath(resourcePathStr);
            if (resourcePathStr && !resourcePath)
            {
                TRACE_ERROR("Invalid resource path '{}' loaded", resourcePathStr);
                return false;
            }
            if (resourceObject && !resourceObject->is(m_resourceClass))
            {
                TRACE_ERROR("Loaded '{}' of class '{}' but expected '{}' here", resourcePath, resourceClass, m_resourceClass->name());
                return false;
            }
            if (resourceClass && !resourceClass->is(m_resourceClass))
            {
                TRACE_ERROR("Loaded resource class '{}' but expected '{}' here", resourcePath, resourceClass, m_resourceClass->name());
                return false;
            }

            if (!resourceObject || !resourceObject->is<IResource>())
            {
                // TODO: create stub ?
            }

            ptr.set(base::rtti_cast<IResource>(resourceObject));
            return true;
        }

        enum ResourceRefBinaryFlags : uint8_t
        {
            Inlined = 1,
            Mapped = 2,
            Loaded = 4,
            Valid = 8,
        };

        bool ResourceRefType::writeBinary(const rtti::TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const
        {
            auto& ptr = *(res::BaseReference*) data;

            uint8_t flags = 0;
            if (!ptr.empty())
            {
                flags |= ResourceRefBinaryFlags::Valid;
                flags |= ResourceRefBinaryFlags::Loaded;
                if (ptr.inlined()) flags |= ResourceRefBinaryFlags::Inlined;
                if (file.m_mapper != nullptr) flags |= ResourceRefBinaryFlags::Mapped;
            }

            file.writeValue(flags);

            if (0 != (flags & ResourceRefBinaryFlags::Valid))
            {
                if (0 != (flags & ResourceRefBinaryFlags::Inlined))
                {
                    stream::MappedObjectIndex index = 0;

                    if (file.m_mapper != nullptr)
                        file.m_mapper->mapPointer(ptr.acquire(), index);

                    // TODO: consider writing object directly as well

                    file.writeValue(index);
                }
                else if (0 != (flags & ResourceRefBinaryFlags::Mapped))
                {
                    stream::MappedPathIndex index = 0;
                    file.m_mapper->mapResourceReference(ptr.key().path().view(), ptr.key().cls(), false, index);
                    file.writeValue(index);
                }
                else
                {
                    file.writeName(ptr.key().cls()->name());
                    file.writeText(ptr.key().path().view());
                }
            }

            return true;
        }

        bool ResourceRefType::readBinary(const rtti::TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const
        {
            res::BaseReference loadedRef;

            uint8_t flags = 0;
            file.readValue(flags);

            if (0 != (flags & ResourceRefBinaryFlags::Valid))
            {
                if (0 != (flags & ResourceRefBinaryFlags::Inlined))
                {
                    stream::MappedObjectIndex index = 0;
                    file.readValue(index);

                    if (file.m_unmapper != nullptr)
                    {
                        ObjectPtr object;
                        file.m_unmapper->unmapPointer(index, object);

                        ResourcePtr loadedResource = rtti_cast<IResource>(object);
                        loadedRef.set(loadedResource);
                    }
                }
                else if (0 != (flags & ResourceRefBinaryFlags::Mapped))
                {
                    stream::MappedPathIndex index = 0;
                    file.readValue(index);

                    if (file.m_unmapper != nullptr)
                    {
                        ObjectPtr object;
                        StringBuf path;
                        ClassType cls;

                        file.m_unmapper->unmapResourceReference(index, path, cls, object);

                        ResourcePtr loadedResource = rtti_cast<IResource>(object);
                        if (!loadedResource)
                        {
                            ResourcePath resourcePath(path);
                            ResourceKey resourceKey(resourcePath, cls.cast<IResource>());

                            // TODO: stub ?
                        }

                        loadedRef.set(loadedResource);
                    }
                }
                else
                {
                    auto className = file.readName();
                    auto path = file.readText();

                    auto resourceClass  = RTTI::GetInstance().findClass(className);
                    auto resourcePath = res::ResourcePath(path);
                    if (resourceClass && resourceClass->is(res::IResource::GetStaticClass()) && resourcePath)
                    {
                        ResourcePtr loadedResource;

                        if (file.m_resourceLoader)
                        {
                            ResourceKey resourceKey(resourcePath, resourceClass.cast<IResource>());
                            loadedResource = file.m_resourceLoader->loadResource(resourceKey);
                        }

                        if (!loadedResource)
                        {
                            // TODO: stub ?
                        }

                        loadedRef.set(loadedResource);
                    }
                }
            }

            *(res::BaseReference*)data = loadedRef;
            return true;
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
