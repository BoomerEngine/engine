/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/app/include/localService.h"
#include "base/socket/include/tcpServer.h"
#include "base/resources/include/resourceLoader.h"
#include "base/system/include/semaphoreCounter.h"

namespace base
{
    namespace cooker
    {
        //--

        /// cooker saving thread, saves to absolute paths
        class BASE_COOKING_API CookerSaveThread : public NoCopy
        {
        public:
            CookerSaveThread();
            ~CookerSaveThread(); // note: will kill all jobs

            /// wait for jobs to finish
            void waitUntilDone();

            /// schedule new content for saving
            bool scheduleSave(const res::ResourcePtr& data, const io::AbsolutePath& path);

        private:
            struct SaveJob : public NoCopy
            {
                res::ResourcePtr unsavedResource;
                io::AbsolutePath absoultePath;
            };

            Queue<SaveJob*> m_saveJobQueue;
            res::ResourcePtr m_saveCurrentResource;
            SpinLock m_saveQueueLock;

            Semaphore m_saveThreadSemaphore;
            Thread m_saveThread;

            std::atomic<uint32_t> m_saveThreadRequestExit;

            ///--

            void processSavingThread();
            bool saveSingleFile(const res::ResourcePtr& data, const io::AbsolutePath& path);
        };

        //--

    } // cooker
} // base