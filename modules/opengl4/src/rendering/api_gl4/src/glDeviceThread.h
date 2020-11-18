/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "base/containers/include/hashSet.h"
#include "base/containers/include/queue.h"
#include "base/system/include/spinLock.h"
#include "base/system/include/thread.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/fibers/include/fiberSystem.h"
#include "base/fibers/include/fiberSyncPoint.h"

#include "rendering/device/include/renderingOutput.h"
#include "rendering/api_common/include/renderingWindow.h"

namespace rendering
{
    namespace gl4
    {

        //---

        // a OpenGL 4 driver thread, all things happen here
        class DeviceThread : public base::NoCopy, public base::mem::GlobalPoolObject<POOL_API_RUNTIME>
        {
        public:
            DeviceThread(Device* drv, WindowManager* windows);
            virtual ~DeviceThread();

            //--

            void advanceFrame();
            void sync();
            void run(const std::function<void()>& func);
            void submit(command::CommandBuffer* masterCommandBuffer);

            //--

            virtual ObjectID createOutput(const OutputInitInfo& info) = 0;
            virtual void bindOutput(ObjectID output) = 0;
            virtual void swapOutput(ObjectID output) = 0;

            //--

            void releaseObject(Object* ptr);

            //--

            GLuint createQuery();
            void releaseQuery(GLuint id);

            //--

            virtual void prepareWindowForDeletion_Thread(uint64_t windowHandle, uint64_t deviceHandle) = 0;
            virtual void postWindowForDeletion_Thread(uint64_t windowHandle) = 0;
            
        protected:
            bool m_useThread;
            volatile bool m_requestExit;
            base::Thread m_thread;
            uint32_t m_threadId = 0;

            base::SpinLock m_sequenceLock;
            base::Array<Frame*> m_sequencePendingList;
            Frame* m_currentFrame;

            Device* m_device;
            WindowManager* m_windows;

            base::fibers::SyncPoint m_cleanupSync;
            base::Array<GLuint> m_freeQueryObjects;

            //--

            base::Array<uint64_t> m_windowsToClose;
            base::Mutex m_windowsToCloseLock;

            //--

            struct Job : public base::mem::GlobalPoolObject<POOL_API_RUNTIME>
            {
                std::function<void()> m_jobFunc;
            };

            base::Queue<Job*> m_jobQueue;
            base::SpinLock m_jobQueueLock;
            base::Semaphore m_jobQueueSemaphore;

            Job* popJob();
            void pushJob(const std::function<void()>& func);

            //--
            
            void threadFunc();

            void closeFinishedFrames_Thread();
            void initializeDebug_Thread();

            friend class Frame;
        };

        //---

    } // gl4
} // rendering