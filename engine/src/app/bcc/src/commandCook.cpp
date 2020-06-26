/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"

#include "base/app/include/command.h"
#include "base/app/include/commandline.h"
#include "base/resources/include/resourcePath.h"
#include "base/resources/include/resourceLoadingService.h"
#include "base/resources/include/resourceUncached.h"
#include "base/resources/include/resourceBinaryLoader.h"
#include "base/resources/include/resourceMetadata.h"
#include "base/object/include/nativeFileReader.h"
#include "base/cooking/include/cooker.h"
#include "base/cooking/include/backgroundBakeService.h"
#include "base/cooking/include/cookerSaveThread.h"
#include "base/cooking/include/cookerSeedFile.h"
#include "base/io/include/absolutePathBuilder.h"
#include "base/io/include/ioSystem.h"
#include "base/containers/include/stringBuilder.h"

namespace bcc
{
    //--

    class CommandCook : public base::app::ICommand
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CommandCook, base::app::ICommand);

    public:
        virtual bool run(const base::app::CommandLine& commandline) override final;

    private:
        base::io::AbsolutePath m_outputDir;

        //--

        base::HashSet<base::res::ResourceKey> m_seedFiles;
        bool collectSeedFiles();
        void scanDepotDirectoryForSeedFiles(base::StringView<char> depotPath, base::Array<base::res::ResourceKey>& outList, uint32_t& outNumDirectoriesVisited) const;

        //--

        struct PendingCookingEntry
        {
            base::res::ResourceKey key;
            base::res::ResourcePtr alreadyLoadedResource;
        };

        bool processSeedFiles();
        void processSingleSeedFile(const base::res::ResourceKey& key);

        bool assembleCookedOutputPath(const base::res::ResourceKey& key, base::SpecificClassType<base::res::IResource> cookedClass, io::AbsolutePath& outPath) const;

        base::res::MetadataPtr loadFileMetadata(base::stream::IBinaryReader& reader) const;
        base::res::MetadataPtr loadFileMetadata(const io::AbsolutePath& cookedOutputPath) const;

        bool checkDependenciesUpToDate(const base::res::Metadata& deps) const;

        bool cookFile(const base::res::ResourceKey& key, base::SpecificClassType<base::res::IResource> cookedClass, io::AbsolutePath& outPath, base::Array<PendingCookingEntry>& outCookingQueue);
        void queueDependencies(const base::res::IResource& object, base::Array<PendingCookingEntry>& outCookingQueue);
        void queueDependencies(const io::AbsolutePath& cookedFile, base::Array<PendingCookingEntry>& outCookingQueue);

        base::HashSet<base::res::ResourceKey> m_allCollectedFiles;
        base::HashSet<base::res::ResourceKey> m_allCookedFiles;
        base::HashSet<base::res::ResourceKey> m_allSeenFile;

        uint32_t m_cookFileIndex = 0;
        uint32_t m_numTotalVisited = 0;
        uint32_t m_numTotalUpToDate = 0;
        uint32_t m_numTotalCopied = 0;
        uint32_t m_numTotalCooked = 0;
        uint32_t m_numTotalFailed = 0;

        bool m_captureLogs = true;
        bool m_discardCookedLogs = true;

        base::UniquePtr<base::cooker::Cooker> m_cooker;
        base::UniquePtr<base::cooker::CookerSaveThread> m_saveThread;
        base::res::IResourceLoader* m_loader = nullptr;

        //--
    };

    RTTI_BEGIN_TYPE_CLASS(CommandCook);
        RTTI_METADATA(base::app::CommandNameMetadata).name("cook");
    RTTI_END_TYPE();

    //--

    bool CommandCook::run(const base::app::CommandLine& commandline)
    {
        m_outputDir = base::io::AbsolutePath::BuildAsDir(commandline.singleValueUTF16("outDir"));
        if (m_outputDir.empty())
        {
            TRACE_ERROR("Missing required argument -outDir");
            return false;
        }

        m_captureLogs = !commandline.hasParam("verboseLogs");
        m_discardCookedLogs = !commandline.hasParam("keepAllLogs");

        TRACE_INFO("Cooking output directory: '{}'", m_outputDir);

        //--

        auto loadingService = base::GetService<base::res::LoadingService>();
        if (!loadingService || !loadingService->loader())
        {
            TRACE_ERROR("Resource loading service not started properly (incorrect depot mapping?), cooking won't be possible.");
            return false;
        }

        if (!loadingService->loader()->queryUncookedDepot())
        {
            TRACE_ERROR("Resource loading service does not have uncooked depot attached. Cooking is only possible from uncooked (editor) data.");
            return false;
        }

        m_loader = loadingService->loader();
        m_cooker.create(*m_loader->queryUncookedDepot(), m_loader, nullptr, true /* final cooker */);
        m_saveThread.create();

        //--

        auto backgroundBacking = base::GetService<base::cooker::BackgroundBaker>();
        if (backgroundBacking)
            backgroundBacking->enabled(false);

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

    void CommandCook::scanDepotDirectoryForSeedFiles(base::StringView<char> depotPath, base::Array<base::res::ResourceKey>& outList, uint32_t& outNumDirectoriesVisited) const
    {
        static const auto seedFileExtension = base::res::IResource::GetResourceExtensionForClass(base::cooker::SeedFile::GetStaticClass());

        outNumDirectoriesVisited += 1;

        m_cooker->depot().enumFilesAtPath(depotPath, [&depotPath, &outList](const base::depot::DepotStructure::FileInfo& info)
            {
                if (info.name.endsWith(seedFileExtension))
                {
                    const auto path = base::res::ResourcePath(base::TempString("{}{}", depotPath, info.name));
                    outList.emplaceBack(path, base::cooker::SeedFile::GetStaticClass());
                }

                return false;
            });

        m_cooker->depot().enumDirectoriesAtPath(depotPath, [this, &depotPath, &outList, &outNumDirectoriesVisited](const base::depot::DepotStructure::DirectoryInfo& info)
            {
                const base::StringBuf path = base::TempString("{}{}/", depotPath, info.name);
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
            base::InplaceArray<base::res::IStaticResource*, 100> staticResources;
            base::res::IStaticResource::CollectAllResources(staticResources);
            TRACE_INFO("Found {} static resources", staticResources.size());

            for (const auto* info : staticResources)
            {
                const auto path = base::res::ResourcePath(info->path());
                const auto cls = info->resourceClass().cast<base::res::IResource>();
                if (cls && path)
                {
                    auto key = base::res::ResourceKey(path, cls);
                    TRACE_INFO("Collected static resource '{}'", key);
                    m_seedFiles.insert(key);
                    m_allCollectedFiles.insert(key);
                }
            }
        }

        // scan depot for seed file
        {
            // collect
            uint32_t numDirectoriesVisited = 0;
            base::InplaceArray<base::res::ResourceKey, 100> seedFilesLists;
            scanDepotDirectoryForSeedFiles("", seedFilesLists, numDirectoriesVisited);
            TRACE_INFO("Found {} seed lists in {} depot directories", seedFilesLists.size(), numDirectoriesVisited);

            // load
            for (const auto& key : seedFilesLists)
            {
                if (const auto seedFile = base::LoadResource<base::cooker::SeedFile>(key.path()).acquire())
                {
                    uint32_t numAdded = 0;
                    for (const auto& fileInfo : seedFile->files())
                    {
                        // resolve the relative path to something more useful
                        base::StringBuf depotPath;
                        if (base::res::ApplyRelativePath(key.path().path(), fileInfo.relativePath, depotPath))
                        {
                            if (fileInfo.resourceClass)
                            {
                                const auto fileKey = base::res::ResourceKey(base::res::ResourcePath(depotPath), fileInfo.resourceClass);
                                if (m_seedFiles.insert(fileKey))
                                {
                                    m_allCollectedFiles.insert(fileKey);
                                    numAdded += 1;
                                }
                            }
                            else
                            {
                                TRACE_WARNING("Missing/lost resource class for seed file '{}', found in '{}'", depotPath, key.path());
                            }
                        }
                        else
                        {
                            TRACE_WARNING("Unable to resolve relative path '{}' in context of file '{}'", fileInfo.relativePath, key.path());
                        }
                    }

                    TRACE_INFO("Loaded {} files from seed file '{}' ({} added to cook list)", seedFile->files().size(), key.path(), numAdded);
                }
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
            TRACE_INFO("Processing seed file {}/{}: {}", i + 1, m_seedFiles.size(), seedFilePath.path());

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

        m_saveThread->waitUntilDone();

        return m_numTotalFailed == 0;
    }

    void CommandCook::processSingleSeedFile(const base::res::ResourceKey& seedFileKey)
    {
        bool valid = true;

        base::Array<PendingCookingEntry> cookingQueue;
        ScopeTimer cookingTimer;

        auto& rootEntry = cookingQueue.emplaceBack();
        rootEntry.key = seedFileKey;

        uint32_t localProcessed = 0;
        while (!cookingQueue.empty())
        {
            // get entry to cook
            auto topEntry = std::move(cookingQueue.back());
            cookingQueue.popBack();
            localProcessed += 1;
            m_numTotalVisited += 1;

            // prevent this file from being recooked second time this session
            if (!m_allSeenFile.insert(topEntry.key))
                continue;

            /// check if can cook this file at all
            SpecificClassType<res::IResource> cookedClass;
            if (!m_cooker->canCook(topEntry.key, cookedClass))
            {
                TRACE_WARNING("Resource '{}' is not cookable and will be skipped. Why is it referenced though?");
                continue;
            }

            // assemble cooked output path - cooked file will be stored there
            base::io::AbsolutePath cookedFilePath;
            if (!assembleCookedOutputPath(topEntry.key, cookedClass, cookedFilePath))
            {
                TRACE_WARNING("Resource '{}' is not cookable (no valid cooked extension)");
                continue;
            }

            // evaluate dirty state of the file, especially if we can skip cooking it :)
            // first, target file must exist to have any chance of skipping the cook :)
            if (IO::GetInstance().fileExists(cookedFilePath))
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
                    TRACE_WARNING("Failed to load metadata for output file '{}'. It might be corrupted, recooking.", topEntry.key);
                }
            }

            // cook the file
            if (cookFile(topEntry.key, cookedClass, cookedFilePath, cookingQueue))
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

    bool CommandCook::checkDependenciesUpToDate(const base::res::Metadata& deps) const
    {
        // check if cooker class still exists
        if (deps.cookerClass == nullptr)
        {
            TRACE_INFO("Cooker class no longer exists for file");
            return false;
        }

        // check that cooker class has still the same version
        const auto currentCookerVersion = deps.cookerClass->findMetadata<base::res::ResourceCookerVersionMetadata>();
        if (currentCookerVersion->version() != deps.cookerClassVersion)
        {
            TRACE_INFO("Cooker '{}' changed version from {} -> {}", deps.cookerClass->name(), deps.cookerClassVersion, currentCookerVersion->version());
            return false;
        }

        // check that resource class has still the same version
        /*const auto currentResourceVersion = deps.resource->findMetadata<base::res::ResourceCookerVersionMetadata>();
        if (currentCookerVersion->version != deps.cookerClassVersion)
        {
            TRACE_INFO("Cooker '{}' changed version from {} -> {}", deps.cookerClass->name(), deps.cookerClassVersion, currentCookerVersion->version);
            return false;
        }*/

        // check file dependencies
        for (const auto& dep : deps.sourceDependencies)
        {
            base::io::TimeStamp timestamp;
            m_cooker->depot().queryFileInfo(dep.sourcePath, nullptr, nullptr, &timestamp);

            if (dep.timestamp == timestamp.value())
                continue;

            if (dep.crc != 0)
            {
                uint64_t crc = 0;
                m_cooker->depot().queryFileInfo(dep.sourcePath, nullptr, &crc, nullptr);

                if (crc == dep.crc)
                    continue;
            }

            TRACE_INFO("Dependency file '{}' has changed", dep.sourcePath);
            return false;
        }

        return true;
    }

    base::res::MetadataPtr CommandCook::loadFileMetadata(base::stream::IBinaryReader& reader) const
    {
        auto loader = base::CreateSharedPtr<base::res::binary::BinaryLoader>();

        base::stream::LoadingContext context;
        context.m_loadImports = false;
        context.m_resourceLoader = nullptr;
        context.m_selectiveLoadingClass = base::res::Metadata::GetStaticClass();
        
        base::stream::LoadingResult result;
        if (!loader->loadObjects(reader, context, result))
            return nullptr;

        if (result.m_loadedRootObjects.size() != 1)
            return nullptr;

        return base::rtti_cast<base::res::Metadata>(result.m_loadedRootObjects[0]);
    }

    base::res::MetadataPtr CommandCook::loadFileMetadata(const io::AbsolutePath& cookedOutputPath) const
    {
        auto fileReader = IO::GetInstance().openForReading(cookedOutputPath);
        if (!fileReader)
            return nullptr;

        base::stream::NativeFileReader streaReader(*fileReader);
        return loadFileMetadata(streaReader);
    }

    bool CommandCook::assembleCookedOutputPath(const base::res::ResourceKey& key, base::SpecificClassType<base::res::IResource> cookedClass, io::AbsolutePath& outPath) const
    {
        const auto loadExtension = base::res::IResource::GetResourceExtensionForClass(cookedClass);
        if (!loadExtension)
        {
            TRACE_ERROR("Resource class '{}' has no cooked extension. Cooking to that format will not be possible.", cookedClass);
            return false;
        }

        base::StringBuilder localPath;
        localPath << "cooked/";
        localPath << key.path().directory();
        localPath << key.path().fileName();
        localPath << "." << loadExtension;

        outPath = m_outputDir.addFile(localPath.view());
        return true;
    }

    //--

    void CommandCook::queueDependencies(const base::res::IResource& object, base::Array<PendingCookingEntry>& outCookingQueue)
    {
        base::HashSet<res::ResourceKey> referencedResources;

        base::ScopeTimer timer;
        {
            base::InplaceArray<const IObject*, 1> objects;
            objects.pushBack(&object);
            base::res::ExtractReferencedResources(objects, true, referencedResources);
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
                    outEntry.key = key;
                }
            }
        }
    }

    void CommandCook::queueDependencies(const io::AbsolutePath& cookedFilePath, base::Array<PendingCookingEntry>& outCookingQueue)
    {
        auto fileReader = IO::GetInstance().openForReading(cookedFilePath);
        if (fileReader)
        {
            auto loader = base::CreateSharedPtr<base::res::binary::BinaryLoader>();

            base::InplaceArray<base::stream::LoadingDependency, 100> dependencies;

            base::stream::NativeFileReader reader(*fileReader);
            if (loader->extractLoadingDependencies(reader, true, dependencies))
            {
                TRACE_INFO("Loaded {} existing dependencies from '{}'", dependencies.size(), cookedFilePath);

                for (const auto& entry : dependencies)
                {
                    auto& outEntry = outCookingQueue.emplaceBack();
                    outEntry.key = base::res::ResourceKey(base::res::ResourcePath(entry.resourceDepotPath), entry.resourceClass.cast<base::res::IResource>());
                }
            }
        }
    }

    class CookingLogCapture : public base::logging::LocalLogSink
    {
    public:
        CookingLogCapture(const io::AbsolutePath& outPath, bool captureFully)
            : m_fullyCaptured(captureFully)
        {
            m_logFilePath = outPath.addExtension(".log");
            m_logOutput = IO::GetInstance().openForWriting(m_logFilePath);
        }

        void discardLog()
        {
            if (m_logOutput)
                m_logOutput.reset();

            IO::GetInstance().deleteFile(m_logFilePath);
        }

        virtual bool print(base::logging::OutputLevel level, const char* file, uint32_t line, const char* context, const char* text) override
        {
            if (m_logOutput)
            {
                base::StringBuilder str;

                switch (level)
                {
                case base::logging::OutputLevel::Error:; str.appendf("!!!! ERROR: "); break;
                case base::logging::OutputLevel::Warning:; str.appendf("! WARNING: "); break;
                }

                str.append(text);
                str.append("\n");

                if (m_logOutput->writeSync(str.c_str(), str.length()) != str.length())
                    m_logOutput.reset();
            }

            return m_fullyCaptured; // consume
        }

        virtual ~CookingLogCapture()
        {
            m_logOutput.reset();
        }

    private:
        base::io::FileHandlePtr m_logOutput;
        base::io::AbsolutePath m_logFilePath;
        bool m_fullyCaptured;
    };

    bool CommandCook::cookFile(const base::res::ResourceKey& key, base::SpecificClassType<base::res::IResource> cookedClass, io::AbsolutePath& outPath, base::Array<PendingCookingEntry>& outCookingQueue)
    {
        // do not cook files more than once, also promote the resource key to it's true class, ie ITexture:lena.png -> StaticTexture:lena.png
        const auto cookKey = base::res::ResourceKey(key.path(), cookedClass);
        if (!m_allCookedFiles.insert(cookKey))
            return true;

        // print header
        TRACE_INFO("Cooking file {}: {}", m_cookFileIndex, key);
        m_cookFileIndex += 1;
        // TODO: break on file

        // capture all log output, we are only interested in success/failure
        base::res::ResourcePtr cookedFile;
        {
            CookingLogCapture logCapture(outPath, m_captureLogs);
            cookedFile = m_cooker->cook(cookKey);

            if (m_discardCookedLogs && cookedFile)
                logCapture.discardLog();
        }

        // oh well
        if (!cookedFile)
        {
            TRACE_ERROR("Failed to cook file '{}'", cookKey.path());
            return false;
        }

        // gather resources used for this resource
        queueDependencies(*cookedFile, outCookingQueue);

        // add to save queue
        m_saveThread->scheduleSave(cookedFile, outPath);
        return true;
    }

    //--

} // bcc
