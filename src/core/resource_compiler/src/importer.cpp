/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"

#include "fingerprint.h"

#include "importer.h"
#include "importInterface.h"
#include "importInterfaceImpl.h"

#include "sourceAssetService.h"

#include "core/object/include/rttiTypeSystem.h"
#include "core/resource/include/metadata.h"
#include "core/resource/include/tags.h"
#include "core/containers/include/path.h"
#include "core/resource/include/depot.h"
#include "core/resource/include/fileLoader.h"

BEGIN_BOOMER_NAMESPACE()

//--

// local depot based resource loader
class LocalDepotBasedLoader : public IImportDepotLoader
{
public:
    LocalDepotBasedLoader()
    {
        m_depot = GetService<DepotService>();
    }

    virtual ResourceID queryResourceID(StringView depotPath) const override final
    {
        DEBUG_CHECK_RETURN_EX_V(!depotPath.empty(), "Invalid depot path", ResourceID());

        const auto metadataLoadPath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);

        ResourceMetadataPtr metadata;
        if (!m_depot->loadFileToXMLObject(depotPath, metadata))
            return ResourceID();

        DEBUG_CHECK_RETURN_EX_V(!metadata->ids.empty(), "Loaded metadata with no IDs", ResourceID());

        return metadata->ids.front(); // use the most recent one
    }

    virtual ResourceMetadataPtr loadExistingMetadata(StringView depotPath) const override final
    {
        DEBUG_CHECK_RETURN_EX_V(!depotPath.empty(), "Invalid depot path", nullptr);
        DEBUG_CHECK_RETURN_EX_V(depotPath.endsWith(ResourceMetadata::FILE_EXTENSION), "Invalid metadata path", nullptr);

        ResourceMetadataPtr metadata;
        m_depot->loadFileToXMLObject(depotPath, metadata);

        return metadata;
    }

    virtual ResourcePtr loadExistingResource(StringView depotPath) const override final
    {
        DEBUG_CHECK_RETURN_EX_V(!depotPath.empty(), "Invalid depot path", nullptr);
        DEBUG_CHECK_RETURN_EX_V(depotPath.endsWith(IResource::FILE_EXTENSION), "Invalid resource path", nullptr);

        if (const auto fileReader = m_depot->createFileAsyncReader(depotPath))
        {
            FileLoadingContext context;
            FileLoadingResult result;
            if (LoadFile(fileReader, context, result))
                return result.root<IResource>();
        }

        return nullptr;
    }

    virtual bool fileExists(StringView depotPath) const override final
    {
        DEBUG_CHECK_RETURN_EX_V(depotPath.endsWith(IResource::FILE_EXTENSION), "Invalid resource path", nullptr);

        TimeStamp timestamp;
        return m_depot->queryFileTimestamp(depotPath, timestamp);
    }

    virtual bool findFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath, ResourceID* outFoundResourceID) const override final
    {
        const auto searchFileName = ReplaceExtension(fileName, ResourceMetadata::FILE_EXTENSION);

        StringBuf foundDepotPath;
        if (m_depot->findFile(depotPath, searchFileName, maxDepth, foundDepotPath))
        {
            if (const auto id = queryResourceID(foundDepotPath))
            {
                outFoundFileDepotPath = foundDepotPath;

                if (outFoundResourceID)
                    *outFoundResourceID = id;

                return true;
            }
        }

        return false;
    }

private:
    DepotService* m_depot = nullptr;
};

const IImportDepotLoader& IImportDepotLoader::GetGlobalDepotLoader()
{
    static LocalDepotBasedLoader theLocalDepot;
    return theLocalDepot;
}

//--

RTTI_BEGIN_TYPE_ENUM(ImportStatus);
    RTTI_ENUM_OPTION(Pending);
    RTTI_ENUM_OPTION(Checking);
    RTTI_ENUM_OPTION(Processing);
    RTTI_ENUM_OPTION(Failed);
    RTTI_ENUM_OPTION(FinishedUpTodate);
    RTTI_ENUM_OPTION(FinishedNewContent);
RTTI_END_TYPE();

//--

