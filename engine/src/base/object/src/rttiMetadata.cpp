/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\metadata #]
***/

#include "build.h"
#include "rttiClassType.h"
#include "rttiMetadata.h"
#include "rttiTypeSystem.h"
#include "rttiNativeClassType.h"

namespace base
{
    namespace rtti
    {
        //---

        IMetadata::~IMetadata()
        {}

        void IMetadata::RegisterType(rtti::TypeSystem& typeSystem)
        {
            Type classType = new rtti::NativeClass("base::rtti::IMetadata", sizeof(IMetadata), alignof(IMetadata), typeid(IMetadata).hash_code());
            //auto baseClassType = typeSystem.findClass("base::IObject"_id);
            //classType->baseClass(baseClassType);
            typeSystem.registerType(classType);
        }

        ClassType IMetadata::cls() const
        {
            return nativeClass();
        }

        SpecificClassType<IMetadata> IMetadata::GetStaticClass()
        {
            static ClassType objectType = RTTI::GetInstance().findClass("base::rtti::IMetadata"_id);
            return SpecificClassType<IMetadata>(*objectType.ptr());
        }

        //---

        ShortTypeNameMetadata::~ShortTypeNameMetadata()
        {}

        ClassType ShortTypeNameMetadata::cls() const
        {
            return GetStaticClass();
        }

        ClassType ShortTypeNameMetadata::nativeClass() const
        {
            return GetStaticClass();
        }

        void ShortTypeNameMetadata::RegisterType(rtti::TypeSystem& typeSystem)
        {
            auto classType = new rtti::NativeClass("base::rtti::ShortTypeNameMetadata", sizeof(ShortTypeNameMetadata), alignof(ShortTypeNameMetadata), typeid(ShortTypeNameMetadata).hash_code());
            classType->bindCtorDtor<ShortTypeNameMetadata>();
            auto baseClassType = typeSystem.findClass("rtti::IMetadata"_id);
            classType->baseClass(baseClassType.ptr());
            typeSystem.registerType(Type(classType));
        }

        SpecificClassType<ShortTypeNameMetadata> ShortTypeNameMetadata::GetStaticClass()
        {
            static ClassType objectType = RTTI::GetInstance().findClass("rtti::ShortTypeNameMetadata"_id);
            return SpecificClassType<ShortTypeNameMetadata>(*objectType.ptr());
        }

        //---

        MetadataContainer::MetadataContainer()
        {
        }

        MetadataContainer::~MetadataContainer()
        {
            removeAllMetadata();
        }

        void MetadataContainer::removeAllMetadata()
        {
           // m_metadata.clearPtr(); // crashes :(
        }

        IMetadata& MetadataContainer::addMetadata(ClassType metadataType)
        {
            for (auto ptr  : m_metadata)
                if (ptr->cls()->is(metadataType))
                    return *ptr;

            DEBUG_CHECK_EX(!metadataType->isAbstract(), TempString("Cannot create metadata from abstract class '{}'", metadataType));

            auto metadata = metadataType->createPointer<IMetadata>(POOL_PERSISTENT);
            m_metadata.pushBack(metadata);
            return *metadata;
        }

        void MetadataContainer::attachMetadata(IMetadata* metadata)
        {
            ASSERT(metadata != nullptr);
            ASSERT(this->metadata(metadata->cls()) == nullptr);
            m_metadata.pushBack(metadata);
        }

        const IMetadata* MetadataContainer::metadata(ClassType metadataType) const
        {
            for (auto ptr  : m_metadata)
                if (ptr->cls()->is(metadataType))
                    return ptr;

            return nullptr;
        }

        void MetadataContainer::collectMetadataList(Array<const IMetadata*>& outMetadataList) const
        {
            if (!m_metadata.empty())
            {
                outMetadataList.reserve(outMetadataList.size() + m_metadata.size());

                for (auto localMetadata : localMetadataList())
                    outMetadataList.pushBack(localMetadata);
            }
        }

        //---

    } // rtti
} // base
