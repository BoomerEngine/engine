/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "glObject.h"
#include "glDevice.h"
#include "glDeviceThread.h"
#include "glDeviceThreadCopy.h"
#include "glFrame.h"

#include "rendering/device/include/renderingCommandBuffer.h"


namespace rendering
{
    namespace gl4
    {

        //--

        base::ConfigProperty<bool> cvPrintGLTimings("GL4", "PrintThreadTimings", false);
        base::ConfigProperty<bool> cvEnableProcessingThread("GL4", "EnableProcessingThread", true);
        base::ConfigProperty<bool> cvEnableDebugOutput("GL4", "EnableDebugOutput", true);

		base::ConfigProperty<uint32_t> cvCopyQueueStagingAreaSizeMB("GL4", "CopyStagingAreaSizeMB", 256);
		base::ConfigProperty<uint32_t> cvCopyQueueStagingAreaPageSize("GL4", "CopyQueueStagingAreaPageSize", 4096);

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

        DeviceThread::DeviceThread(Device* drv, WindowManager* windows)
            : m_jobQueueSemaphore(0, 65536)
            , m_useThread(cvEnableProcessingThread.get())
            , m_requestExit(0)
            , m_windows(windows)
            , m_cleanupSync("InternalGLSync")
            , m_device(drv)
        {
            base::ThreadSetup setup;
            setup.m_priority = base::ThreadPriority::AboveNormal;
            setup.m_name = "GL4DriverThread";
            setup.m_function = [this]() { threadFunc();  };
            setup.m_stackSize = 1U << 20;

            m_currentFrame = new Frame(this);
            
            if (m_useThread)
                m_thread.init(setup);			
        }

        DeviceThread::~DeviceThread()
        {
            TRACE_INFO("Device thread shutting down");

			// delete queue
			run([this]() {
					delete m_copyQueue;
					m_copyQueue = nullptr;
					delete m_copyPool;
					m_copyPool = nullptr;
				});

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

            TRACE_INFO("Device thread closed");
        }        

		void DeviceThread::initializeCopyQueue()
		{
			run([this]()
				{
					const auto copyPoolSize = cvCopyQueueStagingAreaSizeMB.get() << 20;
					const auto copyPoolPageSize = cvCopyQueueStagingAreaPageSize.get();
					m_copyPool = new DeviceCopyStagingPool(m_device, copyPoolSize, copyPoolPageSize);
					m_copyQueue = new DeviceCopyQueue(m_device, m_copyPool, &m_device->objectRegistry());
				});
		}

        void DeviceThread::initializeDebug_Thread()
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

            GL_PROTECT(glEnable(GL_DEBUG_OUTPUT));
            GL_PROTECT(glDebugMessageCallback(&prv::GLDebugPrint, nullptr));
        }

        void DeviceThread::closeFinishedFrames_Thread()
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

            PC_SCOPE_LVL1(CleanupPendingFrames);

            // get finished frames
            base::InplaceArray<Frame*, 10> finishedFrames;
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

        void DeviceThread::advanceFrame()
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
                m_currentFrame = new Frame(this);
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

        void DeviceThread::sync()
        {
            PC_SCOPE_LVL0(DeviceSync);

            ASSERT_EX(Fibers::GetInstance().isMainThread(), "Expected main thread");

            base::ScopeTimer timer;

            // if there is an active frame close it as no more work will be ever submitted to it
            if (nullptr != m_currentFrame)
            {
                auto lock = base::CreateLock(m_sequenceLock);
                m_sequencePendingList.pushBack(m_currentFrame);
                m_currentFrame = new Frame(this);
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
            //m_currentFrame = new Frame(this);

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

        void DeviceThread::releaseObject(Object* ptr)
        {
            if (ptr)
            {
                auto lock = base::CreateLock(m_sequenceLock);
                m_currentFrame->registerObjectForDeletion(ptr);
            }
        }

        void DeviceThread::pushJob(const std::function<void()>& func)
        {
            auto job  = new Job;
            job->m_jobFunc = func;

            {
                auto lock = base::CreateLock(m_jobQueueLock);
                m_jobQueue.push(job);
            }

            m_jobQueueSemaphore.release(1);
        }

        DeviceThread::Job* DeviceThread::popJob()
        {
            auto lock = base::CreateLock(m_jobQueueLock);

            if (m_jobQueue.empty())
                return nullptr;

            auto job  = m_jobQueue.top();
            m_jobQueue.pop();
            return job;
        }

        void DeviceThread::run(const std::function<void()>& func)
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

        void DeviceThread::submit(command::CommandBuffer* masterCommandBuffer)
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
                auto stats = base::RefNew<PerformanceStats>();
                ExecuteCommands(m_device, this, frame, stats, masterCommandBuffer);

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

        GLuint DeviceThread::createQuery()
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

        void DeviceThread::releaseQuery(GLuint id)
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "Only allowed on render thread");

            if (id)
                m_freeQueryObjects.pushBack(id);
        }

		//--

		bool DeviceThread::scheduleAsyncCopy_ClientApi(Object* ptr, const ResourceCopyRange& range, const ISourceDataProvider* sourceData, base::fibers::WaitCounter fence)
		{
			return m_copyQueue->scheduleAsync_ClientApi(ptr, range, sourceData, fence);
		}

		//--

        void DeviceThread::threadFunc()
        {
            if (m_useThread)
            {
                TRACE_INFO("GL4 device thread started");

                m_threadId = base::GetCurrentThreadID();

                for (;;)
                {
					// start/finish any async copying
					if (m_copyQueue)
					{
						auto lock = CreateLock(m_sequenceLock);
						m_copyQueue->update(m_currentFrame);
					}

                    // wait for job
                    m_jobQueueSemaphore.wait(1);

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
					if (m_copyQueue)
					{
						auto lock = CreateLock(m_sequenceLock);
						m_copyQueue->update(m_currentFrame);
					}

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

    } // gl4
} // rendering
