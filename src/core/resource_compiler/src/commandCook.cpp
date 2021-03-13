/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#include "build.h"

#include "core/app/include/command.h"
#include "core/app/include/commandline.h"
#include "core/resource/include/id.h"
#include "core/resource/include/loader.h"
#include "core/resource/include/metadata.h"
#include "core/io/include/io.h"
#include "core/io/include/fileHandle.h"
#include "core/containers/include/stringBuilder.h"
#include "core/resource/include/tags.h"

#include "commandCook.h"
#include "core/resource/include/fileLoader.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(CommandCook);
    RTTI_METADATA(CommandNameMetadata).name("cook");
RTTI_END_TYPE();

//--

bool CommandCook::run(IProgressTracker* progress, const CommandLine& commandline)
{
    m_outputDir = commandline.singleValue("outDir");
    if (m_outputDir.empty())
    {
        TRACE_ERROR("Missing required argument -outDir");
        return false;
    }

    m_captureLogs = !commandline.hasParam("verboseLogs");
    m_discardCookedLogs = !commandline.hasParam("keepAllLogs");

    TRACE_INFO("Cooking output directory: '{}'", m_outputDir);

    //--

    //--

    if (!collectSeedFiles())
        return false;

    if (!processSeedFiles())
        return false;

    //--

    TRACE_INFO("Total {} files processed", m_allCollectedFiles.size());
    return true;
}

//--

void CommandCook::scanDepotDirectoryForSeedFiles(StringView depotPath, Array<StringBuf>& outList, uint32_t& outNumDirectoriesVisited) const
{
    //static const auto seedFileExtension = IResource::GetResourceExtensionForClass(SeedFile::GetStaticClass());

    outNumDirectoriesVisited += 1;

    auto depot = GetService<DepotService>();

    depot->enumFilesAtPath(depotPath, [&depotPath, &outList](StringView name)
        {
            /*if (info.name.endsWith(seedFileExtension))
            {
                const auto path = StringBuf(TempString("{}{}", depotPath, info.name));
                outList.emplaceBack(path, SeedFile::GetStaticClass());
            }*/
        });

    depot->enumDirectoriesAtPath(depotPath, [this, &depotPath, &outList, &outNumDirectoriesVisited](StringView name)
        {
            const StringBuf path = TempString("{}{}/", depotPath, name);
            scanDepotDirectoryForSeedFiles(path, outList, outNumDirectoriesVisited);
            return false;
        });
}

bool CommandCook::collectSeedFiles()
{
    ScopeTimer timer;

    // make sure all static resources are cooked
    // TODO: skip resources from dev only projects
    {
        InplaceArray<IStaticResource*, 100> staticResources;
        IStaticResource::CollectAllResources(staticResources);
        TRACE_INFO("Found {} static resources", staticResources.size());

        for (const auto* info : staticResources)
        {
            if (info->path())
            {
                TRACE_INFO("Collected static resource '{}'", info->path());
                m_seedFiles.insert(StringBuf(info->path()));
                m_allCollectedFiles.insert(StringBuf(info->path()));
            }
        }
    }

    // scan depot for seed file
    {
        // collect
        uint32_t numDirectoriesVisited = 0;
        InplaceArray<StringBuf, 100> seedFilesLists;
        scanDepotDirectoryForSeedFiles("", seedFilesLists, numDirectoriesVisited);
        TRACE_INFO("Found {} seed lists in {} depot directories", seedFilesLists.size(), numDirectoriesVisited);

        // load
        for (const auto& key : seedFilesLists)
        {
            /*if (const auto seedFile = rtti_cast<SeedFile>(LoadResource(key).acquire()))
            {
                uint32_t numAdded = 0;
                for (const auto& file : seedFile->files())
                {
                    if (m_seedFiles.insert(file.key()))
                    {
                        m_allCollectedFiles.insert(file.key());
                        numAdded += 1;
                    }
                }

                TRACE_INFO("Loaded {} files from seed file '{}' ({} added to cook list)", seedFile->files().size(), key.path(), numAdded);
            }*/
        }
    }


    TRACE_INFO("Collected {} initial files for cooking (seed files) in {}", m_seedFiles.size(), timer);
    return true;
}

//--