Importer::Importer(const IImportDepotLoader* customDepot)
{
    if (customDepot)
        m_depot = customDepot;
    else
        m_depot = &IImportDepotLoader::GetGlobalDepotLoader();

    buildClassMap();
}

Importer::~Importer()
{}

SpecificClassType<IResourceImporter> Importer::findImporterClass(StringView sourceAssetReferencePath, ResourceClass importedResourceClass) const
{
    DEBUG_CHECK_RETURN_EX_V(importedResourceClass, "Invalid resource class", nullptr);
    DEBUG_CHECK_RETURN_EX_V(sourceAssetReferencePath, "Invalid asset path", nullptr);

    SpecificClassType<IResourceImporter> importerClass;
    findBestImporter(sourceAssetReferencePath, importedResourceClass, importerClass);
    return importerClass;
}

SpecificClassType<ResourceConfiguration> Importer::findConfigurationClass(StringView sourceAssetReferencePath, ResourceClass importedResourceClass) const
{
    DEBUG_CHECK_RETURN_EX_V(importedResourceClass, "Invalid resource class", ResourceConfiguration::GetStaticClass());
    DEBUG_CHECK_RETURN_EX_V(sourceAssetReferencePath, "Invalid asset path", ResourceConfiguration::GetStaticClass());

    // find importer class
    SpecificClassType<IResourceImporter> importerClass;
    if (!findBestImporter(sourceAssetReferencePath, importedResourceClass, importerClass))
        return nullptr;

    // get the resource configuration class needed to import this resource
    auto resourceConfigurationClass = importerClass->findMetadata<ResourceImporterConfigurationClassMetadata>()->configurationClass();
    DEBUG_CHECK_RETURN_EX_V(resourceConfigurationClass, "Invalid resource configuration class", ResourceConfiguration::GetStaticClass());
    if (!resourceConfigurationClass)
        resourceConfigurationClass = ResourceConfiguration::GetStaticClass();

    // use default so we always have something
    return resourceConfigurationClass;
}

ResourceConfigurationPtr Importer::compileFinalImportConfiguration(StringView sourceAssetPath, ResourceClass importedResourceClass, const ResourceConfiguration* baseConfiguration /*= nullptr*/, const ResourceConfiguration* userConfiguration /*= nullptr*/) const
{
    const auto cls = findConfigurationClass(sourceAssetPath, importedResourceClass);
    DEBUG_CHECK_RETURN_EX_V(cls, "No configuration class found", nullptr);

    // create empty config with default values
    auto config = cls->create<ResourceConfiguration>();
    //config->setupDefaultImportMetadata(); moved to base so it's not changed every time

    // compile the basic configuration based on the source file selected for importing
    GetService<SourceAssetService>()->collectImportConfiguration(config, sourceAssetPath);

    // apply the base configuration
    if (baseConfiguration)
    {
        config->rebase(baseConfiguration);
        config->detach(true);
    }

    // apply user config
    if (userConfiguration)
    {
        config->rebase(userConfiguration);
        config->detach(true);
    }

    DEBUG_CHECK_EX(config->base() == nullptr, "Config should be rebased");
    return config;
}

