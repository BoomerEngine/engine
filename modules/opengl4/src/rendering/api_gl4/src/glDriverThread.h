/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver #]
***/

#pragma once

#include "base/containers/include/hashSet.h"
#include "base/containers/include/queue.h"
#include "base/system/include/spinLock.h"
#include "base/system/include/thread.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/fibers/include/fiberSystem.h"
#include "base/fibers/include/fiberSyncPoint.h"

#include "rendering/driver/include/renderingOutput.h"
#include "rendering/api_common/include/renderingWindow.h"

namespace rendering
{
    namespace gl4
    {
        //---

        class DriverThread;

        // a frame in progress
        class DriverFrame
        {
        public:
            DriverFrame(DriverThread* thread);
            ~DriverFrame(); // calls all the callbacks

            // attach a frame
            void attachPendingFrame();

            // attach a recorded frame
            void attachRecordedFrame(GLsync sync);

            // look to see if this sequence has ended
            // NOTE: will return true if all of the fences in the sequence are completed
            bool checkFences();

            // register a completion callback
            void registerCompletionCallback(SequenceCompletionCallback callback);

            // register an object for deferred deletion
            void registerObjectForDeletion(Object* obj);

        private:
            uint32_t m_numDeclaredFrames;
            uint32_t m_numRecordedFrames;
            base::Array<GLsync> m_pendingFences;
            base::SpinLock m_pendingFencesLock;

            base::Array<SequenceCompletionCallback*> m_completionCallbacks;
            base::SpinLock m_completionCallbacksLock;

            base::InplaceArray<Object*, 128> m_deferedDeletionObjects;
            base::SpinLock m_deferedDeletionObjectsLock;

            DriverThread* m_thread;
        };

        //---

        // an output object - OpenGL "swapchain" kind of object
        class DriverOutput : public Object
        {
        public:
            DriverOutput(Driver* drv, DriverOutputClass cls);
            virtual ~DriverOutput();

            DriverOutputClass outputClass;
            uint64_t windowHandle = 0; // HWND
            uint64_t deviceHandle = 0; // HDC

            ImageFormat colorFormat = ImageFormat::UNKNOWN;
            ImageFormat depthFormat = ImageFormat::UNKNOWN;

            static bool CheckClassType(ObjectType type);
        };       

        //---

        // a OpenGL 4 driver thread, all things happen here
        class DriverThread : public base::NoCopy
        {
        public:
            DriverThread(Driver* drv, WindowManager* windows);
            virtual ~DriverThread();

            //--

            void advanceFrame();
            void sync();
            void run(const std::function<void()>& func);
            void submit(command::CommandBuffer* masterCommandBuffer);

            //--

            virtual ObjectID createOutput(const DriverOutputInitInfo& info) = 0;
            virtual void releaseOutput(ObjectID output) = 0;
            virtual void bindOutput(ObjectID output) = 0;
            virtual void swapOutput(ObjectID output) = 0;

            IDriverNativeWindowInterface* queryOutputWindow(ObjectID output) const;
            bool prepareOutputFrame(ObjectID output, DriverOutputFrameInfo& outFrameInfo);

            //--

            void releaseObject(Object* ptr);

            //--

            GLuint createQuery();
            void releaseQuery(GLuint id);

        protected:
            bool m_useThread;
            volatile bool m_requestExit;
            base::Thread m_thread;
            uint32_t m_threadId = 0;

            base::SpinLock m_sequenceLock;
            base::Array<DriverFrame*> m_sequencePendingList;
            DriverFrame* m_currentFrame;

            Driver* m_driver;
            WindowManager* m_windows;

            base::fibers::SyncPoint m_cleanupSync;

            base::Array<GLuint> m_freeQueryObjects;

            //--

            base::Array<uint64_t> m_windowsToClose;
            base::Mutex m_windowsToCloseLock;

            //--

            struct Job
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
            void finishObjectDeletion_Thread(Object* obj);
            void postWindowForDeletion_Thread(uint64_t windowHandle);

            friend class DriverFrame;
        };

        //---

    } // gl4
} // driver