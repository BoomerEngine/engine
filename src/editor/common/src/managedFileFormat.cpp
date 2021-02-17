/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedFileFormat.h"

#include "base/config/include/configGroup.h"
#include "base/config/include/configEntry.h"
#include "base/config/include/configSystem.h"
#include "base/image/include/image.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceFactory.h"
#include "base/resource/include/resourceTags.h"
#include "base/resource_compiler/include/importInterface.h"

namespace ed
{

    ///---

    static void ExtractTagFromClass(SpecificClassType<res::IResource> resourceClass, Array<ManagedFileTag>& outTags)
    {
        if (const auto* tagData = resourceClass->findMetadata<res::ResourceTagColorMetadata>())
        {
            if (const auto* descData = resourceClass->findMetadata<res::ResourceDescriptionMetadata>())
            {
                const auto desc = StringView(descData->description());
                if (tagData->color().a > 0 && !desc.empty())
                {
                    auto& tag = outTags.emplaceBack();
                    tag.color = tagData->color();
                    tag.name = StringBuf(desc);
                    //tag.baked = resourceClass->findMetadata < res::>() != nullptr;
                }
            }
        }
    }

    ///---

    ManagedFileFormat::ManagedFileFormat(StringView extension)
        : m_extension(extension)
        , m_description("Unknown Format")
        , m_nativeResourceClass(nullptr)
        , m_showInBrowser(true)
        , m_hasTypeThumbnail(false)
    {
        // enum classes
        static InplaceArray<SpecificClassType<res::IFactory>, 64> factoryClasses;
        if (factoryClasses.empty())
            RTTI::GetInstance().enumClasses(factoryClasses);

        // if the engine can read this format directly we will have a native class for it
        m_nativeResourceClass = res::IResource::FindResourceClassByExtension(extension);
        if (m_nativeResourceClass)
        {
            // create the "native" output for the file
            m_description = res::IResource::GetResourceDescriptionForClass(m_nativeResourceClass);

            // find resource factory
            for (auto factoryClass : factoryClasses)
            {
                if (auto factoryTargetClass = factoryClass->findMetadata<res::FactoryClassMetadata>())
                {
                    if (factoryTargetClass->resourceClass() == m_nativeResourceClass)
                    {
                        m_factory = factoryClass.create();
                    }
                }
            }

            // find formats (extensions) we can import this file form
            {
                InplaceArray<StringView, 20> importExtensions;
                res::IResourceImporter::ListImportableExtensionsForClass(m_nativeResourceClass, importExtensions);

                for (const auto& view : importExtensions)
                    m_importExtensions.pushBack(StringBuf(view).toLower());
            }
        }

        // get config entry for the file
        // TODO: move somewhere else
        if (const auto& configGroup = base::config::FindGroup(TempString("Format.{}", extension)))
            m_description = configGroup->entryValue("Description", m_description);

        // debug log
        TRACE_SPAM("Registered format '{}' for extension '{}'", m_description, m_extension);
    }

    ManagedFileFormat::~ManagedFileFormat()
    {}

    res::ResourcePtr ManagedFileFormat::createEmpty() const
    {
        if (m_factory)
            return m_factory->createResource();
        return nullptr;
    }

    void ManagedFileFormat::printTags(IFormatStream& f, StringView separator) const
    {
        for (const auto& tag : tags())
        {
            f.appendf("[tag:{}]", tag.color);

            if (tag.baked)
                f.appendf("[img:cog] ");
            else
                f.appendf("[img:file_empty_edit] ");

            if (tag.color.luminanceSRGB() > 0.5f)
            {
                f << "[color:#000]";
                f << tag.name;
                f << "[/color]";
            }
            else
            {
                f << tag.name;
            }

            f << "[/tag]";

            f << separator;
        }
    }

    const image::Image* ManagedFileFormat::thumbnail() const
    {
        // load thumbnail for this type file
        if (!m_thumbnailLoadAttempted)
        {
            m_thumbnailLoadAttempted = true;

            auto classThumbnailImage = base::LoadImageFromDepotPath(TempString("/engine/interface/thumbnails/{}.png", m_extension));
            if (classThumbnailImage)
            {
                m_thumbnail = classThumbnailImage;
                m_hasTypeThumbnail = true;
            }
            else
            {
                static const auto defaultThumbnail = LoadImageFromDepotPath("/engine/interface/thumbnails/file.png");
                m_thumbnail = defaultThumbnail;
            }
        }

        return m_thumbnail;
    }

    bool ManagedFileFormat::loadableAsType(ClassType resourceClass) const
    {
        if (m_nativeResourceClass && m_nativeResourceClass == resourceClass)
            return true;

        return false;
    }

    ///---

    ManagedFileFormatRegistry::ManagedFileFormatRegistry()
    {
        m_formatMap.reserve(64);
        m_allFormats.reserve(64);
        m_userCreatableFormats.reserve(64);
        m_userImportableFormats.reserve(64);
    }

    void ManagedFileFormatRegistry::cacheFormats()
    {
        InplaceArray<SpecificClassType<res::IResource>, 64> resourceClasses;
        RTTI::GetInstance().enumClasses(resourceClasses);

        // auto cache all native formats
        for (const auto cls : resourceClasses)
        {
            if (const auto ext = res::IResource::GetResourceExtensionForClass(cls))
                format(ext);
        }
    }

    const ManagedFileFormat* ManagedFileFormatRegistry::format(StringView extension)
    {
        //auto lock = CreateLock(m_lock);

        auto extKey = extension.calcCRC64();

        // get from map
        ManagedFileFormat* format = nullptr;
        if (!m_formatMap.find(extKey, format))
        {
            format = new ManagedFileFormat(extension);
            m_formatMap[extKey] = format;

            // gather all formats, sort by user description
            m_allFormats.pushBack(format);
            std::sort(m_allFormats.begin(), m_allFormats.end(), [](ManagedFileFormat* a, ManagedFileFormat* b) { return a->description() < b->description(); });

            // add to local lists
            if (format->canUserCreate())
            {
                m_userCreatableFormats.pushBack(format);
                std::sort(m_userCreatableFormats.begin(), m_userCreatableFormats.end(), [](ManagedFileFormat* a, ManagedFileFormat* b) { return a->description() < b->description(); });
            }

            // add to import list
            if (format->canUserImport())
            {
                m_userImportableFormats.pushBack(format);
                std::sort(m_userImportableFormats.begin(), m_userImportableFormats.end(), [](ManagedFileFormat* a, ManagedFileFormat* b) { return a->description() < b->description(); });
            }
        }

        DEBUG_CHECK(format->extension() == extension);
        return format;
    }

    void ManagedFileFormatRegistry::deinit()
    {
        m_formatMap.clear();
        m_allFormats.clearPtr();
        m_userCreatableFormats.clear();
        m_userImportableFormats.clear();
    }

    ///---

} // depot