void Importer::checkStatus(ImportExtendedStatusFlags& outStatusFlags, const ResourceMetadata* metadata, const ResourceConfiguration* newUserConfiguration, IProgressTracker* progress) const
{
    // invalid metadata
    if (!metadata)
    {
        outStatusFlags |= ImportExtendedStatusBit::MetadataInvalid;
        return;
    }

    // not imported from any files
    if (metadata->importDependencies.empty())
    {
        outStatusFlags |= ImportExtendedStatusBit::NotImported;
        return;
    }

    // check asset dependencies
    bool valid = true;
    for (const auto& dep : metadata->importDependencies)
    {
        SourceAssetFingerprint fingerprint(dep.crc);
        const auto ret = GetService<SourceAssetService>()->checkFileStatus(dep.importPath, dep.timestamp, fingerprint, progress);

        switch (ret)
        {
            case SourceAssetStatus::ContentChanged:
                outStatusFlags |= ImportExtendedStatusBit::SourceFileChanged;
                valid = false;
                break;

            case SourceAssetStatus::Missing:
                outStatusFlags |= ImportExtendedStatusBit::SourceFileMissing;
                valid = false;
                break;

            case SourceAssetStatus::UpToDate:
                break;

            default:
                outStatusFlags |= ImportExtendedStatusBit::SourceFileNotReadable;
                valid = false;
                break;
        }
    }

    // resource class does not exist, importing is not supported
    if (metadata->resourceClassType)
    {
        const auto currentClassVersion = metadata->resourceClassType->findMetadataRef<ResourceDataVersionMetadata>().version();
        if (currentClassVersion != metadata->resourceClassVersion)
        {
            outStatusFlags |= ImportExtendedStatusBit::ResourceVersionChanged;
            valid = false;
        }
    }
    else
    {
        outStatusFlags |= ImportExtendedStatusBit::ResourceClassMissing;
        valid = false;
    }

    // check importer class existence, it's not a problem on its own if it's missing
    if (metadata->importerClassType)
    {
        const auto currentClassVersion = metadata->importerClassType->findMetadataRef<ResourceCookerVersionMetadata>().version();
        if (currentClassVersion != metadata->importerClassVersion)
        {
            outStatusFlags |= ImportExtendedStatusBit::ImporterVersionChanged;
            valid = false;
        }
    }
    else
    {
        outStatusFlags |= ImportExtendedStatusBit::ImporterClassMissing;
    }

    // if we have no configuration or the class changed it's also a problem
    if (metadata->importFullConfiguration)
    {
        const auto& sourceFilePath = metadata->importDependencies[0].importPath;

        const auto newImportConfiguration = compileFinalImportConfiguration(
            sourceFilePath,
            metadata->resourceClassType,
            metadata->importBaseConfiguration,
            newUserConfiguration ? newUserConfiguration : metadata->importUserConfiguration);

        if (!newImportConfiguration || !newImportConfiguration->compare(metadata->importFullConfiguration))
        {
            outStatusFlags |= ImportExtendedStatusBit::ConfigurationChanged;
            valid = false;
        }
    }
    else
    {
        outStatusFlags |= ImportExtendedStatusBit::ConfigurationChanged;
        valid = false;
    }

    // return final status, the details are passed via outStatusFlags
    if (valid)
        outStatusFlags |= ImportExtendedStatusBit::UpToDate;
}

bool Importer::importResource(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress /*= nullptr*/) const
{
    ScopeTimer timer;

    DEBUG_CHECK_RETURN_EX_V(info.assetFilePath, "Invalid import job", false);
    DEBUG_CHECK_RETURN_EX_V(info.depotFilePath, "Invalid import job", false);
    DEBUG_CHECK_RETURN_EX_V(info.resourceClass, "Invalid import job", false);
    DEBUG_CHECK_RETURN_EX_V(info.depotFilePath.endsWith(IResource::FILE_EXTENSION), "Resource file does not follow the convention", false);

    bool upToDate = false;
    if (!import_checkUpToDate(info, outResult, progress, upToDate))
        return true;

    if (upToDate && !info.force)
        return true;

    if (!import_assignID(info, outResult, progress))
        return false;

    if (!import_findImporter(info, outResult, progress))
        return false;

    if (!import_updateConfig(info, outResult, progress))
        return false;

    if (!import_doActualImport(info, outResult, progress))
        return false;

    {
        auto& message = outResult.messages.emplaceBack();
        message.type = logging::OutputLevel::Info;
        message.message = StringBuf(TempString("Imported resource revision {} in {}", outResult.metadata->internalRevision, timer));
    }

    return true;
}