bool CommandCook::processSeedFiles()
{
    for (uint32_t i = 0; i<m_seedFiles.size(); ++i)
    {
        const auto& seedFilePath = m_seedFiles.keys()[i];
        TRACE_INFO("Processing seed file {}/{}: {}", i + 1, m_seedFiles.size(), seedFilePath);

        processSingleSeedFile(seedFilePath);

        if (m_numTotalFailed > 100)
        {
            // something is really wrong
            TRACE_ERROR("More than 100 files failed cooking, something must be VERY wrong. Stopping now.");
            return false;
        }
    }

    TRACE_INFO("Finished processing {} seed files.", m_seedFiles.size());
    TRACE_INFO("Visited {} files, {} up to date, {} copied, {} cooked and {} failed", m_numTotalVisited, m_numTotalUpToDate, m_numTotalCopied, m_numTotalCooked, m_numTotalFailed);

    //m_saveThread->waitUntilDone();

    return m_numTotalFailed == 0;
}

void CommandCook::processSingleSeedFile(const StringBuf& seedFileKey)
{
    bool valid = true;

    Array<PendingCookingEntry> cookingQueue;
    ScopeTimer cookingTimer;

    auto& rootEntry = cookingQueue.emplaceBack();
    rootEntry.path = seedFileKey;

    uint32_t localProcessed = 0;
    while (!cookingQueue.empty())
    {
        // get entry to cook
        auto topEntry = std::move(cookingQueue.back());
        cookingQueue.popBack();
        localProcessed += 1;
        m_numTotalVisited += 1;

        // prevent this file from being recooked second time this session
        if (!m_allSeenFile.insert(topEntry.path))
            continue;

        /// check if can cook this file at all
        SpecificClassType<IResource> cookedClass;
        /*if (!m_cooker->canCook(topEntry.key, cookedClass))
        {
            TRACE_WARNING("Resource '{}' is not cookable and will be skipped. Why is it referenced though?");
            continue;
        }*/

        // assemble cooked output path - cooked file will be stored there
        StringBuf cookedFilePath;
        if (!assembleCookedOutputPath(topEntry.path, cookedClass, cookedFilePath))
        {
            TRACE_WARNING("Resource '{}' is not cookable (no valid cooked extension)");
            continue;
        }

        // evaluate dirty state of the file, especially if we can skip cooking it :)
        // first, target file must exist to have any chance of skipping the cook :)
        if (FileExists(cookedFilePath))
        {
            // load the source dependencies of the file (metadata)
            auto metadata = loadFileMetadata(cookedFilePath);
            if (metadata)
            {
                // if all our dependencies check out then we don't have to cook that file
                if (checkDependenciesUpToDate(*metadata))
                {
                    // we can skip this file but make sure the loading dependencies are cooked
                    queueDependencies(cookedFilePath, cookingQueue);
                    m_numTotalUpToDate += 1;
                    continue;;
                }
            }
            else
            {
                TRACE_WARNING("Failed to load metadata for output file '{}'. It might be corrupted, recooking.", topEntry.path);
            }
        }

        // cook the file
        if (cookFile(topEntry.path, cookedClass, cookedFilePath, cookingQueue))
        {
            m_numTotalCooked += 1;
        }
        else
        {
            m_numTotalFailed += 1;
        }
    }

    TRACE_INFO("Processed {} source files for seed file '{}' in {}", localProcessed, seedFileKey, cookingTimer);
}

bool CommandCook::checkDependenciesUpToDate(const ResourceMetadata& deps) const
{
    /*// check if cooker class still exists
    if (deps.cookerClass == nullptr)
    {
        TRACE_INFO("Cooker class no longer exists for file");
        return false;
    }

    // check that cooker class has still the same version
    const auto currentCookerVersion = deps.cookerClass->findMetadata<ResourceCookerVersionMetadata>();
    if (currentCookerVersion->version() != deps.cookerClassVersion)
    {
        TRACE_INFO("Cooker '{}' changed version from {} -> {}", deps.cookerClass->name(), deps.cookerClassVersion, currentCookerVersion->version());
        return false;
    }*/

    // check that resource class has still the same version
    /*const auto currentResourceVersion = deps.resource->findMetadata<ResourceCookerVersionMetadata>();
    if (currentCookerVersion->version != deps.cookerClassVersion)
    {
        TRACE_INFO("Cooker '{}' changed version from {} -> {}", deps.cookerClass->name(), deps.cookerClassVersion, currentCookerVersion->version);
        return false;
    }*/

    // check file dependencies
    /*for (const auto& dep : deps.importDependencies)
    {
        TimeStamp timestamp;
        GetService<DepotService>()->queryFileTimestamp(dep.sourcePath, timestamp);

        if (dep.timestamp == timestamp.value())
            continue;

        TRACE_INFO("Dependency file '{}' has changed", dep.sourcePath);
        return false;
    }*/

    return true;
}

