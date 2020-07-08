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
#include "base/resource_compiler/include/depotStructure.h"
#include "base/object/include/nativeFileReader.h"
#include "base/object/include/serializationLoader.h"
#include "base/object/include/object.h"
#include "base/resource/include/resourceBinaryLoader.h"
#include "base/resource/include/resourceMetadata.h"


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

            }

            virtual void queueJobStarted(StringView<char> depotPath) override
            {

            }

            virtual void queueJobFinished(StringView<char> depotPath, ImportStatus status) override
            {

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
                if (const auto fileReader = m_depot.createFileReader(depotPath))
                {
                    stream::NativeFileReader streamReader(*fileReader);
                    
                    auto loader = CreateSharedPtr<binary::BinaryLoader>();

                    stream::LoadingContext context;
                    context.m_loadImports = false;
                    context.m_resourceLoader = nullptr;
                    context.m_selectiveLoadingClass = Metadata::GetStaticClass();

                    stream::LoadingResult result;
                    if (!loader->loadObjects(streamReader, context, result))
                        return nullptr;

                    if (result.m_loadedRootObjects.size() != 1)
                        return nullptr;

                    return rtti_cast<Metadata>(result.m_loadedRootObjects[0]);
                }

                return nullptr;
            }

            virtual ResourcePtr loadExistingResource(StringView<char> depotPath) const override final
            {
                if (const auto fileReader = m_depot.createFileReader(depotPath))
                {
                    stream::NativeFileReader streamReader(*fileReader);

                    auto loader = CreateSharedPtr<binary::BinaryLoader>();

                    stream::LoadingContext context;
                    context.m_loadImports = false;
                    context.m_resourceLoader = nullptr;

                    stream::LoadingResult result;
                    if (!loader->loadObjects(streamReader, context, result))
                        return nullptr;

                    if (result.m_loadedRootObjects.size() != 1)
                        return nullptr;

                    return rtti_cast<IResource>(result.m_loadedRootObjects[0]);
                }

                return nullptr;
            }

        private:
            depot::DepotStructure& m_depot;
        };

        //--

        static bool AddWorkToQueue(const app::CommandLine& commandline, ImportQueue& queue)
        {
            // simple case - import single file
            const auto depotPath = commandline.singleValue("depotPath");
            const auto assetPath = commandline.singleValue("assetPath");
            if (depotPath || assetPath)
            {
                if (depotPath && assetPath)
                {
                    ImportJobInfo job;
                    job.depotFilePath = depotPath;
                    job.assetFilePath = assetPath;
                    queue.scheduleJob(job);
                }
                else
                {
                    TRACE_ERROR("Both -depotPath and -assetPath should be speicifed");
                    return false;
                }
            }

            // TODO: job list

            return true;
        }

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