bool Importer::import_checkUpToDate(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress, bool& outUpToDate) const
{
    // load existing metadata
    const auto metadataPath = ReplaceExtension(info.depotFilePath, ResourceMetadata::FILE_EXTENSION);
    outResult.metadata = m_depot->loadExistingMetadata(metadataPath);

    // if we have metadata we can check if resource is up to date
    if (outResult.metadata)
    {
        // check if file is update to date
        checkStatus(outResult.extendedStatusFlags, outResult.metadata, info.userConfig);
        if (outResult.extendedStatusFlags.test(ImportExtendedStatusBit::NotImported))
        {
            auto& message = outResult.messages.emplaceBack();
            message.type = logging::OutputLevel::Warning;
            message.message = StringBuf(TempString("File has metadata but was was not imported in the first place"));

            return false; // we can't import file that 
        }

        if (outResult.extendedStatusFlags.test(ImportExtendedStatusBit::UpToDate))
        {
            outUpToDate = true;

            auto& message = outResult.messages.emplaceBack();
            message.type = logging::OutputLevel::Info;
            message.message = StringBuf(TempString("File is up to date"));
        }
        else
        {
            if (outResult.extendedStatusFlags.test(ImportExtendedStatusBit::ConfigurationChanged))
            {
                auto& message = outResult.messages.emplaceBack();
                message.type = logging::OutputLevel::Info;
                message.message = StringBuf(TempString("File import configuration has changed"));
            }
            if (outResult.extendedStatusFlags.test(ImportExtendedStatusBit::SourceFileChanged))
            {
                auto& message = outResult.messages.emplaceBack();
                message.type = logging::OutputLevel::Info;
                message.message = StringBuf(TempString("Sources assets have changed"));
            }
            if (outResult.extendedStatusFlags.test(ImportExtendedStatusBit::ImporterClassMissing))
            {
                auto& message = outResult.messages.emplaceBack();
                message.type = logging::OutputLevel::Warning;
                message.message = StringBuf(TempString("Importer class used for the original import is missing"));
            }
            if (outResult.extendedStatusFlags.test(ImportExtendedStatusBit::ResourceClassMissing))
            {
                auto& message = outResult.messages.emplaceBack();
                message.type = logging::OutputLevel::Warning;
                message.message = StringBuf(TempString("Resource class used for the original import is missing"));
            }
            if (outResult.extendedStatusFlags.test(ImportExtendedStatusBit::ImporterVersionChanged))
            {
                auto& message = outResult.messages.emplaceBack();
                message.type = logging::OutputLevel::Info;
                message.message = StringBuf(TempString("Importer has a new version"));
            }
            if (outResult.extendedStatusFlags.test(ImportExtendedStatusBit::ResourceVersionChanged))
            {
                auto& message = outResult.messages.emplaceBack();
                message.type = logging::OutputLevel::Info;
                message.message = StringBuf(TempString("Resource data format has a new version"));
            }
            if (outResult.extendedStatusFlags.test(ImportExtendedStatusBit::SourceFileMissing))
            {
                auto& message = outResult.messages.emplaceBack();
                message.type = logging::OutputLevel::Warning;
                message.message = StringBuf(TempString("Original source files are missing"));
            }
            if (outResult.extendedStatusFlags.test(ImportExtendedStatusBit::SourceFileNotReadable))
            {
                auto& message = outResult.messages.emplaceBack();
                message.type = logging::OutputLevel::Warning;
                message.message = StringBuf(TempString("Original source files are invalid"));
            }
        }
    }
    else
    {
        // create new metadata
        outResult.metadata = RefNew<ResourceMetadata>();
    }

    return true;
}

bool Importer::import_assignID(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress) const
{
    if (info.id)
    {
        if (!outResult.metadata->ids.contains(info.id))
        {
            outResult.metadata->ids.pushBack(info.id);

            auto& message = outResult.messages.emplaceBack();
            message.type = logging::OutputLevel::Info;
            message.message = StringBuf(TempString("A new ID '{}' will be assigned to the resource", info.id));
        }
    }
    else if (outResult.metadata->ids.empty())
    {
        const auto newID = ResourceID::Create();
        outResult.metadata->ids.pushBack(newID);

        auto& message = outResult.messages.emplaceBack();
        message.type = logging::OutputLevel::Info;
        message.message = StringBuf(TempString("Resource has no ID, automatic ID '{}' will be assigned", info.id));
    }

    return true;
}

