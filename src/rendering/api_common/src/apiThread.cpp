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
#include "apiWindow.h"
#include "apiExecution.h"
#include "apiCapture.h"
#include "apiBackgroundJobs.h"
#include "apiUtils.h"

#include "rendering/device/include/renderingCommandBuffer.h"

namespace rendering
{
    namespace api
    {

        //--

        IBaseThread::IBaseThread(IBaseDevice* drv, WindowManager* windows)
            : m_jobQueueSemaphore(0, 65536)
            , m_requestExit(0)
            , m_windows(windows)
            , m_cleanupSync("InternalGLSync")
            , m_device(drv)
        {
			memzero(&m_syncInfo, sizeof(m_syncInfo));

			m_syncQueues.cpuQueue = new FrameCompleteionQueue();
			m_syncQueues.recordQueue = new FrameCompleteionQueue();
			m_syncQueues.gpuQueue = new FrameCompleteionQueue();
        }

		IBaseThread::~IBaseThread()
		{
		}

		void IBaseThread::stopThread()
		{
            TRACE_INFO("Device thread shutting down");

			// mark all still live objects as not needed
			if (m_objectRegistry)
				m_objectRegistry->purge();

			// stop background queue
			if (m_backgroundQueue)
			{
				m_backgroundQueue->stop();
				delete m_backgroundQueue;
				m_backgroundQueue = nullptr;
			}

			// finish any high level rendering
			// NOTE: this will also remove all pending objects
			sync(true);
			sync(true);

			// delete queue
			run([this]() {
				threadFinish();
				});

            // close the rendering thread
            TRACE_INFO("Waiting for thread to stop");
            m_requestExit = true;
            m_thread.close();

			// delete queues, they should be empty now
			delete m_syncQueues.cpuQueue;
			m_syncQueues.cpuQueue = nullptr;
			delete m_syncQueues.recordQueue;
			m_syncQueues.recordQueue = nullptr;
			delete m_syncQueues.gpuQueue;
			m_syncQueues.gpuQueue = nullptr;

            TRACE_INFO("Device thread closed");
        }        

		bool IBaseThread::startThread(const base::app::CommandLine& cmdLine, DeviceCaps& outCaps)
		{
			base::ThreadSetup setup;
			setup.m_priority = base::ThreadPriority::AboveNormal;
			setup.m_name = "RenderingThread";
			setup.m_function = [this]() { threadFunc();  };
			setup.m_stackSize = 1U << 20;

			m_thread.init(setup);

			bool status = true;
			run([this, &status, &cmdLine, &outCaps]()
				{
					status = threadStartup(cmdLine, outCaps);
				});

			return status;			
		}

		ObjectRegistry* IBaseThread::createOptimalObjectRegistry(const base::app::CommandLine& cmdLine)
		{
			return new ObjectRegistry(this);
		}

		bool IBaseThread::threadStartup(const base::app::CommandLine& cmdLine, DeviceCaps& outCaps)
		{
			DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

			// start background threads
			m_backgroundQueue = createOptimalBackgroundQueue(cmdLine);
			if (!m_backgroundQueue->initialize(cmdLine))
				return false;

			// create object registry
			m_objectRegistry = AddRef(createOptimalObjectRegistry(cmdLine));
			m_objectCache = createOptimalObjectCache(cmdLine);

			return true;
		}

		void IBaseThread::threadFinish()
		{
			DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

			m_objectRegistry.reset();

			if (m_objectCache)
			{
				m_objectCache->clear();
				delete m_objectCache;
				m_objectCache = nullptr;
			}
		}
		
		void IBaseThread::advanceFrame_Thread(uint64_t frameIndex)
		{
			DEBUG_CHECK_EX(frameIndex == m_syncInfo.threadFrameIndex + 1, "Frame skipping");

			// if there was work submitted for the gpu insert a fence
			if (auto prevFrameIndex = m_syncInfo.threadFrameIndex)
			{
                insertGpuFrameFence_Thread(prevFrameIndex);
				// did we push anything ?
				//DEBUG_CHECK_EX(m_syncInfo.gpuStartedFrameIndex <= prevFrameIndex, "Frame skipping");
				//if (prevFrameIndex == m_syncInfo.gpuStartedFrameIndex)
			}

			// recording thread now reached new frame
			m_syncInfo.threadFrameIndex = frameIndex;
			m_syncQueues.recordQueue->signalNotifications(frameIndex);			
		}

