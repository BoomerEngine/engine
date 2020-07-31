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
#include "base/resource/include/resourceCooker.h"
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
                const auto desc = StringView<char>(descData->description());
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

    static res::StaticResource<image::Image> resFileIcon("/engine/thumbnails/file.png");

    ManagedFileFormat::ManagedFileFormat(StringView<char> extension)
        : m_extension(extension)
        , m_description("Unknown Format")
        , m_nativeResourceClass(nullptr)
        , m_showInBrowser(true)
        , m_hasTypeThumbnail(false)
    {
        // enum classes
        static InplaceArray<SpecificClassType<res::IResourceCooker>, 64> cookerClasses;
        static InplaceArray<SpecificClassType<res::IFactory>, 64> factoryClasses;
        if (cookerClasses.empty() && factoryClasses.empty())
        {
            RTTI::GetInstance().enumClasses(cookerClasses);
            RTTI::GetInstance().enumClasses(factoryClasses);
        }

        // if the engine can read this format directly we will have a native class for it
        m_nativeResourceClass = res::IResource::FindResourceClassByExtension(extension);
        if (m_nativeResourceClass)
        {
            // create the "native" output for the file
            auto& info = m_cookableOutputs.emplaceBack();
            info.resoureClass = m_nativeResourceClass;
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
                InplaceArray<StringView<char>, 20> importExtensions;
                res::IResourceImporter::ListImportableExtensionsForClass(m_nativeResourceClass, importExtensions);

                for (const auto& view : importExtensions)
                    m_importExtensions.pushBack(StringBuf(view).toLower());
            }
        }

        // get config entry for the file
        // TODO: move somewhere else
        if (const auto& configGroup = base::config::FindGroup(TempString("Format.{}", extension)))
            m_description = configGroup->entryValue("Description", m_description);

        // look at cookers if we support this format
        for (auto cookerClass : cookerClasses)
        {
            if (auto sourceFormatData  = cookerClass->findMetadata<res::ResourceSourceFormatMetadata>())
            {
                bool hasExtension = false;
                for (auto& sourceExt : sourceFormatData->extensions())
                {
                    if (0 == extension.caseCmp(sourceExt))
                    {
                        hasExtension = true;
                        break;
                    }
                }

                if (hasExtension)
                {
                    if (auto sourceCookedClass  = cookerClass->findMetadata<res::ResourceCookedClassMetadata>())
                    {
                        for (auto outCookedClass : sourceCookedClass->classList())
                        {
                            auto& info = m_cookableOutputs.emplaceBack();
                            info.resoureClass = outCookedClass;
                            TRACE_SPAM("Format '{}' is cookable into '{}' via direct cooker '{}'", m_extension, outCookedClass->name(), cookerClass->name());
                        }
                    }
                }
            }
        }

        // cooked format tags
        for (const auto& cookedFormat : m_cookableOutputs)
            ExtractTagFromClass(cookedFormat.resoureClass, m_tags);

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

    const image::ImageRef& ManagedFileFormat::thumbnail() const
    {
        // load thumbnail for this type file
        if (!m_thumbnailLoadAttempted)
        {
            m_thumbnailLoadAttempted = true;

            auto classThumbnailImage = LoadResource<image::Image>(TempString("/engine/thumbnails/{}.png", m_extension)); // TODO: move to editor
            if (classThumbnailImage)
            {
                m_thumbnail = classThumbnailImage;
                m_hasTypeThumbnail = true;
            }
            else
            {
                m_thumbnail = resFileIcon.loadAndGet();
            }
        }

        return m_thumbnail;
    }

    bool ManagedFileFormat::loadableAsType(ClassType resourceClass) const
    {
        if (m_nativeResourceClass && m_nativeResourceClass == resourceClass)
            return true;

        for (const auto& output : m_cookableOutputs)
            if (output.resoureClass.is(resourceClass))
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

    const ManagedFileFormat* ManagedFileFormatRegistry::format(StringView<char> extension)
    {
        //auto lock = CreateLock(m_lock);

        auto extKey = extension.calcCRC64();

        // get from map
        ManagedFileFormat* format = nullptr;
        if (!m_formatMap.find(extKey, format))
        {
            format = MemNew(ManagedFileFormat, extension);
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