bool Importer::import_findImporter(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress) const
{
    // find importer class
    if (!findBestImporter(info.assetFilePath, info.resourceClass, outResult.importerClass))
    {
        auto& message = outResult.messages.emplaceBack();
        message.type = logging::OutputLevel::Error;
        message.message = StringBuf(TempString("No cooker found capable of cooking '{}' into {}", info.assetFilePath, info.resourceClass));
        return false;
    }

    // TODO: additional quick checks ? (ie. importer->canImport()?)


    // update class
    const auto currentResourceVersion = info.resourceClass->findMetadataRef<ResourceDataVersionMetadata>().version();
    if (outResult.metadata->resourceClassType)
    {
        if (outResult.metadata->resourceClassType != info.resourceClass)
        {
            auto& message = outResult.messages.emplaceBack();
            message.type = logging::OutputLevel::Info;
            message.message = StringBuf(TempString("Resource will change class from '{}' to '{}'", outResult.metadata->resourceClassType, info.resourceClass));
        }

        if (outResult.metadata->resourceClassVersion != currentResourceVersion)
        {
            auto& message = outResult.messages.emplaceBack();
            message.type = logging::OutputLevel::Info;
            message.message = StringBuf(TempString("Resource will change version from {} to {}", outResult.metadata->resourceClassVersion, currentResourceVersion));
        }
    }

    // update importer
    const auto currentImporterVersion = outResult.importerClass->findMetadataRef<ResourceCookerVersionMetadata>().version();
    if (outResult.metadata->importerClassType)
    {
        if (outResult.metadata->importerClassType != outResult.importerClass)
        {
            auto& message = outResult.messages.emplaceBack();
            message.type = logging::OutputLevel::Warning;
            message.message = StringBuf(TempString("Importer class changed from '{}' to '{}'", outResult.metadata->importerClassType, outResult.importerClass));
        }

        if (outResult.metadata->importerClassVersion != currentImporterVersion)
        {
            auto& message = outResult.messages.emplaceBack();
            message.type = logging::OutputLevel::Info;
            message.message = StringBuf(TempString("Importer version changed from {} to {}", outResult.metadata->importerClassVersion, currentImporterVersion));
        }
    }

    // update classes and version
    outResult.metadata->resourceClassType = info.resourceClass;
    outResult.metadata->resourceClassVersion = currentResourceVersion;
    outResult.metadata->importerClassType = outResult.importerClass;
    outResult.metadata->importerClassVersion = currentImporterVersion;

    return true;
}

bool Importer::import_updateConfig(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress) const
{
    auto metadata = outResult.metadata;

    // get the configuration class needed for import
    const auto configurationClass = outResult.importerClass->findMetadataRef<ResourceImporterConfigurationClassMetadata>().configurationClass();
    ASSERT(configurationClass);

    //---

    if (info.baseConfig)
    {
        metadata->importBaseConfiguration = CloneObject(info.baseConfig);
        metadata->importBaseConfiguration->parent(metadata);
    }

    if (outResult.metadata->importBaseConfiguration)
    {
        if (!outResult.metadata->importBaseConfiguration->is(configurationClass))
        {
            auto& message = outResult.messages.emplaceBack();
            message.type = logging::OutputLevel::Warning;
            message.message = StringBuf(TempString("Resource base configuration class changed from '{}' to '{}'", metadata->importBaseConfiguration->cls(), configurationClass));

            auto oldConfig = metadata->importBaseConfiguration;
            metadata->importBaseConfiguration = configurationClass->create<ResourceConfiguration>();
            metadata->importBaseConfiguration->parent(metadata);
            metadata->importBaseConfiguration->rebase(oldConfig);
            metadata->importBaseConfiguration->detach();
        }
    }
    else
    {
        metadata->importBaseConfiguration = configurationClass->create<ResourceConfiguration>();
        metadata->importBaseConfiguration->parent(metadata);
        metadata->importBaseConfiguration->setupDefaultImportMetadata();
    }

    //---

    if (info.userConfig)
    {
        metadata->importUserConfiguration = CloneObject(info.userConfig);
        metadata->importUserConfiguration->parent(metadata);
    }

    if (outResult.metadata->importUserConfiguration)
    {
        if (!outResult.metadata->importUserConfiguration->is(configurationClass))
        {
            auto& message = outResult.messages.emplaceBack();
            message.type = logging::OutputLevel::Warning;
            message.message = StringBuf(TempString("Resource user configuration class changed from '{}' to '{}'", metadata->importUserConfiguration->cls(), configurationClass));

            auto oldConfig = metadata->importUserConfiguration;
            metadata->importUserConfiguration = configurationClass->create<ResourceConfiguration>();
            metadata->importUserConfiguration->parent(metadata);
            metadata->importUserConfiguration->rebase(oldConfig);
            metadata->importUserConfiguration->detach();
        }
    }
    else
    {
        metadata->importUserConfiguration = configurationClass->create<ResourceConfiguration>();
        metadata->importUserConfiguration->parent(metadata);
    }

    //--

    metadata->importFullConfiguration = compileFinalImportConfiguration(info.assetFilePath, info.resourceClass, metadata->importBaseConfiguration, metadata->importUserConfiguration);
    if (!metadata->importFullConfiguration)
    {
        auto& message = outResult.messages.emplaceBack();
        message.type = logging::OutputLevel::Error;
        message.message = StringBuf(TempString("Failed to compile final import configuration"));
        return false;
    }

    metadata->importFullConfiguration->parent(metadata);
    return true;
}

