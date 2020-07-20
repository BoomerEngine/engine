/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importInterface.h"
#include "base/resource/include/resourceTags.h"
#include "base/resource/include/resourceMetadata.h"

namespace base
{
    namespace res
    {

        //--

        RTTI_BEGIN_TYPE_CLASS(ImportQueueFileStatusChangeMessage);
            RTTI_PROPERTY(depotPath).metadata<replication::SetupMetadata>("maxLength:250");
            RTTI_PROPERTY(status).metadata<replication::SetupMetadata>("");
            RTTI_PROPERTY(time).metadata<replication::SetupMetadata>("");
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(ImportQueueFileCancel);
            RTTI_PROPERTY(depotPath).metadata<replication::SetupMetadata>("maxLength:250");
        RTTI_END_TYPE();

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
            RTTI_METADATA(res::ResourceCookerVersionMetadata).version(1);
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
                if (const auto mtd = cls->findMetadata<base::res::ResourceCookedClassMetadata>())
                {
                    for (const auto& resoureCls : mtd->classList())
                        outResourceClasses.pushBackUnique(resoureCls);
                }
            }
        }

        bool IResourceImporter::ListImportableResourceClassesForExtension(StringView<char> fileExtension, Array<SpecificClassType<IResource>>& outResourceClasses)
        {
            bool somethingAdded = false;
            for (const auto& cls : AllImporterClasses())
            {
                if (const auto mtd = cls->findMetadata<base::res::ResourceSourceFormatMetadata>())
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
                        if (const auto mtd = cls->findMetadata<base::res::ResourceCookedClassMetadata>())
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

        bool IResourceImporter::ListImportConfigurationForExtension(StringView<char> fileExtension, SpecificClassType<IResource> targetClass, SpecificClassType<ResourceConfiguration>& outConfigurationClass)
        {
            bool somethingAdded = false;
            for (const auto& cls : AllImporterClasses())
            {
                if (const auto mtd = cls->findMetadata<base::res::ResourceSourceFormatMetadata>())
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
                        if (const auto mtd = cls->findMetadata<base::res::ResourceCookedClassMetadata>())
                        {
                            for (const auto& resoureCls : mtd->classList())
                            {
                                if (targetClass == resoureCls)
                                {
                                    if (const auto mtd = cls->findMetadata<base::res::ResourceImporterConfigurationClassMetadata>())
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

        bool IResourceImporter::ListImportableExtensionsForClass(SpecificClassType<IResource> resourceClasses, Array<StringView<char>>& outExtensions)
        {
            bool somethingAdded = false;
            for (const auto& cls : AllImporterClasses())
            {
                bool hasClass = false;
                if (const auto mtd = cls->findMetadata<base::res::ResourceCookedClassMetadata>())
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
                    if (const auto mtd = cls->findMetadata<base::res::ResourceSourceFormatMetadata>())
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

    } // res
} // base
