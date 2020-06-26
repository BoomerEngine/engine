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
#include "base/resources/include/resource.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceFactory.h"

namespace ed
{

    ///---

    static void ExtractTagFromClass(SpecificClassType<res::IResource> resourceClass, base::Array<ManagedFileTag>& outTags)
    {
        if (const auto* tagData = resourceClass->findMetadata<base::res::ResourceTagColorMetadata>())
        {
            if (const auto* descData = resourceClass->findMetadata<base::res::ResourceDescriptionMetadata>())
            {
                const auto desc = base::StringView<char>(descData->description());
                if (tagData->color().a > 0 && !desc.empty())
                {
                    auto& tag = outTags.emplaceBack();
                    tag.color = tagData->color();
                    tag.name = base::StringBuf(desc);
                    tag.baked = resourceClass->findMetadata < base::res::ResourceBakedOnlyMetadata>() != nullptr;
                }
            }
        }
    }

    ///---

    static res::StaticResource<image::Image> resFileIcon("engine/thumbnails/file.png");

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

        // get the native class
        m_nativeResourceClass = res::IResource::FindResourceClassByExtension(extension);

        // hack :)
        if (extension.endsWith(".meta"))
        {
            // load generic manifest data
            m_description = "Cooking manifest";
            m_showInBrowser = false;
        }
        else if (extension.endsWith(".thumb"))
        {
            // load generic manifest data
            m_description = "Thumbnail";
            m_showInBrowser = false;
        }
        else
        {
            // load extra settings
            if (m_nativeResourceClass)
            {
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
            }

            // get config entry for the file
            auto configGroupName = StringID(TempString("Format.{}", extension));
            if (auto configGroup  = Config::GetInstance().findGroup(configGroupName))
                m_description = configGroup->entryValue("Description"_id, m_description);

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

                                if (auto* cookerInstance = (const base::res::IResourceCooker*)cookerClass->defaultObject())
                                    cookerInstance->reportManifestClasses(info.manifestClasses);

                                TRACE_SPAM("Format '{}' is cookable into '{}' via direct cooker '{}'", m_extension, outCookedClass->name(), cookerClass->name());
                            }
                        }
                    }
                }
            }

            // dependent cookers
            for (auto cookerClass : cookerClasses)
            {
                if (auto sourceFormatData = cookerClass->findMetadata<res::ResourceSourceFormatMetadata>())
                {
                    for (auto& sourceClass : sourceFormatData->classes())
                    {
                        bool hasSourceFormat = false;
                        base::Array< base::SpecificClassType<base::res::IResourceManifest>> sourceManifestClasses;
                        for (const auto& info : m_cookableOutputs)
                        {
                            if (info.resoureClass == sourceClass)
                            {
                                sourceManifestClasses = info.manifestClasses;
                                hasSourceFormat = true;
                                break;
                            }
                        }

                        if (hasSourceFormat)
                        {
                            if (auto sourceCookedClass = cookerClass->findMetadata<res::ResourceCookedClassMetadata>())
                            {
                                for (auto outCookedClass : sourceCookedClass->classList())
                                {
                                    auto& info = m_cookableOutputs.emplaceBack();
                                    info.resoureClass = outCookedClass;
                                    info.manifestClasses = sourceManifestClasses;

                                    if (auto* cookerInstance = (const base::res::IResourceCooker*)cookerClass->defaultObject())
                                        cookerInstance->reportManifestClasses(info.manifestClasses);

                                    TRACE_SPAM("Format '{}' is cookable into '{}' via a dependent cooker '{}'", m_extension, outCookedClass->name(), cookerClass->name());
                                }
                            }
                        }
                    }
                }
            }
        }

        // native resource tag
        /*if (m_nativeResourceClass)
            ExtractTagFromClass(m_nativeResourceClass, m_tags);*/

        // cooked format tags
        for (const auto& cookedFormat : m_cookableOutputs)
            ExtractTagFromClass(cookedFormat.resoureClass, m_tags);

        // debug log
        TRACE_SPAM("Registered format '{}' for extension '{}'", m_description, m_extension);
    }

    ManagedFileFormat::~ManagedFileFormat()
    {}

    base::res::ResourcePtr ManagedFileFormat::createEmpty() const
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

            auto fileThumbnailPath = res::ResourcePath(TempString("engine/thumbnails/{}.png", m_extension).c_str());
            auto classThumbnailImage = LoadResource<image::Image>(fileThumbnailPath);
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

    bool ManagedFileFormat::loadableAsType(base::ClassType resourceClass) const
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
    }

    void ManagedFileFormatRegistry::cacheFormats()
    {
        InplaceArray<SpecificClassType<res::IResource>, 64> resourceClasses;
        RTTI::GetInstance().enumClasses(resourceClasses);

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
        }

        DEBUG_CHECK(format->extension() == extension);
        return format;
    }

    void ManagedFileFormatRegistry::deinit()
    {
        m_formatMap.clear();
        m_allFormats.clearPtr();
        m_userCreatableFormats.clear();
    }

    ///---

} // depot


