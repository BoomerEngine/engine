/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiObject.h"
#include "apiObjectRegistry.h"
#include "apiObjectCache.h"
#include "apiThread.h"
#include "apiFrame.h"
#include "apiWindow.h"
#include "apiCopyQueue.h"
#include "apiExecution.h"
#include "apiTransientBuffer.h"
#include "apiCapture.h"

#include "rendering/device/include/renderingCommandBuffer.h"

namespace rendering
{
    namespace api
    {

		//--

		base::ConfigProperty<uint32_t> cvCopyQueueStagingAreaSizeMB("Rendering", "CopyStagingAreaSizeMB", 256);
		base::ConfigProperty<uint32_t> cvCopyQueueStagingAreaPageSize("Rendering", "CopyQueueStagingAreaPageSize", 4096);

        //--

        IBaseThread::IBaseThread(IBaseDevice* drv, WindowManager* windows)
            : m_jobQueueSemaphore(0, 65536)
            , m_requestExit(0)
            , m_windows(windows)
            , m_cleanupSync("InternalGLSync")
            , m_device(drv)
        {
            m_currentFrame = new Frame(this, ++m_currentFrameIndex);
        }

		IBaseThread::~IBaseThread()
		{
			ASSERT(m_currentFrame == nullptr);
		}

		void IBaseThread::stopThread()
		{
            TRACE_INFO("Device thread shutting down");

			// finish any high level rendering
			sync();

			// delete queue
			run([this]() {
				threadFinish();
				});

            // delete final frame - should be empty
            delete m_currentFrame;
            m_currentFrame = nullptr;

            // close the rendering thread
            TRACE_INFO("Waiting for thread to stop");
            m_requestExit = true;
            m_thread.close();

            // close all abandoned windows
            for (auto window : m_windowsToClose)
                m_windows->closeWindow(window);

            TRACE_INFO("Device thread closed");
        }        

		bool IBaseThread::startThread(const base::app::CommandLine& cmdLine)
		{
			base::ThreadSetup setup;
			setup.m_priority = base::ThreadPriority::AboveNormal;
			setup.m_name = "RenderingThread";
			setup.m_function = [this]() { threadFunc();  };
			setup.m_stackSize = 1U << 20;

			m_thread.init(setup);

			bool status = true;
			run([this, &status, &cmdLine]()
				{
					status = threadStartup(cmdLine);
				});

			return status;			
		}

		ObjectRegistry* IBaseThread::createOptimalObjectRegistry(const base::app::CommandLine& cmdLine)
		{
			return new ObjectRegistry(this);
		}

		bool IBaseThread::threadStartup(const base::app::CommandLine& cmdLine)
		{
			DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

			const auto copyPoolSize = cvCopyQueueStagingAreaSizeMB.get() << 20;
			const auto copyPoolPageSize = cvCopyQueueStagingAreaPageSize.get();

			// create object registry
			m_objectRegistry = AddRef(createOptimalObjectRegistry(cmdLine));
			m_objectCache = createOptimalObjectCache(cmdLine);

			// create copy queue and pool
			m_copyPool = createOptimalStagingPool(copyPoolSize, copyPoolPageSize, cmdLine);
			m_copyQueue = createOptimalCopyQueue(cmdLine);

			// create transient data pools
			m_transientConstantPool = createOptimalTransientConstantPool(cmdLine);
			m_transientStagingPool = createOptimalTransientStagingPool(cmdLine);

			return true;
		}

		void IBaseThread::threadFinish()
		{
			DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

			delete m_objectRegistry;
			m_objectRegistry = nullptr;

			delete m_objectCache;
			m_objectCache = nullptr;

			delete m_copyQueue;
			m_copyQueue = nullptr;

			delete m_copyPool;
			m_copyPool = nullptr;

			delete m_transientStagingPool;
			m_transientStagingPool = nullptr;

			delete m_transientConstantPool;
			m_transientConstantPool = nullptr;

		}

        void IBaseThread::closeFinishedFrames_Thread()
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

