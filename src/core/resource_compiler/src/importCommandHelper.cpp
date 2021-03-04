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

#include "core/app/include/command.h"
#include "core/app/include/commandline.h"
#include "core/resource/include/loadingService.h"
#include "core/resource/include/loader.h"
#include "core/resource/include/fileLoader.h"
#include "core/resource/include/fileSaver.h"
#include "core/object/include/object.h"
#include "core/resource/include/metadata.h"
#include "core/xml/include/xmlUtils.h"
#include "core/net/include/messageConnection.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE()

//--

// depot based resource loader
class LocalDepotBasedLoader : public IImportDepotLoader
{
public:
    LocalDepotBasedLoader()
    {
        m_depot = GetService<DepotService>();
    }

    virtual ResourceMetadataPtr loadExistingMetadata(StringView depotPath) const override final
    {
        if (const auto fileReader = m_depot->createFileAsyncReader(depotPath))
        {
            FileLoadingContext context;
            return LoadFileMetadata(fileReader, context);
        }

        return nullptr;
    }

    virtual ResourcePtr loadExistingResource(StringView depotPath) const override final
    {
        if (const auto fileReader = m_depot->createFileAsyncReader(depotPath))
        {
            FileLoadingContext context;
            if (LoadFile(fileReader, context))
            {
                if (const auto ret = context.root<IResource>())
                    return ret;
            }
        }

        return nullptr;
    }

    virtual bool depotFileExists(StringView depotPath) const override final
    {
        TimeStamp timestamp;
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

static bool AddWorkToQueue(const ImportList* assetList, ImportQueue& outQueue, bool force)
{
    bool hasWork = false;

    if (assetList)
    {
        for (const auto& file : assetList->files())
        {
            ImportJobInfo info;
            info.assetFilePath = file.assetPath;
            info.depotFilePath = file.depotPath;
            info.forceImport = force;
            info.userConfig = CloneObject(file.userConfiguration);
            outQueue.scheduleJob(info);
            hasWork = true;
        }
    }

    return hasWork;
}

bool ProcessImport(const ImportList* files, IProgressTracker* mainProgress, IImportQueueCallbacks* callbacks, bool force)
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
        ImportQueue queue(&repository, &loader, &saver, callbacks);

        // add work to queue
        if (AddWorkToQueue(files, queue, force))
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

END_BOOMER_NAMESPACE()
