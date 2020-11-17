/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver #]
***/

#include "build.h"
#include "glDriver.h"
#include "glDriverThread.h"
#include "glMicropExecutor.h"

#include "rendering/driver/include/renderingCommandBuffer.h"

namespace rendering
{
    namespace gl4
    {

        //--

        base::ConfigProperty<bool> cvPrintGLTimings("GL4", "PrintThreadTimings", false);
        base::ConfigProperty<bool> cvEnableProcessingThread("GL4", "EnableProcessingThread", true);
        base::ConfigProperty<bool> cvEnableDebugOutput("GL4", "EnableDebugOutput", true);

        //--

        namespace prv
        {

            void GLDebugPrint(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
            {
                if (id == 131222)
                    return;
                if (id == 131154)
                    return;

                // skip over debug info
                if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
                    return;

                // skip over if disabled
                if (!cvEnableDebugOutput.get())
                    return;

                // determine the source name
                const char* sourceName = "UnknownSource";
                switch (source) {
                case GL_DEBUG_SOURCE_API:
                    sourceName = "API";
                    break;
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                    sourceName = "WindowSystem";
                    break;
                case GL_DEBUG_SOURCE_SHADER_COMPILER:
                    sourceName = "ShaderCompiler";
                    break;
                case GL_DEBUG_SOURCE_THIRD_PARTY:
                    sourceName = "ThirdParty";
                    break;
                case GL_DEBUG_SOURCE_APPLICATION:
                    sourceName = "App";
                    break;
                case GL_DEBUG_SOURCE_OTHER:
                    sourceName = "Other";
                    break;
                }

                // determine message type
                const char* typeName = "UnknownType";
                switch (type)
                {
                    case GL_DEBUG_TYPE_ERROR: typeName = "Error"; break;
                    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeName = "DeprectedBehavior"; break;
                    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeName = "UndefinedBehavior"; break;
                    case GL_DEBUG_TYPE_PORTABILITY: typeName = "PortabilityIssues"; break;
                    case GL_DEBUG_TYPE_PERFORMANCE: typeName = "PerformanceIssue"; break;
                    case GL_DEBUG_TYPE_MARKER: typeName = "Marker"; break;
                    case GL_DEBUG_TYPE_PUSH_GROUP: typeName = "PushGroup"; break;
                    case GL_DEBUG_TYPE_POP_GROUP: typeName = "PopGroup"; break;
                    case GL_DEBUG_TYPE_OTHER: typeName = "Other"; break;
                }

                // determine severity
                const char* severityName = "UnknownSeverity";
                switch (severity)
                {
                case GL_DEBUG_SEVERITY_HIGH: severityName = "High"; break;
                case GL_DEBUG_SEVERITY_MEDIUM: severityName = "Medium"; break;
                case GL_DEBUG_SEVERITY_LOW: severityName = "Low"; break;
                case GL_DEBUG_SEVERITY_NOTIFICATION: severityName = "Info"; break;
                }

                // print
                if (type == GL_DEBUG_TYPE_ERROR)
                {
                    TRACE_ERROR("{}({}) from {}: ({}) {}", typeName, severityName, sourceName, id, message);
                }
                else
                {
                    TRACE_INFO("{}({}) from {}: ({}) {}", typeName, severityName, sourceName, id, message);
                }
            }

        } // prv

        //--

        DriverOutput::DriverOutput(Driver* drv, DriverOutputClass cls)
            : Object(drv, ObjectType::Output)
            , outputClass(cls)
        {}

        DriverOutput::~DriverOutput()
        {
            DEBUG_CHECK_EX(deviceHandle == 0, "Output still has a device context");
            DEBUG_CHECK_EX(windowHandle == 0, "Output still has a window");
        }

        bool DriverOutput::CheckClassType(ObjectType type)
        {
            return type == ObjectType::Output;
        }

        //--

        DriverThread::DriverThread(Driver* drv, WindowManager* windows)
            : m_jobQueueSemaphore(0, 65536)
            , m_useThread(cvEnableProcessingThread.get())
            , m_requestExit(0)
            , m_windows(windows)
            , m_cleanupSync("InternalGLSync")
            , m_driver(drv)
        {
            base::ThreadSetup setup;
            setup.m_priority = base::ThreadPriority::AboveNormal;
            setup.m_name = "GL4DriverThread";
            setup.m_function = [this]() { threadFunc();  };
            setup.m_stackSize = 1U << 20;

            m_currentFrame = new DriverFrame(this);
            
            if (m_useThread)
                m_thread.init(setup);
        }

