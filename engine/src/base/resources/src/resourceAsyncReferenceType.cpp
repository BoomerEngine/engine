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
            : IType(FormatAsyncRefTypeName(classType->name()))
            , m_resourceClass(classType)
        {
            m_traits.metaType = rtti::MetaType::AsyncResourceRef;
            m_traits.alignment = alignof(res::BaseAsyncReference);
            m_traits.size = sizeof(res::BaseAsyncReference);
            m_traits.initializedFromZeroMem = true;
            m_traits.requiresConstructor = false;
            m_traits.requiresDestructor = false;
            m_traits.simpleCopyCompare = true;
            m_traits.hashable = true;
        }

        ResourceAsyncRefType::~ResourceAsyncRefType()
        {
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

        bool ResourceAsyncRefType::writeText(const rtti::TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const
        {
            auto& ptr = *(const res::BaseAsyncReference*) data;
            stream.writeValue(ptr.key().path().view(), ptr.key().cls(), nullptr);
            return true;
        }


        bool ResourceAsyncRefType::readText(const rtti::TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const
        {
            auto& ptr = *(res::BaseAsyncReference*) data;

            auto policy = stream::ResourceLoadingPolicy::NeverLoad;

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
            if (resourceClass && !resourceClass->is(m_resourceClass))
            {
                TRACE_ERROR("Loaded resource class '{}' but expected '{}' here", resourcePath, resourceClass, m_resourceClass->name());
                return false;
            }

            ptr.set(resourcePath, resourceClass.cast<IResource>());
            return true;
        }

        enum ResourceRefBinaryFlags : uint8_t
        {
            Inlined = 1,
            Mapped = 2 ,
            Loaded = 4,
            Valid = 8,
        };

        bool ResourceAsyncRefType::writeBinary(const rtti::TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const
        {
            auto& ptr = *(res::BaseAsyncReference*) data;

            uint8_t flags = 0;
            if (!ptr.empty())
            {
                flags |= ResourceRefBinaryFlags::Valid;
                if (file.m_mapper != nullptr) flags |= ResourceRefBinaryFlags::Mapped;
            }

            file.writeValue(flags);

            if (0 != (flags & ResourceRefBinaryFlags::Valid))
            {
                if (0 != (flags & ResourceRefBinaryFlags::Mapped))
                {
                    stream::MappedPathIndex index = 0;
                    file.m_mapper->mapResourceReference(ptr.key().path().view(), ptr.key().cls(), true, index);
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

        bool ResourceAsyncRefType::readBinary(const rtti::TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const
        {
            res::BaseAsyncReference loadedRef;

            uint8_t flags = 0;
            file.readValue(flags);

            if (0 != (flags & ResourceRefBinaryFlags::Valid))
            {
                if (0 != (flags & ResourceRefBinaryFlags::Inlined))
                {
                    // inlined objects can't become async refs
                    loadedRef.reset();
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

                        ResourcePath resourcePath(path);
                        ResourceKey resourceKey(resourcePath, cls.cast<IResource>());
                        loadedRef.set(resourceKey);
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
                        ResourceKey resourceKey(resourcePath, resourceClass.cast<IResource>());
                        loadedRef.set(resourceKey);
                    }
                }
            }

            *(res::BaseAsyncReference*)data = loadedRef;
            return true;
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

            return Type(MemNew(ResourceAsyncRefType, (const SpecificClassType<IResource>&) classType));
        }

        //---

    } // rtti
} // base
