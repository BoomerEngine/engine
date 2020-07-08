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

#include "base/resource/include/resourceMetadata.h"
#include "base/object/include/rttiTypeSystem.h"
#include "importInterfaceImpl.h"

namespace base
{
    namespace res
    {

        //--

        RTTI_BEGIN_TYPE_CLASS(ImportJobInfo);
            RTTI_PROPERTY(assetFilePath);
            RTTI_PROPERTY(depotFilePath);
            RTTI_PROPERTY(config);
        RTTI_END_TYPE();

        //--

        Importer::Importer(SourceAssetRepository* assets)
            : m_assets(assets)
        {
            buildClassMap();
        }

        Importer::~Importer()
        {}
            
        ImportStatus Importer::checkStatus(const Metadata& metadata) const
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

            // check asset dependencies
            for (const auto& dep : metadata.importDependencies)
            {
                uint64_t currentCRC = 0; // TODO: replace with "PFFF" - probabilistic fingerprint
                if (!m_assets->fileExists(dep.importPath, &currentCRC))
                    return ImportStatus::MissingAssets;

                if (currentCRC != dep.crc)
                    return ImportStatus::NotUpToDate;
            }

            // looks up to date
            return ImportStatus::UpToDate;
        }

        ImportStatus Importer::importResource(const ImportJobInfo& info, const IResource* existingData, ResourcePtr& outImportedResource, IProgressTracker* progress /*= nullptr*/) const
        {
            // no import path
            if (info.assetFilePath.empty() || info.depotFilePath.empty())
                return ImportStatus::NotSupported;

            // get the extension requested resource class for given depot extension
            const auto depotExtension = info.depotFilePath.view().afterLast(".");
            const auto targetResourceClass = IResource::FindResourceClassByExtension(depotExtension);
            if (!targetResourceClass)
                return ImportStatus::NotSupported;

            // find importer class
            SpecificClassType<IResourceImporter> importerClass;
            if (findBestImporter(info.assetFilePath, targetResourceClass, importerClass))
                return ImportStatus::NotSupported;

            // create an instance of importer for this job
            auto importer = importerClass.create();

            // TODO: add a change for importer to test resource and check if it's up to date without the need for reimporting


            // create import interface
            LocalImporterInterface importerInterface(m_assets, existingData, info.assetFilePath, ResourcePath(info.depotFilePath), ResourceMountPoint(), progress, info.config);
            const auto importedResource = importer->importResource(importerInterface);
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
                    auto targetResourceClass = targetClassMetadata->nativeClass().cast<IResource>();
                    if (targetResourceClass && !targetResourceClass->isAbstract())
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
                    else if (targetClassMetadata->nativeClass())
                    {
                        TRACE_WARNING("Importer class '{}' cannot use class '{}' as output", cls->name(), targetClassMetadata->nativeClass()->name());
                    }
                    else
                    {
                        TRACE_WARNING("Importer class '{}' has invalid output class specified", cls->name());
                    }
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
