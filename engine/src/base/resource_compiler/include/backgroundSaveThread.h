/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/app/include/localService.h"
#include "base/socket/include/tcpServer.h"
#include "base/resource/include/resourceLoader.h"
#include "base/system/include/semaphoreCounter.h"

namespace base
{
    namespace res
    {
        //--

        /// background saving thread
        class BASE_RESOURCE_COMPILER_API BackgroundSaveThread : public NoCopy
        {
        public:
            BackgroundSaveThread(depot::DepotStructure& depot);
            ~BackgroundSaveThread();

            /// schedule new content for saving
            bool scheduleSave(const StringBuf& depotPath, const ResourcePtr& data);

        private:
            struct SaveJob : public NoCopy
            {
                ResourcePtr unsavedResource;
                ResourceMountPoint mountPoint;
                std::atomic<bool> canceled = false;
                StringBuf depotPath;
            };

            Queue<SaveJob*> m_saveJobQueue;
            HashMap<StringBuf, SaveJob*> m_saveJobMap;
            SpinLock m_saveQueueLock;

            Semaphore m_saveThreadSemaphore;
            Thread m_saveThread;

            std::atomic<uint32_t> m_saveThreadRequestExit;

            depot::DepotStructure& m_depot;

            ///--

            void processSavingThread();
        };

        //--

    } // res
} // base