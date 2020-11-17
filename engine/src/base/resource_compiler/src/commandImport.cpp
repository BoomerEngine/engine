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


        RTTI_BEGIN_TYPE_CLASS(CommandImport);
            RTTI_METADATA(app::CommandNameMetadata).name("import");
        RTTI_END_TYPE();

        //--

        class ImportQueueProgressReporter : public IImportQueueCallbacks
        {
        public:
            ImportQueueProgressReporter(net::MessageConnection* connection)
                : m_connection(connection)
            {}

            ~ImportQueueProgressReporter()
            {}

            //--

        protected:
            net::MessageConnection* m_connection;

            virtual void queueJobAdded(const ImportJobInfo& info) override
            {
                ImportQueueFileStatusChangeMessage msg;
                msg.depotPath = info.depotFilePath;
                msg.status = ImportStatus::Pending;
                m_connection->send(msg);
            }

            virtual void queueJobStarted(StringView depotPath) override
            {
                ImportQueueFileStatusChangeMessage msg;
                msg.depotPath = StringBuf(depotPath);
                msg.status = ImportStatus::Processing;
                m_connection->send(msg);
            }

            virtual void queueJobFinished(StringView depotPath, ImportStatus status, double timeTaken) override
            {
                ImportQueueFileStatusChangeMessage msg;
                msg.depotPath = StringBuf(depotPath);
                msg.status = status;
                msg.time = timeTaken;
                m_connection->send(msg);
            }

            virtual void queueJobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text) override
            {
                // TODO
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

            virtual MetadataPtr loadExistingMetadata(StringView depotPath) const override final
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

            virtual ResourcePtr loadExistingResource(StringView depotPath) const override final
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

            const auto importListPath = commandline.singleValue("assetListPath");
            if (!importListPath.empty())
            {
                if (const auto assetListDoc = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), importListPath))
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

        CommandImport::CommandImport()
        {
        }

        CommandImport::~CommandImport()
        {
        }

        //--

        bool CommandImport::checkFileCanceled(const StringBuf& depotPath) const
        {
            auto lock = CreateLock(m_canceledFilesLock);
            return m_canceledFiles.contains(depotPath);
        }

        void CommandImport::handleImportQueueFileCancel(const base::res::ImportQueueFileCancel& message)
        {
            auto lock = CreateLock(m_canceledFilesLock);
            if (m_canceledFiles.insert(message.depotPath))
            {
                TRACE_WARNING("Import: Explicit request to cancel import of file '{}'", message.depotPath);
            }
        }

        bool CommandImport::run(base::net::MessageConnectionPtr connection, const app::CommandLine& commandline)
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
                ImportQueueProgressReporter reporter(connection);
                ImportQueue queue(&repository, &loader, &saver, &reporter);

                // add work to queue
                if (AddWorkToQueue(commandline, queue))
                {
                    // process all the jobs
                    while (queue.processNextJob(this))
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
