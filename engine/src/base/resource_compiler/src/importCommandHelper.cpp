/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#include "build.h"

#include "importQueue.h"
#include "importer.h"
#include "importSaveThread.h"
#include "importSourceAssetRepository.h"
#include "importCommandHelper.h"
#include "importFileService.h"
#include "importFileList.h"
#include "importInterface.h"

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
#include "base/net/include/messageConnection.h"


namespace base
{
    namespace res
    {

        //--

        // depot based resource loader
        class LocalDepotBasedLoader : public IImportDepotLoader
        {
        public:
            LocalDepotBasedLoader(depot::DepotStructure& depot)
                : m_depot(depot)
            {}

            virtual MetadataPtr loadExistingMetadata(StringView depotPath) const override final
            {
                if (const auto fileReader = m_depot.createFileAsyncReader(depotPath))
                {
                    FileLoadingContext context;
                    return base::res::LoadFileMetadata(fileReader, context);
                }

                return nullptr;
            }

            virtual ResourcePtr loadExistingResource(StringView depotPath) const override final
            {
                if (const auto fileReader = m_depot.createFileAsyncReader(depotPath))
                {
                    FileLoadingContext context;
                    if (base::res::LoadFile(fileReader, context))
                    {
                        if (const auto ret = context.root<IResource>())
                            return ret;
                    }
                }

                return nullptr;
            }

            virtual bool depotFileExists(StringView depotPath) const override final
            {
                io::TimeStamp timestamp;
                return m_depot.queryFileTimestamp(depotPath, timestamp);
            }

            virtual bool depotFindFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const override final
            {
                return m_depot.findFile(depotPath, fileName, maxDepth, outFoundFileDepotPath);
            }

        private:
            depot::DepotStructure& m_depot;
        };

        //--

        static bool AddWorkToQueue(const ImportList* assetList, ImportQueue& outQueue)
        {
            bool hasWork = false;

            if (assetList)
            {
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

            return hasWork;
        }

        bool ProcessImport(const ImportList* files, IProgressTracker* mainProgress, IImportQueueCallbacks* callbacks)
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
                ImportQueue queue(&repository, &loader, &saver, callbacks);

                // add work to queue
                if (AddWorkToQueue(files, queue))
                {
                    // process all the jobs
                    while (queue.processNextJob(mainProgress))
                    {
                        // placeholder for optional work we may want to do BETWEEN JOBS
                    }
                }
                else
                {
                    TRACE_INFO("No work specified for ")
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
