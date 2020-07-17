/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#include "build.h"

#include "commandImport.h"
#include "importQueue.h"
#include "importer.h"
#include "importSaveThread.h"
#include "importSourceAssetRepository.h"
#include "importFileService.h"

#include "base/app/include/command.h"
#include "base/app/include/commandline.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/resource/include/resourceLoader.h"
#include "base/resource/include/resourceFileLoader.h"
#include "base/resource/include/resourceFileSaver.h"
#include "base/resource_compiler/include/depotStructure.h"
#include "base/object/include/object.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/xml/include/xmlUtils.h"
#include "importFileList.h"


namespace base
{
    namespace res
    {
        //--


        RTTI_BEGIN_TYPE_CLASS(CommandImport);
        RTTI_METADATA(app::CommandNameMetadata).name("import");
        RTTI_END_TYPE();

        //--

        class LocalProgressReporter : public IImportQueueCallbacks
        {
        public:
            virtual void queueJobAdded(const ImportJobInfo& info) override
            {
                TRACE_INFO("ImportQueue: added job to cook '{}' from '{}'", info.depotFilePath, info.assetFilePath);
            }

            virtual void queueJobStarted(StringView<char> depotPath) override
            {
                TRACE_INFO("ImportQueue: started cooking of '{}'", depotPath);
            }

            virtual void queueJobFinished(StringView<char> depotPath, ImportStatus status, double timeTaken) override
            {
                TRACE_INFO("ImportQueue: finished cooking of '{}' in {}: {}", depotPath, TimeInterval(timeTaken), status);
            }

            virtual void queueJobProgressUpdate(StringView<char> depotPath, uint64_t currentCount, uint64_t totalCount, StringView<char> text) override
            {

            }
        };

        //--

        // depot based resource loader
        class LocalDepotBasedLoader : public IImportDepotLoader
        {
        public:
            LocalDepotBasedLoader(depot::DepotStructure& depot)
                : m_depot(depot)
            {}

            virtual MetadataPtr loadExistingMetadata(StringView<char> depotPath) const override final
            {
                if (const auto fileReader = m_depot.createFileAsyncReader(depotPath))
                {
                    base::res::ResourceMountPoint mountPoint;
                    m_depot.queryFileMountPoint(depotPath, mountPoint);

                    FileLoadingContext context;
                    context.basePath = mountPoint.path();
                    return base::res::LoadFileMetadata(fileReader, context);
                }

                return nullptr;
            }

            virtual ResourcePtr loadExistingResource(StringView<char> depotPath) const override final
            {
                if (const auto fileReader = m_depot.createFileAsyncReader(depotPath))
                {
                    base::res::ResourceMountPoint mountPoint;
                    m_depot.queryFileMountPoint(depotPath, mountPoint);

                    FileLoadingContext context;
                    context.basePath = mountPoint.path();
                    if (base::res::LoadFile(fileReader, context))
                    {
                        if (const auto ret = context.root<IResource>())
                            return ret;
                    }
                }

                return nullptr;
            }

        private:
            depot::DepotStructure& m_depot;
        };

        //--

        static bool AddWorkToQueue(const app::CommandLine& commandline, ImportQueue& outQueue)
        {
            bool hasWork = false;

            const auto depotPath = commandline.singleValue("depotPath");
            const auto assetPath = commandline.singleValue("assetPath");
            if (depotPath && assetPath)
            {
                ImportJobInfo info;
                info.assetFilePath = assetPath;
                info.depotFilePath = depotPath;
                outQueue.scheduleJob(info);
                hasWork = true;
            }

            const auto importListPath = commandline.singleValueUTF16("assetListPath");
            if (!importListPath.empty())
            {
                if (const auto assetListDoc = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), io::AbsolutePath::Build(importListPath)))
                {
                    if (const auto assetList = LoadObjectFromXML<ImportList>(assetListDoc))
                    {
                        TRACE_INFO("Found {} files at import list '{}'", assetList->files().size(), importListPath);
                        for (const auto& file : assetList->files())
                        {
                            ImportJobInfo info;
                            info.assetFilePath = file.assetPath;
                            info.depotFilePath = file.depotPath;
                            info.userConfig = CloneObject(file.userConfiguration);
                            outQueue.scheduleJob(info);
                            hasWork = true;
                        }
                    }
                    else
                    {
                        TRACE_ERROR("Failed to parse import list from XML '{}'", importListPath);
                    }
                }
                else
                {
                    TRACE_ERROR("Failed to load import list from XML '{}'", importListPath);
                }
            }

            return hasWork;
        }

        //--

        bool CommandImport::run(const app::CommandLine& commandline)
        {
            // find the source asset service - we need it to have access to source assets
            auto assetSource = GetService<ImportFileService>();
            if (!assetSource)
            {
                TRACE_ERROR("Source asset service not started, importing not possible");
                return false;
            }

            // get the loading service and get depot
            auto loadingService = GetService<LoadingService>();
            if (!loadingService || !loadingService->loader())
            {
                TRACE_ERROR("Resource loading service not started properly (incorrect depot mapping?), cooking won't be possible.");
                return false;
            }

            // get the depot
            auto* depot = loadingService->loader()->queryUncookedDepot();
            if (!depot)
            {
                TRACE_ERROR("Resource loading service does not have uncooked depot attached. Cooking is only possible from uncooked (editor) data.");
                return false;
            }

            // protected region
            // TODO: add "__try" "__expect" ?
            {
                // create the saving thread
                ImportSaverThread saver(*depot);

                // create loader
                LocalDepotBasedLoader loader(*depot);

                // create asset source cache
                SourceAssetRepository repository(assetSource);

                // create the import queue
                LocalProgressReporter queueProgress;
                ImportQueue queue(&repository, &loader, &saver, &queueProgress);

                // add work to queue
                if (AddWorkToQueue(commandline, queue))
                {
                    // process jobs
                    while (queue.processNextJob());
                }

                // wait for saver to finish saving
                saver.waitUntilDone();
            }

            // we are done
            return true;
        }

        //--

    } // res
} // base
