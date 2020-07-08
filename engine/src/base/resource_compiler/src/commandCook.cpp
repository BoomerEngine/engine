/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#include "build.h"

#include "base/app/include/command.h"
#include "base/app/include/commandline.h"
#include "base/resource/include/resourcePath.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/resource/include/resourceUncached.h"
#include "base/resource/include/resourceBinaryLoader.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/object/include/nativeFileReader.h"
#include "base/io/include/absolutePathBuilder.h"
#include "base/io/include/ioSystem.h"
#include "base/containers/include/stringBuilder.h"
#include "base/resource_compiler/include/depotStructure.h"

#include "cooker.h"
#include "cookerSaveThread.h"
#include "cookerSeedFile.h"

#include "commandCook.h"

namespace base
{
    namespace res
    {
        //--

        RTTI_BEGIN_TYPE_CLASS(CommandCook);
            RTTI_METADATA(app::CommandNameMetadata).name("cook");
        RTTI_END_TYPE();

        //--

        bool CommandCook::run(const app::CommandLine& commandline)
        {
            m_outputDir = io::AbsolutePath::BuildAsDir(commandline.singleValueUTF16("outDir"));
            if (m_outputDir.empty())
            {
                TRACE_ERROR("Missing required argument -outDir");
                return false;
            }

            m_captureLogs = !commandline.hasParam("verboseLogs");
            m_discardCookedLogs = !commandline.hasParam("keepAllLogs");

            TRACE_INFO("Cooking output directory: '{}'", m_outputDir);

            //--

            auto loadingService = GetService<LoadingService>();
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

            if (!collectSeedFiles())
                return false;

            if (!processSeedFiles())
                return false;

            //--

            TRACE_INFO("Total {} files processed", m_allCollectedFiles.size());
            return true;
        }

        //--

