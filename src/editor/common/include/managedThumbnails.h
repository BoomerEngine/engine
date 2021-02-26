/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "core/io/include/ioDirectoryWatcher.h"
#include "core/system/include/spinLock.h"
#include "core/system/include/mutex.h"
#include "core/resource/include/resourceLoader.h"
#include "core/io/include/timestamp.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

/// frontend for thumbnail loading/generation
class EDITOR_COMMON_API ManagedThumbnailHelper : public NoCopy
{
public:
    ManagedThumbnailHelper();
    ~ManagedThumbnailHelper();

    ///---

    /// process pending thumbnail updates (main thread call)
    void processPendingUpdates();

    /// request a thumbnail to be loaded 
    void loadThumbnail(ManagedFile* file);

    /// create a request fro generating the thumbnail for given file
    /// NOTE: this will happen regardless if the thumbnail exists or not
    void refreshThumbnail(ManagedFile* file);

private:
    //---

    // pending loading request
    struct LoadingRequest
    {
        RTTI_DECLARE_POOL(POOL_MANAGED_DEPOT)
    public:

        ManagedFile* file;
        uint32_t frameIndex;
    };

    // list of thumbnails to load files are processed with the order they have here
    Array<LoadingRequest*> m_loadingQueue;
    HashMap<ManagedFile*, LoadingRequest*> m_loadingMap;
    bool m_loadingQueueChanged = false;
    uint32_t m_loadingQueueFrameIndex = 0;
    SpinLock m_loadingQueueLock;
    fibers::WaitCounter m_loadingJobFence; // if valid means we are loading something

    // stateless, load a thumbnail for given file
    void loadThumbnailData(ManagedFile* file);
    void processLoadingJob(ManagedFile* file);

    // get next file to load a thumbnail for
    ManagedFile* popFileLoadingQueueOrSignalFence();
    ManagedFile* popFileLoadingQueue_NoLock();           
};

//---

END_BOOMER_NAMESPACE_EX(ed)
 