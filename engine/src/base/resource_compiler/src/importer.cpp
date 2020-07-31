/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importer.h"
#include "importInterface.h"
#include "importFileService.h"
#include "importSourceAssetRepository.h"
#include "importInterfaceImpl.h"
#include "importFileFingerprint.h"

#include "base/resource/include/resourceMetadata.h"
#include "base/object/include/rttiTypeSystem.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace res
    {

        //--

        IImportDepotChecker::~IImportDepotChecker()
        {}

        //--

        RTTI_BEGIN_TYPE_ENUM(ImportStatus);
            RTTI_ENUM_OPTION(Pending);
            RTTI_ENUM_OPTION(Checking);
            RTTI_ENUM_OPTION(Processing);
            RTTI_ENUM_OPTION(UpToDate);
            RTTI_ENUM_OPTION(NotUpToDate);
            RTTI_ENUM_OPTION(NotSupported);
            RTTI_ENUM_OPTION(MissingAssets);
            RTTI_ENUM_OPTION(InvalidAssets);
            RTTI_ENUM_OPTION(NewAssetImported);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(ImportJobInfo);
            RTTI_PROPERTY(assetFilePath);
            RTTI_PROPERTY(depotFilePath);
            RTTI_PROPERTY(externalConfig);
            RTTI_PROPERTY(userConfig);
        RTTI_END_TYPE();

        //--

        Importer::Importer(SourceAssetRepository* assets, const IImportDepotChecker* depotChecker)
            : m_assets(assets)
            , m_depotChecker(depotChecker)
        {
            buildClassMap();
        }

        Importer::~Importer()
        {}
            
        ResourceConfigurationPtr Importer::compileFinalImportConfiguration(const StringBuf& depotPath, const Metadata& metadata, const ResourceConfigurationPtr& newUserConfiguration) const
        {
            // get the extension requested resource class for given depot extension
            const auto depotExtension = depotPath.view().afterLast(".");
            const auto targetResourceClass = IResource::FindResourceClassByExtension(depotExtension);
            if (!targetResourceClass)
                return nullptr;

            // find importer class
            SpecificClassType<IResourceImporter> importerClass;
            const auto& sourcePath = metadata.importDependencies[0].importPath;
            if (!findBestImporter(sourcePath, targetResourceClass, importerClass))
                return nullptr;

            // get the resource configuration class needed to import this resource
            auto resourceConfigurationClass = importerClass->findMetadata<ResourceImporterConfigurationClassMetadata>()->configurationClass();
            DEBUG_CHECK_EX(resourceConfigurationClass, "Invalid resource configuration class");
            if (!resourceConfigurationClass)
                resourceConfigurationClass = ResourceConfiguration::GetStaticClass();

            // load the asset depot configuration for the resource
            auto folderBaseConfig = m_assets->compileBaseResourceConfiguration(sourcePath, resourceConfigurationClass);

            // if we were given extra config how to import this resource then use it
            ResourceConfigurationPtr baseConfig;
            if (metadata.importBaseConfiguration && metadata.importBaseConfiguration->is(resourceConfigurationClass))
            {
                baseConfig = CloneObject(metadata.importBaseConfiguration); // keep only delta properties
                baseConfig->rebase(folderBaseConfig); // apply delta properties on top of what we have in the folder
            }
            else
            {
                baseConfig = resourceConfigurationClass.create(); // create empty one
                baseConfig->rebase(folderBaseConfig); // apply delta properties on top of what we have in the folder
            }

            // apply user config
            ResourceConfigurationPtr importConfig;
            if (newUserConfiguration && newUserConfiguration->is(resourceConfigurationClass))
            {
                importConfig = CloneObject(newUserConfiguration); // keep only delta properties from user config
                importConfig->rebase(baseConfig); // apply delta properties on top of what we have as the base
            }
            else if (metadata.importUserConfiguration && metadata.importUserConfiguration->is(resourceConfigurationClass))
            {
                importConfig = CloneObject(metadata.importUserConfiguration); // keep only delta properties from user config
                importConfig->rebase(baseConfig); // apply delta properties on top of what we have as the base
            }
            else
            {
                importConfig = resourceConfigurationClass.create(); // create empty one
                importConfig->rebase(baseConfig); // apply delta properties on top of what we have as the base
            }

            // done
            return importConfig;
        }

        static uint64_t CalcConfigurationKey(const ResourceConfiguration* cfg)
        {
            if (cfg)
            {
                CRC64 crc;
                cfg->computeConfigurationKey(crc);
                return crc;
            }

            return 0;
        }

        ImportStatus Importer::checkStatus(const StringBuf& depotPath, const Metadata& metadata, const ResourceConfigurationPtr& newUserConfiguration, IProgressTracker* progress) const
        {
            // check if import class still exists
            if (metadata.cookerClass == nullptr)
                return ImportStatus::NotSupported;

            // check that cooker class has still the same version
            const auto currentCookerVersion = metadata.cookerClass->findMetadata<res::ResourceCookerVersionMetadata>();
            if (currentCookerVersion->version() != metadata.cookerClassVersion)
                return ImportStatus::NotUpToDate;
            
            // check that resource serialization class has still the same version
            /*const auto currentCookerVersion = metadata.cookerClass->findMetadata<res::ResourceCookerVersionMetadata>();
            if (currentCookerVersion->version() != metadata.cookerClassVersion)
                return ImportStatus::NotUpToDate;*/

            // not imported from any files
            if (metadata.importDependencies.empty())
                return ImportStatus::NotImportable;

            // check asset dependencies
            for (const auto& dep : metadata.importDependencies)
            {
                ImportFileFingerprint fingerprint(dep.crc);
                const auto ret = m_assets->checkFileStatus(dep.importPath, dep.timestamp, fingerprint, progress);

                switch (ret)
                {
                    case SourceAssetStatus::ContentChanged: 
                        return ImportStatus::NotUpToDate;

                    case SourceAssetStatus::Missing: 
                        return ImportStatus::MissingAssets;

                    case SourceAssetStatus::ReadFailure: 
                        return ImportStatus::InvalidAssets;

                    case SourceAssetStatus::Canceled: 
                        return ImportStatus::Canceled;
                }
            }

            // build import configuration
            {
                const auto importConfiguration = compileFinalImportConfiguration(depotPath, metadata, newUserConfiguration);
                if (!importConfiguration)
                    return ImportStatus::NotSupported;

                // check configuration
                const auto originalConfigurationKey = CalcConfigurationKey(metadata.importFullConfiguration);
                const auto currentConfigurationKey = CalcConfigurationKey(importConfiguration);
                if (originalConfigurationKey != currentConfigurationKey)
                    return ImportStatus::NotUpToDate;
            }

            // looks up to date
            return ImportStatus::UpToDate;
        }



        ImportStatus Importer::importResource(const ImportJobInfo& info, const IResource* existingData, ResourcePtr& outImportedResource, IProgressTracker* progress /*= nullptr*/) const
        {
            // no import path
            DEBUG_CHECK_EX(!info.assetFilePath.empty() && !info.depotFilePath.empty(), "Invalid call");
            if (info.assetFilePath.empty() || info.depotFilePath.empty())
                return ImportStatus::NotSupported;

            // get the extension requested resource class for given depot extension
            const auto depotExtension = info.depotFilePath.view().afterLast(".");
            const auto targetResourceClass = IResource::FindResourceClassByExtension(depotExtension);
            if (!targetResourceClass)
            {
                TRACE_WARNING("Unrecognized resource extension '{}' in '{}'", depotExtension, info.depotFilePath);
                return ImportStatus::NotSupported;
            }

            // find importer class
            SpecificClassType<IResourceImporter> importerClass;
            if (!findBestImporter(info.assetFilePath, targetResourceClass, importerClass))
            {
                TRACE_WARNING("No cooker found capable of cooking '{}' into {}", info.assetFilePath, targetResourceClass->name());
                return ImportStatus::NotSupported;
            }

            // get the resource configuration class needed to import this resource
            auto resourceConfigurationClass = importerClass->findMetadata<ResourceImporterConfigurationClassMetadata>()->configurationClass();
            DEBUG_CHECK_EX(resourceConfigurationClass, "Invalid resource configuration class");
            if (!resourceConfigurationClass)
                resourceConfigurationClass = ResourceConfiguration::GetStaticClass();

            // load the asset depot configuration for the resource
            auto folderBaseConfig = m_assets->compileBaseResourceConfiguration(info.assetFilePath, resourceConfigurationClass);

            // if we were given extra config how to import this resource then use it
            ResourceConfigurationPtr baseConfig;
            if (info.externalConfig && info.externalConfig->is(resourceConfigurationClass))
            {
                baseConfig = CloneObject(info.externalConfig); // keep only delta properties
                baseConfig->rebase(folderBaseConfig); // apply delta properties on top of what we have in the folder
            }
            else
            {
                baseConfig = resourceConfigurationClass.create(); // create empty one
                baseConfig->rebase(folderBaseConfig); // apply delta properties on top of what we have in the folder
            }

            // apply user config
            ResourceConfigurationPtr importConfig;
            if (info.userConfig && info.userConfig->is(resourceConfigurationClass))
            {
                importConfig = CloneObject(info.userConfig); // keep only delta properties from user config
                importConfig->rebase(baseConfig); // apply delta properties on top of what we have as the base
            }
            else
            {
                importConfig = resourceConfigurationClass.create(); // create empty one
                importConfig->rebase(baseConfig); // apply delta properties on top of what we have as the base
            }

            // create an instance of importer for this job
            auto importer = importerClass.create();

            // TODO: add a change for importer to test resource and check if it's up to date without the need for reimporting

            // create import interface
            LocalImporterInterface importerInterface(m_assets, m_depotChecker, existingData, info.assetFilePath, info.depotFilePath, ResourceMountPoint(), progress, importConfig);
            const auto importedResource = importer->importResource(importerInterface);

            // regardless if we produced asset or not never return anything if we got canceled
            if (progress->checkCancelation())
                return ImportStatus::Canceled;

            // no output
            if (!importedResource)
            {
                TRACE_ERROR("Failed to import resource '{}' from '{}'", info.depotFilePath, info.assetFilePath);
                return ImportStatus::InvalidAssets;
            }

            // extract metadata
            auto metadata = importerInterface.buildMetadata();

            // TODO: thumbnail
            // TODO: additional asset information (tags, etc)
                
            // fill in additional metadata information
            metadata->cookerClass = importerClass;
            metadata->cookerClassVersion = importerClass->findMetadata<res::ResourceCookerVersionMetadata>()->version();
            metadata->resourceClassVersion = 0;// importedResource->findMetadata<res::ResourceCookerVersionMetadata>()->version();

            // update revision number
            if (existingData && existingData->metadata())
                metadata->internalRevision = existingData->metadata()->internalRevision + 1;
            else
                metadata->internalRevision = 1; // non zero for imported stuff

            // remember the configurations used to bake the resource
            metadata->importBaseConfiguration = baseConfig;
            baseConfig->parent(metadata);

            // remember the final configuration as well
            metadata->importUserConfiguration = importConfig;
            importConfig->parent(metadata);

            // build the full import configuration
            metadata->importFullConfiguration = CloneObject(importConfig);
            metadata->importFullConfiguration->rebase(baseConfig);
            metadata->importFullConfiguration->detach();
            metadata->importFullConfiguration->parent(metadata);

            // store metadata in the imported object
            importedResource->metadata(metadata);

            // we are done
            outImportedResource = importedResource;
            return ImportStatus::NewAssetImported;
        }

        //--

        void Importer::buildClassMap()
        {
            // get all importer classes
            InplaceArray<SpecificClassType<IResourceImporter>, 32> importerClasses;
            RTTI::GetInstance().enumClasses(importerClasses);
            TRACE_INFO("Found {} importer classes", importerClasses.size());

            // extract supported formats
            for (const auto cls : importerClasses)
            {
                auto extensionMetadata = cls->findMetadata<ResourceSourceFormatMetadata>();
                auto targetClassMetadata = cls->findMetadata<ResourceCookedClassMetadata>();
                if (extensionMetadata && targetClassMetadata)
                {
                    // check that class is valid
                    for (const auto targetResourceClass : targetClassMetadata->classList())
                    {
                        // create entry for each reported extension
                        for (const auto& ext : extensionMetadata->extensions())
                        {
                            if (!ext.empty())
                            {
                                auto extString = StringBuf(ext).toLower();
                                auto& table = m_importableClassesPerExtension[extString];

                                // make sure we don't have duplicates
                                bool addEntry = true;
                                for (const auto& entry : table)
                                {
                                    if (entry.targetResourceClass == targetResourceClass)
                                    {
                                        TRACE_ERROR("Duplicated import entry for extension '{}' into class '{}. Both importer '{}' and '{}' can service it", 
                                            extString, targetResourceClass->name(), cls->name(), entry.importerClass->name());
                                        addEntry = false;
                                        break;
                                    }
                                }

                                // register importer class as usable for importing given type of resource
                                if (addEntry)
                                {
                                    auto& entry = table.emplaceBack();
                                    entry.importerClass = cls;
                                    entry.targetResourceClass = targetResourceClass;
                                    TRACE_INFO("Found importer '{}' for loading '{}' into '{}'", cls->name(), extString, targetResourceClass->name());
                                }
                            }
                        }
                    }
                    /*else if (targetClassMetadata->nativeClass())
                    {
                        TRACE_WARNING("Importer class '{}' cannot use class '{}' as output", cls->name(), targetClassMetadata->nativeClass()->name());
                    }
                    else
                    {
                        TRACE_WARNING("Importer class '{}' has invalid output class specified", cls->name());
                    }*/
                }
                else
                {
                    TRACE_WARNING("Importer class '{}' has invalid metadata (missing ResourceSourceFormatMetadata/ResourceCookedClassMetadata)", cls->name());
                }
            }
        }

        bool Importer::findBestImporter(StringView<char> assetFilePath, SpecificClassType<IResource> targetResourceClass, SpecificClassType<IResourceImporter>& outImporterClass) const
        {
            // find the asset extensions
            const auto ext = StringBuf(assetFilePath.afterLast(".")).toLower();

            // get entry for that resource type
            if (const auto* table = m_importableClassesPerExtension.find(ext))
            {
                // look for importer that can service that class DIRECTLY
                for (const auto& entry : *table)
                {
                    if (entry.targetResourceClass == targetResourceClass)
                    {
                        outImporterClass = entry.importerClass;
                        return true;
                    }
                }
            }

            // extension/class combination is not supported
            return false;
        }

        //--

    } // res
} // base
