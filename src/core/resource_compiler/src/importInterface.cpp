/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importInterface.h"
#include "core/resource/include/tags.h"
#include "core/resource/include/metadata.h"

BEGIN_BOOMER_NAMESPACE()

//--

IResourceImporterInterface::~IResourceImporterInterface()
{}

//--

RTTI_BEGIN_TYPE_CLASS(ResourceImporterConfigurationClassMetadata);
RTTI_END_TYPE();

ResourceImporterConfigurationClassMetadata::ResourceImporterConfigurationClassMetadata()
{

}
//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceImporter);
    RTTI_METADATA(ResourceCookerVersionMetadata).version(1);
    RTTI_METADATA(ResourceImporterConfigurationClassMetadata).configurationClass<ResourceConfiguration>(); // we ALWAYS have a config class
RTTI_END_TYPE();

IResourceImporter::~IResourceImporter()
{}

/// TODO: add sensible caching

static const Array<SpecificClassType<IResourceImporter>>& AllImporterClasses()
{
    static InplaceArray<SpecificClassType<IResourceImporter>, 64> allImporters;
    if (allImporters.empty())
        RTTI::GetInstance().enumClasses(allImporters);
    return allImporters;
}

void IResourceImporter::ListImportableResourceClasses(Array<SpecificClassType<IResource>>& outResourceClasses)
{
    for (const auto& cls : AllImporterClasses())
    {
        if (const auto mtd = cls->findMetadata<ResourceCookedClassMetadata>())
        {
            for (const auto& resoureCls : mtd->classList())
                outResourceClasses.pushBackUnique(resoureCls);
        }
    }
}

bool IResourceImporter::ListImportableResourceClassesForExtension(StringView fileExtension, Array<SpecificClassType<IResource>>& outResourceClasses)
{
    bool somethingAdded = false;
    for (const auto& cls : AllImporterClasses())
    {
        if (const auto mtd = cls->findMetadata<ResourceSourceFormatMetadata>())
        {
            bool hasExtension = false;
            for (const auto& ext : mtd->extensions())
            {
                if (0 == ext.caseCmp(fileExtension))
                {
                    hasExtension = true;
                    break;
                }
            }

            if (hasExtension)
            {
                if (const auto mtd = cls->findMetadata<ResourceCookedClassMetadata>())
                {
                    for (const auto& resoureCls : mtd->classList())
                    {
                        outResourceClasses.pushBackUnique(resoureCls);
                        somethingAdded = true;
                    }
                }
            }
        }
    }

    return somethingAdded;
}

bool IResourceImporter::ListImportConfigurationForExtension(StringView fileExtension, SpecificClassType<IResource> targetClass, SpecificClassType<ResourceConfiguration>& outConfigurationClass)
{
    bool somethingAdded = false;
    for (const auto& cls : AllImporterClasses())
    {
        if (const auto mtd = cls->findMetadata<ResourceSourceFormatMetadata>())
        {
            bool hasExtension = false;
            for (const auto& ext : mtd->extensions())
            {
                if (0 == ext.caseCmp(fileExtension))
                {
                    hasExtension = true;
                    break;
                }
            }

            if (hasExtension)
            {
                if (const auto mtd = cls->findMetadata<ResourceCookedClassMetadata>())
                {
                    for (const auto& resoureCls : mtd->classList())
                    {
                        if (targetClass == resoureCls)
                        {
                            if (const auto mtd = cls->findMetadata<ResourceImporterConfigurationClassMetadata>())
                            {
                                if (mtd->configurationClass())
                                {
                                    outConfigurationClass = mtd->configurationClass();
                                    somethingAdded = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return somethingAdded;
}

bool IResourceImporter::ListImportableExtensionsForClass(SpecificClassType<IResource> resourceClasses, Array<StringView>& outExtensions)
{
    bool somethingAdded = false;
    for (const auto& cls : AllImporterClasses())
    {
        bool hasClass = false;
        if (const auto mtd = cls->findMetadata<ResourceCookedClassMetadata>())
        {
            for (const auto& resoureCls : mtd->classList())
            {
                if (!resourceClasses || resoureCls.is(resourceClasses))
                {
                    hasClass = true;
                    break;
                }
            }
        }

        if (hasClass)
        {
            if (const auto mtd = cls->findMetadata<ResourceSourceFormatMetadata>())
            {
                for (const auto& ext : mtd->extensions())
                {
                    bool addExtension = true;

                    for (const auto& existingExt : outExtensions)
                    {
                        if (0 == ext.caseCmp(existingExt))
                        {
                            addExtension = false;
                            break;
                        }
                    }

                    if (addExtension)
                    {
                        outExtensions.pushBack(ext);
                        somethingAdded = true;
                    }
                }
            }
        }
    }

    return somethingAdded;
}

//--

END_BOOMER_NAMESPACE()