        DriverThread::~DriverThread()
        {
            TRACE_INFO("Driver thread shutting down");

            // wait for all work on the GPU to finish
            sync();

            // delete unsubmitted frame
            delete m_currentFrame;
            m_currentFrame = nullptr;

            // close the rendering thread
            if (m_useThread)
            {
                TRACE_INFO("Waiting for thread to stop");
                m_requestExit = true;
                m_thread.close();
            }

            // close all abandoned windows
            for (auto window : m_windowsToClose)
                m_windows->closeWindow(window);

            TRACE_INFO("Driver thread closed");
        }

        void DriverThread::postWindowForDeletion_Thread(uint64_t windowHandle)
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

            if (windowHandle)
            {
                m_windows->disconnectWindow(windowHandle);

                auto lock = CreateLock(m_windowsToCloseLock);
                m_windowsToClose.pushBack(windowHandle);
            }
        }

        void DriverThread::finishObjectDeletion_Thread(Object* obj)
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

            if (obj->objectType() == ObjectType::Output)
                releaseOutput(obj->handle());

            delete obj;
        }

        void DriverThread::initializeDebug_Thread()
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

            GL_PROTECT(glEnable(GL_DEBUG_OUTPUT));
            GL_PROTECT(glDebugMessageCallback(&prv::GLDebugPrint, nullptr));
        }

        void DriverThread::closeFinishedFrames_Thread()
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

            PC_SCOPE_LVL1(CleanupPendingFrames);

            // get finished frames
            base::InplaceArray<DriverFrame*, 10> finishedFrames;
            {
                auto lock = base::CreateLock(m_sequenceLock);

                bool sequencesFinished = false;
                for (uint32_t i = 0; i < m_sequencePendingList.size(); ++i)
                {
                    auto pendingSequnce = m_sequencePendingList[i];
                    if (pendingSequnce->checkFences())
                    {
                        // delete the sequence object, this will call the completion fences
                        sequencesFinished = true;
                        m_sequencePendingList[i] = nullptr;
                        finishedFrames.pushBack(pendingSequnce);
                    }
                }

                // remove empty slots
                if (sequencesFinished)
                    m_sequencePendingList.removeAll(nullptr);
            }

            // clear frames
            finishedFrames.clearPtr();
        }

        void DriverThread::advanceFrame()
        {
            ASSERT_EX(Fibers::GetInstance().isMainThread(), "Expected main thread");

            // wait if previous cleanup job has not yet finished
            {
                base::ScopeTimer timer;
                m_cleanupSync.acquire();

                auto elapsed = timer.timeElapsed();
                if (elapsed > 0.0001 && cvPrintGLTimings.get())
                {
                    TRACE_ERROR("GL Waiting for GPU: {}", TimeInterval(elapsed));
                }
            }

            // get the frame that is ending
            {
                auto lock = base::CreateLock(m_sequenceLock);

                if (m_currentFrame)
                    m_sequencePendingList.pushBack(m_currentFrame);
                m_currentFrame = new DriverFrame(this);
            }

            // push a cleanup job to release objects from frames that were completed
            auto cleanup = [this]()
            {
                closeFinishedFrames_Thread();
                m_cleanupSync.release();
            };

            // post job
            pushJob(cleanup);

            // process all jobs now
            if (!m_useThread)
                threadFunc();

            // close windows used by outputs that are not closed
            {
                auto lock = CreateLock(m_windowsToCloseLock);
                for (auto wnd : m_windowsToClose)
                    m_windows->closeWindow(wnd);
                m_windowsToClose.reset();
            }
        }

        void DriverThread::sync()
        {
            PC_SCOPE_LVL0(DeviceSync);

            ASSERT_EX(Fibers::GetInstance().isMainThread(), "Expected main thread");

            base::ScopeTimer timer;

            // if there is an active frame close it as no more work will be ever submitted to it
            if (nullptr != m_currentFrame)
            {
                auto lock = base::CreateLock(m_sequenceLock);
                m_sequencePendingList.pushBack(m_currentFrame);
                m_currentFrame = new DriverFrame(this);
            }

            // all frames should be finished 
            run([this]() {
                // wait for all GPU work to finish
                {
                    auto glFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

                    glFlush();

                    while (1)
                    {
                        auto ret = glClientWaitSync(glFence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000);
                        if (ret != GL_TIMEOUT_EXPIRED)
                            break;
                    }

                    glDeleteSync(glFence);
                }

                // process all fenced from finished frames
                closeFinishedFrames_Thread();
            });

            // make sure they indeed area
            DEBUG_CHECK_EX(m_sequencePendingList.empty(), "There are still some unfinished frames after device sync, that should not happen since all fenced should be signalled");
            //DEBUG_CHECK_EX(nullptr == m_currentFrame, "There are still some unfinished frames after device sync, that should not happen since all fenced should be signalled");

            // start new, fresh frame
            //m_currentFrame = new DriverFrame(this);

            // notify if elapsed time is outstandingly long
            auto elapsedTime = timer.milisecondsElapsed();
            if (elapsedTime > 150.0f)
            {
                TRACE_WARNING("Long device sync ({})", timer);
            }
            else if (elapsedTime > 500.0f)
            {
                TRACE_ERROR("VERY Long device sync ({})", timer);
            }
        }

        void DriverThread::releaseObject(Object* ptr)
        {
            if (ptr)
            {
                auto lock = base::CreateLock(m_sequenceLock);
                m_currentFrame->registerObjectForDeletion(ptr);
            }
        }

        void DriverThread::pushJob(const std::function<void()>& func)
        {
            auto job  = new Job;
            job->m_jobFunc = func;

            {
                auto lock = base::CreateLock(m_jobQueueLock);
                m_jobQueue.push(job);
            }

            m_jobQueueSemaphore.release(1);
        }

        DriverThread::Job* DriverThread::popJob()
        {
            auto lock = base::CreateLock(m_jobQueueLock);

            if (m_jobQueue.empty())
                return nullptr;

            auto job  = m_jobQueue.top();
            m_jobQueue.pop();
            return job;
        }

        void DriverThread::run(const std::function<void()>& func)
        {
            PC_SCOPE_LVL1(RunOnDriver);

            auto signal = Fibers::GetInstance().createCounter("GLJobCounter", 1);

            auto job = [this, signal, func]()
            {
                func();
                Fibers::GetInstance().signalCounter(signal);
            };

            pushJob(job);

            if (!m_useThread)
                threadFunc();

            Fibers::GetInstance().waitForCounterAndRelease(signal);
        }

        IDriverNativeWindowInterface* DriverThread::queryOutputWindow(ObjectID id) const
        {
            const auto* output = ResolveStaticObject<DriverOutput>(id);
            DEBUG_CHECK_EX(output != nullptr, "Not an output object");
            if (output)
                return m_windows->windowInterface(output->windowHandle);

            return nullptr;
        }

        bool DriverThread::prepareOutputFrame(ObjectID id, DriverOutputFrameInfo& outFrameInfo)
        {
            const auto* output = ResolveStaticObject<DriverOutput>(id);
            DEBUG_CHECK_EX(output != nullptr, "Not an output object");
            if (output)
            {
                const auto maxSize = m_driver->maxRenderTargetSize();

                outFrameInfo.colorFormat = output->colorFormat;
                outFrameInfo.depthFormat = output->depthFormat;
                outFrameInfo.maxWidth = maxSize.x;
                outFrameInfo.maxHeight = maxSize.y;
                outFrameInfo.samples = 1; // TODO

                if (!m_windows->prepareWindowForRendering(output->windowHandle, outFrameInfo.width, outFrameInfo.height))
                    return false;

                outFrameInfo.viewport.min.x = 0;
                outFrameInfo.viewport.min.y = 0;
                outFrameInfo.viewport.max.x = outFrameInfo.width;
                outFrameInfo.viewport.max.y = outFrameInfo.height;
                outFrameInfo.flippedY = true;

                return true;
            }

            return nullptr;
        }

        void DriverThread::submit(command::CommandBuffer* masterCommandBuffer)
        {
            // no work, can happen as 0 is a valid state
            if (!masterCommandBuffer)
                return;

            // schedule work
            auto lock = base::CreateLock(m_sequenceLock);

            // look at the current sequence, if there were frames submitted add the sequence to the completion list
            // if there was nothing submitted keep the current sequence as it is
            ASSERT(m_currentFrame != nullptr);
            m_currentFrame->attachPendingFrame();

            // create a job
            auto frame = m_currentFrame;
            auto job = [this, masterCommandBuffer, frame]()
            {
                // execute frame
                auto stats = base::RefNew<DriverPerformanceStats>();
                ExecuteCommands(m_driver, this, frame, stats, masterCommandBuffer);

                // create a pending frame fence and add it to the current sequence
                // NOTE: only after all fences complete we will try to close the opened sequence and release the resources
                auto glFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                frame->attachRecordedFrame(glFence);

                // release command buffers to pool
                masterCommandBuffer->release();

            };

            // post a job
            pushJob(job);
        }

        GLuint DriverThread::createQuery()
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "Only allowed on render thread");

            GLuint ret = 0;

            if (m_freeQueryObjects.empty())
            {
                GL_PROTECT(glGenQueries(1, &ret));
            }
            else
            {
                ret = m_freeQueryObjects.back();
                m_freeQueryObjects.popBack();
            }

            return ret;
        }

        void DriverThread::releaseQuery(GLuint id)
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "Only allowed on render thread");

            if (id)
                m_freeQueryObjects.pushBack(id);
        }

        void DriverThread::threadFunc()
        {
            if (m_useThread)
            {
                TRACE_INFO("GL4 device thread started");

                m_threadId = base::GetCurrentThreadID();

                for (;;)
                {
                    // wait for job
                    m_jobQueueSemaphore.wait(10);

                    // request exit ?
                    if (m_requestExit)
                        break;

                    // get the job
                    auto job = popJob();
                    if (!job)
                        continue;

                    // run the job
                    {
                        base::ScopeTimer timer;
                        PC_SCOPE_LVL0(DeviceThreadJob);
                        job->m_jobFunc();
                        delete job;

                        if (cvPrintGLTimings.get())
                            TRACE_INFO("GL RenderJob: {}", TimeInterval(timer.timeElapsed()));

                    }
                }

                TRACE_INFO("GL4 device thread stopped");
            }
            else
            {
                // manual job loop
                while (auto job = popJob())
                {
                    base::ScopeTimer timer;
                    PC_SCOPE_LVL0(DeviceThreadJob);
                    job->m_jobFunc();
                    delete job;

                    if (cvPrintGLTimings.get())
                        TRACE_INFO("GL RenderJob: {}", TimeInterval(timer.timeElapsed()));
                }
            }
        }

        //--

        DriverFrame::DriverFrame(DriverThread* thread)
            : m_numDeclaredFrames(0)
            , m_numRecordedFrames(0)
            , m_thread(thread)
        {
            m_pendingFences.reserve(4);
        }

        DriverFrame::~DriverFrame()
        {
            ASSERT_EX(m_pendingFences.empty(), "Deleting sequence with unfinished fences");

            // call completion callbacks
            for (auto callback  : m_completionCallbacks)
                (*callback)();
            m_completionCallbacks.clearPtr();

            // delete object
            if (!m_deferedDeletionObjects.empty())
            {
                TRACE_INFO("Has {} objects to delete", m_deferedDeletionObjects.size());
                for (auto obj : m_deferedDeletionObjects)
                    m_thread->finishObjectDeletion_Thread(obj);
            }
        }

        void DriverFrame::attachPendingFrame()
        {
            m_numDeclaredFrames += 1;
        }

        void DriverFrame::attachRecordedFrame(GLsync sync)
        {
            auto lock = base::CreateLock(m_pendingFencesLock);

            m_numRecordedFrames += 1;
            m_pendingFences.pushBack(sync);
            ASSERT_EX(m_numRecordedFrames <= m_numDeclaredFrames, "More fences than declared frames");
            ASSERT_EX(m_pendingFences.size() <= m_numRecordedFrames, "More fences than declared frames");
        }

        bool DriverFrame::checkFences()
        {
            // take the lock
            auto lock = base::CreateLock(m_pendingFencesLock);

            // check if the fence has completed
            for (int j = m_pendingFences.lastValidIndex(); j >= 0; --j)
            {
                auto fence = m_pendingFences[j];
                auto ret = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);

                if (ret == GL_ALREADY_SIGNALED || ret == GL_CONDITION_SATISFIED)
                {
                    // fence was completed, remove it from the list
                    m_pendingFences.removeUnordered(fence);
                    glDeleteSync(fence);
                }
                else if (ret == GL_TIMEOUT_EXPIRED)
                {
                    // fence was not yet completed
                }
                else
                {
                    // something failed, warn and remove the fence to prevent deadlocks
                    auto error = glGetError();
                    TRACE_WARNING("Frame fence failed with return code {} and error code {}", ret, error);
                    m_pendingFences.removeUnordered(fence);
                    glDeleteSync(fence);
                } 
            }

            // return true if all fences were completed
            return m_pendingFences.empty() && (m_numDeclaredFrames == m_numRecordedFrames);
        }

        void DriverFrame::registerObjectForDeletion(Object* obj)
        {
            if (obj)
            {
                auto lock = base::CreateLock(m_completionCallbacksLock);
                m_deferedDeletionObjects.pushBack(obj);
            }
        }

        void DriverFrame::registerCompletionCallback(SequenceCompletionCallback callback)
        {
            if (callback)
            {
                auto lock = base::CreateLock(m_completionCallbacksLock);

                auto callBackCopy = new SequenceCompletionCallback(std::move(callback));
                m_completionCallbacks.emplaceBack(callBackCopy);
            }
        }

        //--

    } // gl4
} // device
