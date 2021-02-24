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
#include "base/object/include/object.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/xml/include/xmlUtils.h"
#include "base/net/include/messageConnection.h"
#include "base/resource/include/depotService.h"

BEGIN_BOOMER_NAMESPACE(base::res)

//--

RTTI_BEGIN_TYPE_CLASS(CommandImport);
    RTTI_METADATA(app::CommandNameMetadata).name("import");
RTTI_END_TYPE();

//--

class ImportQueueProgressReporter : public IImportQueueCallbacks
{
public:
    ImportQueueProgressReporter()
    {}

    ~ImportQueueProgressReporter()
    {}

    //--

protected:
    virtual void queueJobAdded(const ImportJobInfo& info) override
    {
    }

    virtual void queueJobStarted(StringView depotPath) override
    {
    }

    virtual void queueJobFinished(StringView depotPath, ImportStatus status, double timeTaken) override
    {
    }

    virtual void queueJobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text) override
    {
    }
};

//--

// depot based resource loader
class LocalDepotBasedLoader : public IImportDepotLoader
{
public:
    LocalDepotBasedLoader()
    {
        m_depot = GetService<DepotService>();
    }

    virtual MetadataPtr loadExistingMetadata(StringView depotPath) const override final
    {
        if (const auto fileReader = m_depot->createFileAsyncReader(depotPath))
        {
            FileLoadingContext context;
            return base::res::LoadFileMetadata(fileReader, context);
        }

        return nullptr;
    }

    virtual ResourcePtr loadExistingResource(StringView depotPath) const override final
    {
        if (const auto fileReader = m_depot->createFileAsyncReader(depotPath))
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
        return m_depot->queryFileTimestamp(depotPath, timestamp);
    }

    virtual bool depotFindFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const override final
    {
        return m_depot->findFile(depotPath, fileName, maxDepth, outFoundFileDepotPath);
    }

private:
    DepotService* m_depot = nullptr;
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

bool CommandImport::run(IProgressTracker* progress, const app::CommandLine& commandline)
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

    // protected region
    // TODO: add "__try" "__expect" ?
    {
        // create the saving thread
        ImportSaverThread saver;

        // create loader
        LocalDepotBasedLoader loader;

        // create asset source cache
        SourceAssetRepository repository(assetSource);

        // create the import queue
        ImportQueueProgressReporter reporter;
        ImportQueue queue(&repository, &loader, &saver, &reporter);

        // add work to queue
        if (AddWorkToQueue(commandline, queue))
        {
            // process all the jobs
            while (queue.processNextJob(progress))
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

END_BOOMER_NAMESPACE(base::res)