        void IBaseThread::advanceFrame()
        {
            ASSERT_EX(Fibers::GetInstance().isMainThread(), "Expected main thread");

            // wait if previous cleanup job has not yet finished
            {
                base::ScopeTimer timer;
                m_cleanupSync.acquire();

                /*auto elapsed = timer.timeElapsed();
                if (elapsed > 0.0001)
                {
                    TRACE_ERROR("GL Waiting for GPU: {}", TimeInterval(elapsed));
                }*/
            }

            // get the frame that is ending
            {
                auto lock = base::CreateLock(m_sequenceLock);

                if (m_currentFrame)
                    m_sequencePendingList.pushBack(m_currentFrame);
                m_currentFrame = new Frame(this, ++m_currentFrameIndex);
            }

            // push a cleanup job to release objects from frames that were completed
            auto cleanup = [this]()
            {
                closeFinishedFrames_Thread();
                m_cleanupSync.release();
            };

            // post job
            pushJob(cleanup);

            // close windows used by outputs that are not closed
            {
                auto lock = CreateLock(m_windowsToCloseLock);
                for (auto wnd : m_windowsToClose)
                    m_windows->closeWindow(wnd);
                m_windowsToClose.reset();
            }
        }

        void IBaseThread::sync()
        {
            PC_SCOPE_LVL0(DeviceSync);

            ASSERT_EX(Fibers::GetInstance().isMainThread(), "Expected main thread");

            base::ScopeTimer timer;

            // if there is an active frame close it as no more work will be ever submitted to it
            if (nullptr != m_currentFrame)
            {
                auto lock = base::CreateLock(m_sequenceLock);
                m_sequencePendingList.pushBack(m_currentFrame);
                m_currentFrame = new Frame(this, ++m_currentFrameIndex);
            }

            // all frames should be finished 
			run([this]() {
				syncGPU_Thread();
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

        void IBaseThread::scheduleObjectForDestruction(IBaseObject* ptr)
        {
            auto lock = base::CreateLock(m_sequenceLock);
            m_currentFrame->registerObjectForDeletion(ptr);
        }

        void IBaseThread::pushJob(const std::function<void()>& func)
        {
            auto job  = new Job;
            job->m_jobFunc = func;

            {
                auto lock = base::CreateLock(m_jobQueueLock);
                m_jobQueue.push(job);
            }

            m_jobQueueSemaphore.release(1);
        }

        IBaseThread::Job* IBaseThread::popJob()
        {
			if (m_requestExit)
				return nullptr;

            auto lock = base::CreateLock(m_jobQueueLock);

            if (m_jobQueue.empty())
                return nullptr;

            auto job = m_jobQueue.top();
            m_jobQueue.pop();
            return job;
        }

        void IBaseThread::run(const std::function<void()>& func)
        {
            PC_SCOPE_LVL1(RunOnRenderThread);

            auto signal = Fibers::GetInstance().createCounter("RenderThreadInjectedJob", 1);

            auto job = [this, signal, func]()
            {
                func();
                Fibers::GetInstance().signalCounter(signal);
            };

            pushJob(job);

            Fibers::GetInstance().waitForCounterAndRelease(signal);
        }

		//--

		static void CollectCommandBuffers(command::CommandBuffer* commandBuffer, base::Array<command::CommandBuffer*>& outCommandBuffers)
		{
			commandBuffer->visitHierarchy([&outCommandBuffers](command::CommandBuffer* ptr)
				{
					outCommandBuffers.pushBack(ptr);
					return false;
				});
		}

		void BuildTransientData(IBaseThread* drv, Frame* seq, PerformanceStats& outStats, command::CommandBuffer* commandBuffer, RuntimeDataAllocations& outData)
		{
			PC_SCOPE_LVL2(BuildTransientData);

			// timer
			base::ScopeTimer timer;

			// collect all referenced command buffers
			base::InplaceArray<command::CommandBuffer*, 100> allCommandBuffers;
			CollectCommandBuffers(commandBuffer, allCommandBuffers);

			{
				PC_SCOPE_LVL2(Collect);

				// report constant usage
				for (auto& commandBuffer : allCommandBuffers)
				{
					if (commandBuffer->gatheredState().totalConstantsUploadSize > 0)
					{
						outData.reportConstantsBlockSize(commandBuffer->gatheredState().totalConstantsUploadSize);
						auto prev = commandBuffer->gatheredState().constantUploadHead;
						for (auto cur = commandBuffer->gatheredState().constantUploadHead; cur; cur = cur->nextConstants)
						{
							uint32_t mergedOffset = 0;

							const void* constData = cur->dataPtr;
							outData.reportConstData(cur->offset, cur->dataSize, constData, mergedOffset);
							cur->mergedRuntimeOffset = mergedOffset;
							prev = cur;

							outStats.uploadTotalSize += cur->dataSize;
							outStats.uploadConstantsCount += 1;
							outStats.uploadConstantsSize += cur->dataSize;
						}
					}
				}

				// report and prepare transient buffers
				for (auto& commandBuffer : allCommandBuffers)
				{
					/*for (auto cur = commandBuffer->gatheredState().triansienBufferAllocHead; cur; cur = cur->next)
					{
						// allocate the resident storage for the buffer on the GPU
						auto initialData = (cur->initializationDataSize != 0) ? cur->initializationData : nullptr;
						transientFrameBuilder.reportBuffer(cur->buffer, initialData, cur->initializationDataSize);

						if (cur->initializationDataSize > 0)
						{
							outStats.m_uploadTotalSize += cur->initializationDataSize;
							outStats.m_uploadTransientBufferCount += 1;
							outStats.m_uploadTransientBufferSize += cur->initializationDataSize;
						}
					}*/
				}

				// report the buffer updates
				for (auto& commandBuffer : allCommandBuffers)
				{
					for (auto cur = commandBuffer->gatheredState().dynamicBufferUpdatesHead; cur; cur = cur->next)
					{
						outData.reportBufferUpdate(cur->dataBlockPtr, cur->dataBlockSize, cur->stagingBufferOffset);

						if (cur->dataBlockSize > 0)
						{
							outStats.uploadTotalSize += cur->dataBlockSize;
							outStats.uploadDynamicBufferCount += 1;
							outStats.uploadDynamicBufferSize += cur->dataBlockSize;
						}
					}
				}
			}

			// update stats
			outStats.uploadTime = timer.timeElapsed();
		}

		//--

        void IBaseThread::submit(command::CommandBuffer* masterCommandBuffer)
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
				base::ScopeTimer totalTime;

				// capture if requested
				auto capture = IFrameCapture::ConditionalStartCapture(masterCommandBuffer);

				// collect stats
				auto stats = base::RefNew<PerformanceStats>();

				// build the transient data for the frame
				RuntimeDataAllocations frameData;
				BuildTransientData(this, frame, *stats, masterCommandBuffer, frameData);

                // execute frame
				execute_Thread(*frame, *stats, masterCommandBuffer, frameData);

                // create a pending frame fence and add it to the current sequence
                // NOTE: only after all fences complete we will try to close the opened sequence and release the resources
                if (auto fence = createOptimalFrameFence())
					frame->attachRecordedFrame(fence);

                // release command buffers to pool
                masterCommandBuffer->release();
            };

            // post a job
            pushJob(job);
        }

		//--

        void IBaseThread::threadFunc()
        {
            TRACE_INFO("Render thread started");

            m_threadId = base::GetCurrentThreadID();

            while (!m_requestExit)
            {
				// start/finish any async copying
				if (m_copyQueue)
				{
					auto lock = CreateLock(m_sequenceLock);
					m_copyQueue->update(m_currentFrame);
				}

                // wait for job
                m_jobQueueSemaphore.wait(1);

                // get the job
                if (auto job = popJob())
				{
                    PC_SCOPE_LVL0(IDeviceThreadJob);
                    job->m_jobFunc();
                    delete job;
                }
            }

            TRACE_INFO("Render thread stopped");
        }
        
        //--

    } // gl4
} // rendering
