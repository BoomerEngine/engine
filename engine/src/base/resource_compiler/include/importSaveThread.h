/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "base/system/include/semaphoreCounter.h"
#include "base/containers/include/queue.h"
#include "base/system/include/thread.h"

namespace base
{
    namespace res
    {
        //--

        /// importer saving thread, saves back to depot
        class BASE_RESOURCE_COMPILER_API ImportSaverThread : public NoCopy
        {
        public:
            ImportSaverThread(depot::DepotStructure& depot);
            ~ImportSaverThread(); // note: will kill all jobs

            /// wait for jobs to finish
            void waitUntilDone();

            /// schedule new content for saving
            bool scheduleSave(const ResourcePtr& data, const StringBuf& depotPath);

        private:
            struct SaveJob : public NoCopy
            {
                res::ResourcePtr unsavedResource;
                StringBuf depotPath;
            };

            Queue<SaveJob*> m_saveJobQueue;
            res::ResourcePtr m_saveCurrentResource;
            SpinLock m_saveQueueLock;

            Semaphore m_saveThreadSemaphore;
            Thread m_saveThread;

            std::atomic<uint32_t> m_saveThreadRequestExit;

            ///--

            depot::DepotStructure& m_depot;

            ///--

            void processSavingThread();
            bool saveSingleFile(const res::ResourcePtr& data, const StringBuf& path);
        };

        //--

    } // cooker
} // base