		void IBaseThread::checkFences_Thread()
		{
			// if GPU if fast we may already be done, check the fences
			uint64_t completedFrame = 0;
			while (checkGpuFrameFence_Thread(completedFrame))
			{
				// GPU has finished work, signal queue
				DEBUG_CHECK_EX(completedFrame > m_syncInfo.gpuFinishedFrameIndex, "Frame skipping");
				m_syncInfo.gpuFinishedFrameIndex = completedFrame;
				m_syncQueues.gpuQueue->signalNotifications(completedFrame);
			}
		}

		DeviceSyncInfo IBaseThread::syncInfo() const
		{
			DeviceSyncInfo ret;
			ret.cpuFrameIndex = m_syncInfo.cpuFrameIndex;
			ret.threadFrameIndex = m_syncInfo.threadFrameIndex;
			//ret.gpuStartedFrameIndex = m_syncInfo.gpuStartedFrameIndex;
			ret.gpuFinishedFrameIndex = m_syncInfo.gpuFinishedFrameIndex;
			return ret;
		}

        void IBaseThread::sync(bool flush)
        {
			DEBUG_CHECK_RETURN_EX(Fibers::GetInstance().isMainThread(), "Expected main thread");

			base::ScopeTimer timer;
			PC_SCOPE_LVL0(FrameSync);

			// update windows
			//m_windows->updateWindows();

			// wait if previous cleanup job has not yet finished
			if (!m_frameSync.empty())
				Fibers::GetInstance().waitForCounterAndRelease(m_frameSync);
			//m_cleanupSync.acquire();

			// start new CPU frame
			const auto newFrameIndex = ++m_syncInfo.cpuFrameIndex;
			m_syncQueues.cpuQueue->signalNotifications(newFrameIndex);

			// create new signal
			auto newFrameSignal = Fibers::GetInstance().createCounter("FrameSync", 1);

			// push a cleanup job to release objects from frames that were completed
			pushJob([this, newFrameIndex, newFrameSignal, flush]()
				{
					//ASSERT_EX(m_syncInfo.gpuStartedFrameIndex <= newFrameIndex);
                    //m_syncInfo.gpuStartedFrameIndex = newFrameIndex;

					advanceFrame_Thread(newFrameIndex);

					if (flush)
					{
						syncGPU_Thread();
					}

					checkFences_Thread();				

					Fibers::GetInstance().signalCounter(newFrameSignal);
					//m_cleanupSync.release();
				});

			// wait for flush or not :)
			if (flush)
				Fibers::GetInstance().waitForCounterAndRelease(newFrameSignal);
			else
				m_frameSync = newFrameSignal;
			
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

		bool IBaseThread::registerCompletionCallback(DeviceCompletionType type, IDeviceCompletionCallback* callback)
		{
			DEBUG_CHECK_RETURN_EX_V(callback != nullptr, "No callback specified", false);

			switch (type)
			{
			case DeviceCompletionType::CPUFrameFinished:
				return m_syncQueues.gpuQueue->registerNotification(m_syncInfo.cpuFrameIndex, callback);

			case DeviceCompletionType::GPUFrameRecorded:
				return m_syncQueues.recordQueue->registerNotification(m_syncInfo.cpuFrameIndex, callback);

			case DeviceCompletionType::GPUFrameFinished:
				return m_syncQueues.gpuQueue->registerNotification(m_syncInfo.cpuFrameIndex, callback);
			}

			return false;
		}

		bool IBaseThread::registerCurrentFrameGPUComplectionCallback(const std::function<void(void)>& func)
		{
			return m_syncQueues.gpuQueue->registerNotification(m_syncInfo.threadFrameIndex, func);
		}

        void IBaseThread::scheduleObjectForDestruction(IBaseObject* ptr)
        {
			ASSERT(ptr && ptr->canDelete());

			TRACE_INFO("Scheduled {}({}) for deletion at frame {}, current recording: {}, finished on gpu: {}",
				(void*)ptr, ptr->objectType(), m_syncInfo.cpuFrameIndex, m_syncInfo.threadFrameIndex, m_syncInfo.gpuFinishedFrameIndex);

			m_syncQueues.gpuQueue->registerNotification(m_syncInfo.cpuFrameIndex, [ptr]()
				{
					delete ptr;
				});
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

		void BuildExecutionData(IBaseThread* drv, uint64_t cpuFrameIndex, PerformanceStats& outStats, command::CommandBuffer* commandBuffer, FrameExecutionData& outData)
		{
			PC_SCOPE_LVL2(BuildExecutionData);

			// timer
			base::ScopeTimer timer;

			// collect all referenced command buffers
			base::InplaceArray<command::CommandBuffer*, 100> allCommandBuffers;
			CollectCommandBuffers(commandBuffer, allCommandBuffers);

			{
				PC_SCOPE_LVL2(Collect);

				// report constant usage
				uint32_t currentConstantBufferSize = 0;
				for (auto& commandBuffer : allCommandBuffers)
				{
					auto prev = commandBuffer->gatheredState().constantUploadHead;
					for (auto cur = commandBuffer->gatheredState().constantUploadHead; cur; cur = cur->nextConstants)
					{
						ASSERT_EX(cur->dataSize <= outData.m_constantBufferSize, "Constant data can't be bigger than 64K, use other buffers");

						// start new buffer if overfilling 
						// TODO: reuse empty space in previous ones ?
						const auto alignedDataSize = base::Align<uint32_t>(cur->dataSize, outData.m_constantBufferAlignment);
						if (currentConstantBufferSize + alignedDataSize > outData.m_constantBufferSize)
						{
							auto& entry = outData.m_constantBuffers.emplaceBack();
							entry.usedSize = (uint16_t)currentConstantBufferSize;
							outStats.uploadConstantsBuffersCount += 1;
							outStats.uploadConstantsSize += currentConstantBufferSize;
							currentConstantBufferSize = 0;
						}

						// remember to copy the data :)
						auto& copyEntry = outData.m_constantBufferCopies.emplaceBack();
						copyEntry.bufferIndex = outData.m_constantBuffers.size();
						copyEntry.bufferOffset = currentConstantBufferSize;
						copyEntry.srcData = cur->dataPtr;
						copyEntry.srcDataSize = cur->dataSize;
						outStats.uploadConstantsCount += 1;

						// write back were we placed the data
						cur->bufferIndex = outData.m_constantBuffers.size();
						cur->bufferOffset = currentConstantBufferSize;
						currentConstantBufferSize += alignedDataSize;
					}
				}

				// finish last buffer
				if (currentConstantBufferSize > 0)
				{
					auto& entry = outData.m_constantBuffers.emplaceBack();
					entry.usedSize = (uint16_t)currentConstantBufferSize;
					outStats.uploadConstantsBuffersCount += 1;
					outStats.uploadConstantsSize += currentConstantBufferSize;
					currentConstantBufferSize = 0;
				}

				// report general need of staging area
				for (auto& commandBuffer : allCommandBuffers)
				{
					for (auto cur = commandBuffer->gatheredState().dynamicBufferUpdatesHead; cur; cur = cur->next)
					{
						ASSERT_EX(cur->dataBlockSize > 0, "Empty update");

						auto& entry = outData.m_stagingAreas.emplaceBack();
						entry.op = cur;

						outStats.uploadTotalSize += cur->dataBlockSize;
						outStats.uploadDynamicBufferCount += 1;
						outStats.uploadDynamicBufferSize += cur->dataBlockSize;
					}
				}
			}

			// update stats
			outStats.uploadTime = timer.timeElapsed();
		}

		//--

        void IBaseThread::submit(command::CommandBuffer* masterCommandBuffer)
        {
			DEBUG_CHECK_RETURN_EX(masterCommandBuffer, "No work");

			// get current cpu-side frame index that will protect all resources
			// NOTE: technically submit can happen from any thread but it's not recommended 
			// TODO: I feel race here if not used from main thread, especially close to the "advanceFrame"
			auto cpuFrameIndex = m_syncInfo.cpuFrameIndex;

            // create a job
            auto job = [this, masterCommandBuffer, cpuFrameIndex]()
            {
				base::ScopeTimer totalTime;

				// capture if requested
				auto capture = IFrameCapture::ConditionalStartCapture(masterCommandBuffer);

				// collect stats
				auto stats = base::RefNew<PerformanceStats>();

				// build the transient data for the frame
				FrameExecutionData executionData;
				executionData.m_constantBufferSize = m_constantBufferSize;
				executionData.m_constantBufferAlignment = m_constantBufferAlignment;
				BuildExecutionData(this, cpuFrameIndex, *stats, masterCommandBuffer, executionData);

                // execute frame
				execute_Thread(cpuFrameIndex, *stats, masterCommandBuffer, executionData);

				// update the sync info after sending first payload for current frame
				//m_syncInfo.gpuStartedFrameIndex = cpuFrameIndex;

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
				// process "sync" version of the background job queue
				if (m_backgroundQueue)
					m_backgroundQueue->update();

                // wait for job, it will be signaled on next job or after a timeout
                m_jobQueueSemaphore.wait(1);

				// check for gpu side completion of jobs
				checkFences_Thread();

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
