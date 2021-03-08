/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "assetFormat.h"

#include "core/config/include/group.h"
#include "core/config/include/entry.h"
#include "core/config/include/system.h"
#include "core/image/include/image.h"
#include "core/resource/include/resource.h"
#include "core/resource/include/resource.h"
#include "core/resource/include/factory.h"
#include "core/resource/include/tags.h"
#include "core/resource_compiler/include/importInterface.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

static void ExtractTagFromClass(SpecificClassType<IResource> resourceClass, Array<ManagedFileTag>& outTags)
{
    if (const auto* tagData = resourceClass->findMetadata<ResourceTagColorMetadata>())
    {
        if (const auto* descData = resourceClass->findMetadata<ResourceDescriptionMetadata>())
        {
            const auto desc = StringView(descData->description());
            if (tagData->color().a > 0 && !desc.empty())
            {
                auto& tag = outTags.emplaceBack();
                tag.color = tagData->color();
                tag.name = StringBuf(desc);
                //tag.baked = resourceClass->findMetadata < >() != nullptr;
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
    static InplaceArray<SpecificClassType<IResourceFactory>, 64> factoryClasses;
    if (factoryClasses.empty())
        RTTI::GetInstance().enumClasses(factoryClasses);

    // if the engine can read this format directly we will have a native class for it
    m_nativeResourceClass = IResource::FindResourceClassByExtension(extension);
    if (m_nativeResourceClass)
    {
        // create the "native" output for the file
        m_description = IResource::GetResourceDescriptionForClass(m_nativeResourceClass);

        // find resource factory
        for (auto factoryClass : factoryClasses)
        {
            if (auto factoryTargetClass = factoryClass->findMetadata<ResourceFactoryClassMetadata>())
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
            IResourceImporter::ListImportableExtensionsForClass(m_nativeResourceClass, importExtensions);

            for (const auto& view : importExtensions)
                m_importExtensions.pushBack(StringBuf(view).toLower());
        }
    }

    // get config entry for the file
    // TODO: move somewhere else
    if (const auto& configGroup = config::FindGroup(TempString("Format.{}", extension)))
        m_description = configGroup->entryValue("Description", m_description);

    // debug log
    TRACE_SPAM("Registered format '{}' for extension '{}'", m_description, m_extension);
}

ManagedFileFormat::~ManagedFileFormat()
{}

ResourcePtr ManagedFileFormat::createEmpty() const
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

        auto classThumbnailImage = LoadImageFromDepotPath(TempString("/engine/interface/thumbnails/{}.png", m_extension));
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
    InplaceArray<SpecificClassType<IResource>, 64> resourceClasses;
    RTTI::GetInstance().enumClasses(resourceClasses);

    // auto cache all native formats
    for (const auto cls : resourceClasses)
    {
        if (const auto ext = IResource::GetResourceExtensionForClass(cls))
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

END_BOOMER_NAMESPACE_EX(ed)


