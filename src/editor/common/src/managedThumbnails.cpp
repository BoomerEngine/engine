/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedThumbnails.h"
#include "core/image/include/image.h"
#include "core/io/include/io.h"
#include "core/io/include/directoryWatcher.h"
#include "core/containers/include/stringParser.h"
#include "core/process/include/process.h"
#include "core/resource/include/thumbnail.h"

#include "editorService.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

ConfigProperty cvThumbnailsGenerationEnabled("Editor.Thumbnails", "EnableGeneration", true);
ConfigProperty cvThumbnailsLoadingEnabled("Editor.Thumbnails", "EnableLoading", true);

///---

ManagedThumbnailHelper::ManagedThumbnailHelper()
{
}

ManagedThumbnailHelper::~ManagedThumbnailHelper()
{
    FiberSemaphore loadingFence;
    {
        auto lock = CreateLock(m_loadingQueueLock);
        m_loadingQueue.clearPtr();
        m_loadingMap.clear();
        loadingFence = std::move(m_loadingJobFence);
    }

    WaitForFence(loadingFence);
}

void ManagedThumbnailHelper::loadThumbnail(ManagedFile* file)
{
    auto lock = CreateLock(m_loadingQueueLock);

    LoadingRequest* req = nullptr;
    if (m_loadingMap.find(file, req))
    {
        if (req->frameIndex != m_loadingQueueFrameIndex)
        {
            req->frameIndex = m_loadingQueueFrameIndex;
            m_loadingQueueChanged = true;
        }
    }
    else
    {
        req = new LoadingRequest;
        req->file = file;
        req->frameIndex = m_loadingQueueFrameIndex;

        m_loadingQueue.pushBack(req);
        m_loadingMap[file] = req;
        m_loadingQueueChanged = true;
    }
}

void ManagedThumbnailHelper::refreshThumbnail(ManagedFile* file)
{
    // TODO:
}

///---

ManagedFile* ManagedThumbnailHelper::popFileLoadingQueueOrSignalFence()
{
    auto lock = CreateLock(m_loadingQueueLock);

    if (auto* file = popFileLoadingQueue_NoLock())
        return file;

    DEBUG_CHECK_EX(!m_loadingJobFence.empty(), "Ups, we were not loading");
    SignalFence(m_loadingJobFence); // no more loading, jobs done
    return nullptr;
}

ManagedFile* ManagedThumbnailHelper::popFileLoadingQueue_NoLock()
{
    if (m_loadingQueue.empty())
        return nullptr;

    auto* frontRequest = m_loadingQueue.front();
    m_loadingQueue.erase(0); // TODO: optimize

    auto fileToLoad = frontRequest->file;
    delete frontRequest;

    return fileToLoad;
}

void ManagedThumbnailHelper::processPendingUpdates()
{
    PC_SCOPE_LVL1(UpdateThumbnails);

    // sort loading queue, so more urgent requests are processed first
    {
        auto lock = CreateLock(m_loadingQueueLock);
        m_loadingQueueFrameIndex += 1;

        if (m_loadingQueueChanged)
        {
            std::stable_sort(m_loadingQueue.begin(), m_loadingQueue.end(), [](LoadingRequest* a, LoadingRequest* b)
                {
                    return a->frameIndex < b->frameIndex;
                });
        }

        // start loading job if not already running
        if (m_loadingJobFence.empty())
        {
            if (auto* file = popFileLoadingQueue_NoLock())
            {
                m_loadingJobFence = CreateFence("ThumbnailLoadingFence", 1);

                RunFiber("ThumbnailLoading") << [this, file](FIBER_FUNC)
                {
                    processLoadingJob(file); 
                };
            }
        }
    }
}

///----

void ManagedThumbnailHelper::loadThumbnailData(ManagedFile* file)
{
    /*PC_SCOPE_LVL1(LoadThumbnailData);
    StringBuf filePath = TempString("{}.thumb", file->depotPath());
    if (auto loader = m_depot.createFileReader(filePath))
    {
        NativeFileReader reader(*loader);
        if (auto data = rtti_cast<ResourceThumbnail>(LoadUncached(filePath, ResourceThumbnail::GetStaticClass(), reader)))
        {
            file->newThumbnailDataAvaiable(*data);
        }
        else
        {
            TRACE_WARNING("Failed to load thumbnail data from .thumb file '{}'. File may be corrupted.", file->depotPath());
        }
    }
    else
    {
        TRACE_INFO("No thumbnail data for '{}'", file->depotPath());
    }*/
}

void ManagedThumbnailHelper::processLoadingJob(ManagedFile* file)
{
    // load data for the requested file
    loadThumbnailData(file);

    // pick next file to load or, if there's nothing, signal the fence
    if (auto nextFile = popFileLoadingQueueOrSignalFence())
    {
        // schedule loading for next file
        RunFiber("ThumbnailLoading") << [this, file](FIBER_FUNC)
        {
            processLoadingJob(file);
        };
    }
}

//---

END_BOOMER_NAMESPACE_EX(ed)