bool Importer::import_doActualImport(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress) const
{
    // create an instance of importer for this job
    auto importer = outResult.importerClass.create();
    if (!importer)
    {
        auto& message = outResult.messages.emplaceBack();
        message.type = logging::OutputLevel::Error;
        message.message = StringBuf(TempString("Failed to initialize importer '{}'", outResult.importerClass));
        return false;
    }

    // create import interface
    const auto finalConfig = outResult.metadata->importFullConfiguration;
    LocalImporterInterface importerInterface(m_depot, info.assetFilePath, info.depotFilePath, progress, finalConfig);
    const auto importedResource = importer->importResource(importerInterface);

    // regardless if we produced asset or not never return anything if we got canceled
    if (progress->checkCancelation())
    {
        auto& message = outResult.messages.emplaceBack();
        message.type = logging::OutputLevel::Error;
        message.message = StringBuf(TempString("Import was canceled before it finished, results are invalid"));
        outResult.extendedStatusFlags |= ImportExtendedStatusBit::Canceled;
        return false;
    }

    // no output
    if (!importedResource)
    {
        auto& message = outResult.messages.emplaceBack();
        message.type = logging::OutputLevel::Error;
        message.message = StringBuf(TempString("Import failed, check log for details"));
        return false;
    }

    // suck in the dependencies
    outResult.metadata->importDependencies.clear();
    for (const auto& info : importerInterface.dependencies())
    {
        auto& dep = outResult.metadata->importDependencies.emplaceBack();
        dep.crc = info.fingerprint.rawValue();
        dep.timestamp = info.timestamp;
        dep.importPath = info.assetPath;
    }

    // report follow up imports
    for (const auto& info : importerInterface.followupImports())
    {
        auto& dep = outResult.followupImports.emplaceBack();
        dep.id = info.id;
        dep.assetFilePath = info.assetPath;
        dep.depotFilePath = info.depotPath;
        dep.externalConfig = info.config;
        dep.resourceClass = info.resourceClass;
    }

    // update revision number
    outResult.metadata->internalRevision += 1;
    outResult.resource = importedResource;

    // setup proper depot file path
    outResult.depotPath = ReplaceExtension(info.depotFilePath, IResource::FILE_EXTENSION);

    // we are done
    return true;
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
        auto targetClassMetadata = cls->findMetadata<ResourceImportedClassMetadata>();
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
            TRACE_WARNING("Importer class '{}' has invalid metadata (missing ResourceSourceFormatMetadata/ResourceImportedClassMetadata)", cls->name());
        }
    }
}

bool Importer::findBestImporter(StringView assetFilePath, SpecificClassType<IResource> targetResourceClass, SpecificClassType<IResourceImporter>& outImporterClass) const
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

END_BOOMER_NAMESPACE()
