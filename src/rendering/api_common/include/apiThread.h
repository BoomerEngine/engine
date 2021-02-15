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
#include "rendering/device/include/renderingDeviceApi.h"

namespace rendering
{
	namespace api
	{
		//---

		// a general API-side device thread, all things happen here
		class RENDERING_API_COMMON_API IBaseThread : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_API_RUNTIME)

		public:
			IBaseThread(IBaseDevice* drv, WindowManager* windows);
			virtual ~IBaseThread();

			//--
			
			// object registry - contains all public objects indexed via their handle (ObjectID)
			INLINE ObjectRegistry* objectRegistry() const { return m_objectRegistry; }

			// object registry - contains internal API objects
			INLINE IBaseObjectCache* objectCache() const { return m_objectCache; }

			// background job queue (shader compilation mostly)
			INLINE IBaseBackgroundQueue* backgroundQueue() const { return m_backgroundQueue; }

			//--

			// start device thread, initializes API
			bool startThread(const base::app::CommandLine& cmdLine, DeviceCaps& outCaps);

			// finish device thread, closes API
			void stopThread();

			//--

			// API object is no longer needed, add it to current frame as an object to delete once frame finished
			void scheduleObjectForDestruction(IBaseObject* ptr);

			// request a completion callback
			bool registerCompletionCallback(DeviceCompletionType type, IDeviceCompletionCallback* callback);

			// request a completion callback for current frame finishing on GPU
			bool registerCurrentFrameGPUComplectionCallback(const std::function<void(void)>& func);

			//--

			// get sync information
			DeviceSyncInfo syncInfo() const;

			// sync processing between host, render thread and GPU (with optional full pipeline flush)
			void sync(bool flush);

			// run a custom function in the thread context
			void run(const std::function<void()>& func);

			// submit command buffers for execution
			void submit(command::CommandBuffer* masterCommandBuffer);

			//--

			// create swapchain for specific output 
			// NOTE: this is OS/third party specific, can be called from any thread but can be internally synchronized if needed
			virtual IBaseSwapchain* createOptimalSwapchain(const OutputInitInfo& info) = 0;

			// create buffer data object
			virtual IBaseBuffer* createOptimalBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData) = 0;

			// create image data object
			virtual IBaseImage* createOptimalImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData) = 0;

			// create sampler data 
			virtual IBaseSampler* createOptimalSampler(const SamplerState& state) = 0;

			// create shaders
			virtual IBaseShaders* createOptimalShaders(const ShaderData* data) = 0;

			//-----

			// sync GPU (stop all work), usually inserts a GPU fence and waits for it
			// NOTE: called from render thread
			virtual void syncGPU_Thread() = 0;

			// record native command buffers
			// NOTE: called from render thread
			virtual void execute_Thread(uint64_t frameIndex, PerformanceStats& stats, command::CommandBuffer* masterCommandBuffer, const FrameExecutionData& data) = 0;

			//--

		protected:
			volatile bool m_requestExit = false;

			base::Thread m_thread;
			uint32_t m_threadId = 0;

			IBaseDevice* m_device = nullptr;
			WindowManager* m_windows = nullptr;

			uint32_t m_constantBufferAlignment = 0;
			uint32_t m_constantBufferSize = 0;

			struct
			{
				volatile uint32_t cpuFrameIndex;
				volatile uint32_t threadFrameIndex;
				//volatile uint32_t gpuStartedFrameIndex;
				volatile uint32_t gpuFinishedFrameIndex;
			} m_syncInfo;			

			struct
			{
				FrameCompleteionQueue* cpuQueue = nullptr;
				FrameCompleteionQueue* recordQueue = nullptr; // signaled at the end of the recording
				FrameCompleteionQueue* gpuQueue = nullptr; // signaled at the end of the GPU work
			} m_syncQueues;

			//--

			
			base::RefPtr<ObjectRegistry> m_objectRegistry = nullptr;

			IBaseObjectCache* m_objectCache = nullptr;

			IBaseBackgroundQueue* m_backgroundQueue = nullptr;

			//--

			base::fibers::SyncPoint m_cleanupSync;
			base::fibers::WaitCounter m_frameSync;

			//--

			struct Job : public base::NoCopy
			{
				RTTI_DECLARE_POOL(POOL_API_RUNTIME)

			public:
				std::function<void()> m_jobFunc;
			};

			base::Queue<Job*> m_jobQueue;
			base::SpinLock m_jobQueueLock;
			base::Semaphore m_jobQueueSemaphore;

			Job* popJob();
			void pushJob(const std::function<void()>& func);

			//--

			virtual IBaseObjectCache* createOptimalObjectCache(const base::app::CommandLine& cmdLine) = 0;

			virtual IBaseBackgroundQueue* createOptimalBackgroundQueue(const base::app::CommandLine& cmdLine) = 0;

			virtual ObjectRegistry* createOptimalObjectRegistry(const base::app::CommandLine& cmdLine);

			virtual bool threadStartup(const base::app::CommandLine& cmdLine, DeviceCaps& outCaps); // called on thread to initialize API
			virtual void threadFinish(); // called on thread to initialize API

			void threadFunc();

			void advanceFrame_Thread(uint64_t frameIndex);
			void checkFences_Thread();

			virtual void insertGpuFrameFence_Thread(uint64_t frameIndex) = 0;

			virtual bool checkGpuFrameFence_Thread(uint64_t& outFrameIndex) = 0;

			friend class Frame;
		};

		//---

	} // api
} // rendering