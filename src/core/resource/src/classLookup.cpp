/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\utils #]
***/

#include "build.h"
#include "classLookup.h"
#include "resource.h"
#include "tags.h"

BEGIN_BOOMER_NAMESPACE()

namespace prv
{
    ResourceClassLookup::ResourceClassLookup()
    {
        buildClassMap();
    }

    SpecificClassType<IResource> ResourceClassLookup::resolveResourceClassHash(ResourceClassHash classHash) const
    {
        SpecificClassType<IResource> ret = nullptr;
        m_classesByHash.find(classHash, ret);
        return ret;
    }

    SpecificClassType<IResource> ResourceClassLookup::resolveResourceExtension(StringView extension) const
    {
        SpecificClassType<IResource> ret = nullptr;
        m_classesByExtension.find(extension.calcCRC64(), ret);
        return ret;
    }
            
    void ResourceClassLookup::deinit()
    {
        m_classesByHash.clear();
        m_classesByExtension.clear();
        m_classesByShortName.clear();
    }

    void ResourceClassLookup::buildClassMap()
    {
        InplaceArray< SpecificClassType<IResource>, 100> allResourceClasses;
        RTTI::GetInstance().enumClasses(allResourceClasses);

        uint32_t numRegistered = 0;
        for (auto classPtr : allResourceClasses)
        {
            // ignore abstract classes
            if (classPtr->isAbstract())
                continue;

            // add to map by hashed name (FourCC)
            {
                StringBuf ext = StringBuf(classPtr->name().view());
                ResourceClassHash hash = ext.cRC64();

                // duplicate ?
                SpecificClassType<IResource> existingClass = nullptr;
                if (m_classesByHash.find(hash, existingClass))
                {
                    FATAL_ERROR(TempString("Resource hash conflict between class '{}' and '{}'", classPtr->name(), existingClass->name()));
                    continue;
                }

                // put in map
                TRACE_SPAM("Resource class '{}' mapped with hash 0x{}", classPtr->name(), Hex(hash));
                m_classesByHash.set(hash, classPtr);
            }

            // add to map by native extension - only text resources
            {
                auto metadata  = static_cast<const ResourceExtensionMetadata*>(classPtr->MetadataContainer::metadata(ResourceExtensionMetadata::GetStaticClass()));
                const StringView ext = metadata ? metadata->extension() : "";
                if (ext.empty())
                    continue;

                auto hash = ext.calcCRC64();

                // duplicate ?
                SpecificClassType<IResource> existingClass = nullptr;
                if (m_classesByExtension.find(hash, existingClass))
                {
                    FATAL_ERROR(TempString("Resource extension '{}' conflict between class '{}' and '{}'", ext, classPtr->name(), existingClass->name()));
                    continue;
                }

                // put in map
                TRACE_SPAM("Resource class '{}' mapped with extension '{}'", classPtr->name(), ext);
                m_classesByExtension.set(hash, classPtr);
            }

            numRegistered += 1;
        }

        TRACE_SPAM("Registered {} resource classes", numRegistered);
    }

} // prv

END_BOOMER_NAMESPACE()