ResourceMetadataPtr CommandCook::loadFileMetadata(StringView cookedOutputPath) const
{
    if (auto fileReader = OpenForAsyncReading(cookedOutputPath))
    {
        FileLoadingContext context;
        return nullptr;// return LoadFileMetadata(fileReader, context);
    }

    return nullptr;
}

bool CommandCook::assembleCookedOutputPath(const StringBuf& key, SpecificClassType<IResource> cookedClass, StringBuf& outPath) const
{
    /*const auto loadExtension = IResource::GetResourceExtensionForClass(cookedClass);
    if (!loadExtension)
    {
        TRACE_ERROR("Resource class '{}' has no cooked extension. Cooking to that format will not be possible.", cookedClass);
        return false;
    }

    StringBuilder localPath;
    localPath << m_outputDir;
    localPath << "cooked/";
    localPath << key.view().baseDirectory();
    localPath << key.view().fileName();
    localPath << "." << loadExtension;

    outPath = localPath.toString();*/
    return true;
}

//--

void CommandCook::queueDependencies(const IResource& object, Array<PendingCookingEntry>& outCookingQueue)
{
    HashSet<StringBuf> referencedResources;

    ScopeTimer timer;
    {
        InplaceArray<const IObject*, 1> objects;
        objects.pushBack(&object);
        //ExtractReferencedResources(objects, true, referencedResources);
    }

    if (!referencedResources.empty())
    {
        TRACE_INFO("Found {} referenced resources, adding them to cook list", referencedResources.size());

        for (const auto& key : referencedResources)
        {
            if (m_allCollectedFiles.insert(key))
            {
                TRACE_INFO("Added '{}' to cooking queue", key);
                auto& outEntry = outCookingQueue.emplaceBack();
                outEntry.path = key;
            }
        }
    }
}

void CommandCook::queueDependencies(StringView cookedFilePath, Array<PendingCookingEntry>& outCookingQueue)
{
    if (auto fileReader = OpenForAsyncReading(cookedFilePath))
    {
        InplaceArray<FileLoadingDependency, 100> dependencies;
        FileLoadingContext loadingContext;
        if (LoadFileDependencies(fileReader, loadingContext, dependencies))
        {
            TRACE_INFO("Loaded {} existing dependencies from '{}'", dependencies.size(), cookedFilePath);

            for (const auto& entry : dependencies)
            {
                auto& outEntry = outCookingQueue.emplaceBack();
                // TODO: translate ID to path
                //outEntry.path = entry.id;
            }
        }
    }
}

bool CommandCook::cookFile(const StringBuf& key, SpecificClassType<IResource> cookedClass, StringBuf& outPath, Array<PendingCookingEntry>& outCookingQueue)
{
    // do not cook files more than once, also promote the resource key to it's true class, ie ITexture:lena.png -> StaticTexture:lena.png
    if (!m_allCookedFiles.insert(key))
        return true;

    // print header
    TRACE_INFO("Cooking file {}: {}", m_cookFileIndex, key);
    m_cookFileIndex += 1;
    // TODO: break on file

    // capture all log output, we are only interested in success/failure
    ResourcePtr cookedFile;
    {
        //CookingLogCapture logCapture(outPath, m_captureLogs);
        //cookedFile = m_cooker->cook(cookKey);

        //if (m_discardCookedLogs && cookedFile)
            //  logCapture.discardLog();
    }

    // oh well
    if (!cookedFile)
    {
        TRACE_ERROR("Failed to cook file '{}'", key);
        return false;
    }

    // gather resources used for this resource
    queueDependencies(*cookedFile, outCookingQueue);

    // add to save queue
//    m_saveThread->scheduleSave(cookedFile, outPath);
    return true;
}

//--

END_BOOMER_NAMESPACE()