        void CommandCook::scanDepotDirectoryForSeedFiles(StringView<char> depotPath, Array<ResourceKey>& outList, uint32_t& outNumDirectoriesVisited) const
        {
            static const auto seedFileExtension = IResource::GetResourceExtensionForClass(SeedFile::GetStaticClass());

            outNumDirectoriesVisited += 1;

            m_cooker->depot().enumFilesAtPath(depotPath, [&depotPath, &outList](const depot::DepotStructure::FileInfo& info)
                {
                    if (info.name.endsWith(seedFileExtension))
                    {
                        const auto path = ResourcePath(TempString("{}{}", depotPath, info.name));
                        outList.emplaceBack(path, SeedFile::GetStaticClass());
                    }

                    return false;
                });

            m_cooker->depot().enumDirectoriesAtPath(depotPath, [this, &depotPath, &outList, &outNumDirectoriesVisited](const depot::DepotStructure::DirectoryInfo& info)
                {
                    const StringBuf path = TempString("{}{}/", depotPath, info.name);
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
                    const auto path = ResourcePath(info->path());
                    const auto cls = info->resourceClass().cast<IResource>();
                    if (cls && path)
                    {
                        auto key = ResourceKey(path, cls);
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
                InplaceArray<ResourceKey, 100> seedFilesLists;
                scanDepotDirectoryForSeedFiles("", seedFilesLists, numDirectoriesVisited);
                TRACE_INFO("Found {} seed lists in {} depot directories", seedFilesLists.size(), numDirectoriesVisited);

                // load
                for (const auto& key : seedFilesLists)
                {
                    if (const auto seedFile = LoadResource<SeedFile>(key.path()).acquire())
                    {
                        uint32_t numAdded = 0;
                        for (const auto& fileInfo : seedFile->files())
                        {
                            // resolve the relative path to something more useful
                            StringBuf depotPath;
                            if (ApplyRelativePath(key.path().path(), fileInfo.relativePath, depotPath))
                            {
                                if (fileInfo.resourceClass)
                                {
                                    const auto fileKey = ResourceKey(ResourcePath(depotPath), fileInfo.resourceClass);
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

        void CommandCook::processSingleSeedFile(const ResourceKey& seedFileKey)
        {
            bool valid = true;

            Array<PendingCookingEntry> cookingQueue;
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
                SpecificClassType<IResource> cookedClass;
                if (!m_cooker->canCook(topEntry.key, cookedClass))
                {
                    TRACE_WARNING("Resource '{}' is not cookable and will be skipped. Why is it referenced though?");
                    continue;
                }

                // assemble cooked output path - cooked file will be stored there
                io::AbsolutePath cookedFilePath;
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

        bool CommandCook::checkDependenciesUpToDate(const Metadata& deps) const
        {
            // check if cooker class still exists
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
            }

            // check that resource class has still the same version
            /*const auto currentResourceVersion = deps.resource->findMetadata<ResourceCookerVersionMetadata>();
            if (currentCookerVersion->version != deps.cookerClassVersion)
            {
                TRACE_INFO("Cooker '{}' changed version from {} -> {}", deps.cookerClass->name(), deps.cookerClassVersion, currentCookerVersion->version);
                return false;
            }*/

            // check file dependencies
            for (const auto& dep : deps.sourceDependencies)
            {
                io::TimeStamp timestamp;
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

        MetadataPtr CommandCook::loadFileMetadata(stream::IBinaryReader& reader) const
        {
            auto loader = CreateSharedPtr<binary::BinaryLoader>();

            stream::LoadingContext context;
            context.m_loadImports = false;
            context.m_resourceLoader = nullptr;
            context.m_selectiveLoadingClass = Metadata::GetStaticClass();
        
            stream::LoadingResult result;
            if (!loader->loadObjects(reader, context, result))
                return nullptr;

            if (result.m_loadedRootObjects.size() != 1)
                return nullptr;

            return rtti_cast<Metadata>(result.m_loadedRootObjects[0]);
        }

        MetadataPtr CommandCook::loadFileMetadata(const io::AbsolutePath& cookedOutputPath) const
        {
            auto fileReader = IO::GetInstance().openForReading(cookedOutputPath);
            if (!fileReader)
                return nullptr;

            stream::NativeFileReader streaReader(*fileReader);
            return loadFileMetadata(streaReader);
        }

        bool CommandCook::assembleCookedOutputPath(const ResourceKey& key, SpecificClassType<IResource> cookedClass, io::AbsolutePath& outPath) const
        {
            const auto loadExtension = IResource::GetResourceExtensionForClass(cookedClass);
            if (!loadExtension)
            {
                TRACE_ERROR("Resource class '{}' has no cooked extension. Cooking to that format will not be possible.", cookedClass);
                return false;
            }

            StringBuilder localPath;
            localPath << "cooked/";
            localPath << key.path().directory();
            localPath << key.path().fileName();
            localPath << "." << loadExtension;

            outPath = m_outputDir.addFile(localPath.view());
            return true;
        }

        //--

        void CommandCook::queueDependencies(const IResource& object, Array<PendingCookingEntry>& outCookingQueue)
        {
            HashSet<ResourceKey> referencedResources;

            ScopeTimer timer;
            {
                InplaceArray<const IObject*, 1> objects;
                objects.pushBack(&object);
                ExtractReferencedResources(objects, true, referencedResources);
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

        void CommandCook::queueDependencies(const io::AbsolutePath& cookedFilePath, Array<PendingCookingEntry>& outCookingQueue)
        {
            auto fileReader = IO::GetInstance().openForReading(cookedFilePath);
            if (fileReader)
            {
                auto loader = CreateSharedPtr<binary::BinaryLoader>();

                InplaceArray<stream::LoadingDependency, 100> dependencies;

                stream::NativeFileReader reader(*fileReader);
                if (loader->extractLoadingDependencies(reader, true, dependencies))
                {
                    TRACE_INFO("Loaded {} existing dependencies from '{}'", dependencies.size(), cookedFilePath);

                    for (const auto& entry : dependencies)
                    {
                        auto& outEntry = outCookingQueue.emplaceBack();
                        outEntry.key = ResourceKey(ResourcePath(entry.resourceDepotPath), entry.resourceClass.cast<IResource>());
                    }
                }
            }
        }

        class CookingLogCapture : public logging::LocalLogSink
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

            virtual bool print(logging::OutputLevel level, const char* file, uint32_t line, const char* context, const char* text) override
            {
                if (m_logOutput)
                {
                    StringBuilder str;

                    switch (level)
                    {
                    case logging::OutputLevel::Error:; str.appendf("!!!! ERROR: "); break;
                    case logging::OutputLevel::Warning:; str.appendf("! WARNING: "); break;
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
            io::FileHandlePtr m_logOutput;
            io::AbsolutePath m_logFilePath;
            bool m_fullyCaptured;
        };

        bool CommandCook::cookFile(const ResourceKey& key, SpecificClassType<IResource> cookedClass, io::AbsolutePath& outPath, Array<PendingCookingEntry>& outCookingQueue)
        {
            // do not cook files more than once, also promote the resource key to it's true class, ie ITexture:lena.png -> StaticTexture:lena.png
            const auto cookKey = ResourceKey(key.path(), cookedClass);
            if (!m_allCookedFiles.insert(cookKey))
                return true;

            // print header
            TRACE_INFO("Cooking file {}: {}", m_cookFileIndex, key);
            m_cookFileIndex += 1;
            // TODO: break on file

            // capture all log output, we are only interested in success/failure
            ResourcePtr cookedFile;
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

    } // res
} // base
